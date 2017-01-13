#include <deque>
#include <map>
#include <vector>
#include <string>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdio.h>
#include <errno.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <unistd.h>
#include <dlfcn.h>

using namespace std;

#include "config.hpp"

#ifdef HAVE_PYTHON

#define NPY_NO_DEPRECATED_API NPY_1_7_API_VERSION

#include "Python.h"
#include "numpy/arrayobject.h"

#include "easylogging++.h"
#include "functions.hpp"
#include "cvedia.hpp"
#include "api.hpp"
#include "pythonmodules.hpp"

int PythonModules::LoadPythonCore() {
	dlopen("libpython3.5m.so", RTLD_NOW | RTLD_GLOBAL);
	Py_InitializeEx(0);
	PyEval_InitThreads();
	import_array();
}

vector<export_module> PythonModules::ListModules(string path) {

	vector<export_module> modules;

	DIR* dir;
	struct dirent* p_ent;

	vector<string> python_scripts;

	PyGILState_STATE gstate;
	gstate = PyGILState_Ensure();

	PySys_SetArgvEx(0, NULL, 0);

	if ((dir = opendir(path.c_str())) != NULL) {
		while ((p_ent = readdir(dir)) != NULL) {

			string fname(p_ent->d_name);

			if (hasEnding(fname,".py")) {
				python_scripts.push_back(path + fname);
			}
		}

		closedir(dir);
	} else {
		LOG(ERROR) << "Could not open python module folder at " << path;
		return modules;
	}

	if (python_scripts.size() == 0) {
		LOG(ERROR) << "No python modules found";
		return modules;		
	}

	for (string script_file : python_scripts) {
		LOG(DEBUG) << "Reading module options from python script " << script_file;

		export_module module;

		ifstream script_stream(script_file);
		if (!script_stream.is_open()) {
			LOG(ERROR) << "Error reading script file " << script_file;
			continue;
		}

		string script_data((istreambuf_iterator<char>(script_stream)),
					istreambuf_iterator<char>());

		// Load the python entry points for later execution
		PyObject* pCompiledFn = Py_CompileString(script_data.c_str() , "" , Py_file_input);
		if (pCompiledFn == NULL) {
			PyErr_PrintEx(0);
	        LOG(ERROR) << "Error running Py_CompileString";
	        continue;
		}

		PyObject* pModule = PyImport_ExecCodeModule("check_module" , pCompiledFn);
		if (pModule == NULL) {
			if (gDebug == 1) {
				PyErr_PrintEx(0);
		        LOG(ERROR) << "Python error";
	        }
	        continue;
		}

	    Py_XDECREF(pCompiledFn);

	    // Find the initialize function
		PyObject* p_loadmodule;
		p_loadmodule = PyObject_GetAttrString(pModule ,"load_module");
		if (p_loadmodule == NULL) {
			PyErr_PrintEx(0);
	        LOG(ERROR) << "Could not find 'initialize' def in Python script";
	        continue;
		}

		// Call the initialize function of our python script
		PyObject* dict = PyObject_CallObject(p_loadmodule, NULL);

		if (dict == NULL) {
			PyErr_PrintEx(0);
			continue;
		}

		if (!PyDict_Check(dict)) {
			Py_XDECREF(dict);
			LOG(ERROR) << "Call to 'load_module' did not return a pyton dictionary";
			continue;
		}

		char* p_modulename = PyUnicode_AsUTF8(PyDict_GetItemString(dict, "module_name"));
		string module_name(p_modulename);
		Py_XDECREF(p_modulename);

		module.module_name = module_name;

		PyObject* pyList = PyDict_GetItemString(dict, "params");
		int param_cnt = PyList_Size(pyList);

		for (int param_idx=0; param_idx<param_cnt; param_idx++) {

			PyObject* pyDictEntry = PyList_GetItem(pyList, param_idx);

			bool required 	= (Py_True == PyDict_GetItemString(pyDictEntry, "required"));
			char* p_option 	= PyUnicode_AsUTF8(PyDict_GetItemString(pyDictEntry, "option"));
			char* p_example = PyUnicode_AsUTF8(PyDict_GetItemString(pyDictEntry, "example"));
			char* p_descr 	= PyUnicode_AsUTF8(PyDict_GetItemString(pyDictEntry, "description"));

			string param_option(p_option);
			string param_example(p_example);
			string param_descr(p_descr);

			export_module_param params;

			params.required = required;
			params.option = param_option;
			params.example = param_example;
			params.description = param_descr;

			module.module_params.push_back(params);

			Py_XDECREF(p_option);
			Py_XDECREF(p_example);
			Py_XDECREF(p_descr);

			Py_XDECREF(pyDictEntry);
		}

		Py_XDECREF(pyList);

		Py_XDECREF(dict);

		// Done
	    Py_XDECREF(p_loadmodule);

	    modules.push_back(module);
	}

	return modules;
}

#endif
