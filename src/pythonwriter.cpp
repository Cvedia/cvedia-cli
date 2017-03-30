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
#include <unistd.h>
#include <dlfcn.h>

using namespace std;

#include "config.hpp"

#ifdef HAVE_PYTHON

#define NPY_NO_DEPRECATED_API NPY_1_7_API_VERSION

#include "Python.h"
#include "numpy/arrayobject.h"

#include "easylogging++.h"
#include "cvedia.hpp"
#include "api.hpp"
#include "pythonwriter.hpp"

PythonWriter::PythonWriter(string export_name, map<string, string> options) {

	LOG(INFO) << "Initializing PythonWriter";

	mModuleOptions = options;
	mExportName = export_name;
}

PythonWriter::~PythonWriter() {

}

WriterStats PythonWriter::GetStats() {

	return mCsvStats;
}

void PythonWriter::ClearStats() {

	// Clear the stats structure
	mCsvStats = {};
}

bool PythonWriter::CanHandle(string support) {

	bool can_handle = false;
	PyObject* pTuple = PyTuple_New(1);
	PyObject* feat = PyUnicode_FromString(support.c_str());

	PyTuple_SetItem(pTuple, 0, feat);

	// Call the can_handle function of our python script
	PyObject* rslt = PyObject_CallObject(pCanHandleFn, pTuple);

	Py_XDECREF(pTuple);

	if (rslt == NULL) {
//		PyErr_PrintEx(0);
		return false;
	}

	if (PyObject_IsTrue(rslt))
		can_handle = true;

	Py_XDECREF(rslt);

	return can_handle;
}

int PythonWriter::Initialize(DatasetMetadata* dataset_meta, int mode) {

	dlopen("libpython3.5m.so", RTLD_NOW | RTLD_GLOBAL);
	Py_InitializeEx(0);
	PyEval_InitThreads();
	import_array();

	PyGILState_STATE gstate;
	gstate = PyGILState_Ensure();

	PySys_SetArgvEx(0, NULL, 0);

	ifstream script_file("python/tfrecordswriter.py");
	if (!script_file.is_open()) {
        LOG(ERROR) << "Error reading script file python/tfrecordswriter.py";
		return -1;
	}

	string script_data((istreambuf_iterator<char>(script_file)),
				istreambuf_iterator<char>());

	// Load the python entry points for later execution
	PyObject* pCompiledFn = Py_CompileString(script_data.c_str() , "" , Py_file_input);
	if (pCompiledFn == NULL) {
		PyErr_PrintEx(0);
        LOG(ERROR) << "Error running Py_CompileString";
		return -1;
	}

	PyObject* pModule = PyImport_ExecCodeModule("cvedia" , pCompiledFn);
	if (pModule == NULL) {
		PyErr_PrintEx(0);
        LOG(ERROR) << "Python error";
        return -1;
	}

    Py_XDECREF(pCompiledFn);

    // Find the initialize function
	pInitFn = PyObject_GetAttrString(pModule ,"initialize");
	if (pInitFn == NULL) {
		PyErr_PrintEx(0);
        LOG(ERROR) << "Could not find 'initialize' def in Python script";
		return -1;
	}

    // Find the initialize function
	pCanHandleFn = PyObject_GetAttrString(pModule ,"can_handle");
	if (pCanHandleFn == NULL) {
		PyErr_PrintEx(0);
        LOG(ERROR) << "Could not find 'can_handle' def in Python script";
		return -1;
	}

    // Find the begin_writing function
	pBeginWritingFn = PyObject_GetAttrString(pModule ,"begin_writing");
	if (pBeginWritingFn == NULL) {
		PyErr_PrintEx(0);
        LOG(ERROR) << "Could not find 'begin_writing' def in Python script";
		return -1;
	}

    // Find the end_writing function
	pEndWritingFn = PyObject_GetAttrString(pModule ,"end_writing");
	if (pEndWritingFn == NULL) {
		PyErr_PrintEx(0);
        LOG(ERROR) << "Could not find 'end_writing' def in Python script";
		return -1;
	}

	// Find the write function
	pWriteFn = PyObject_GetAttrString(pModule ,"write_data");
	if (pWriteFn == NULL) {
		PyErr_PrintEx(0);
        LOG(ERROR) << "Could not find 'write_data' def in Python script";
		return -1;
	}

	// Find the check_integrity function
	pCheckIntegrityFn = PyObject_GetAttrString(pModule ,"check_integrity");
	if (pCheckIntegrityFn == NULL) {

		if (CanHandle("integrity")) {
			PyErr_PrintEx(0);
	        LOG(ERROR) << "Could not find 'check_integrity' def in Python script";
			return -1;
		}
	}

	// Find the finalize function
	pFinalFn = PyObject_GetAttrString(pModule ,"finalize");
	if (pFinalFn == NULL) {
		PyErr_PrintEx(0);
        LOG(ERROR) << "Could not find 'finalize' def in Python script";
		return -1;
	}
	
	// Hold on to these for later
	Py_XINCREF(pBeginWritingFn);
	Py_XINCREF(pWriteFn);
	Py_XINCREF(pEndWritingFn);
	Py_XINCREF(pFinalFn);

    Py_XDECREF(pModule);

	mBasePath = mModuleOptions["working_dir"];

    PyObject* dict_module_options = PyDict_New();
    PyObject* dict_meta = PyDict_New();
    vector<PyObject* > pyObjs;
	PyObject* pySetList = PyList_New(0);
	PyObject* pyFieldList = PyList_New(0);

	// Build a list of all DatasetMetadata
    for (auto const& entry : dataset_meta->mapping_by_id) {
	    PyObject* dict_field = PyDict_New();

    	PyObject* id_key = PyUnicode_FromString("id");
    	PyObject* id_value = PyLong_FromLong(entry->id);

    	PyObject* name_key = PyUnicode_FromString("name");
    	PyObject* name_value = PyUnicode_FromString(entry->name.c_str());

		PyDict_SetItem(dict_field, name_key, name_value);
		PyDict_SetItem(dict_field, id_key, id_value);

		PyList_Append(pyFieldList, dict_field);

		pyObjs.push_back(dict_field);
		pyObjs.push_back(id_key);
		pyObjs.push_back(id_value);
		pyObjs.push_back(name_key);
		pyObjs.push_back(name_value);
    }

	// Build a list of all DatasetMetadata
    for (auto const& entry : dataset_meta->sets) {
    	PyObject* set_name = PyUnicode_FromString(entry.set_name.c_str());
		PyList_Append(pySetList, set_name);
		pyObjs.push_back(set_name);
    }

    // Convert our std::map to a python dictionary
    for (auto const& entry : mModuleOptions) {
    	PyObject* key = PyUnicode_FromString(entry.first.c_str());
    	PyObject* val = PyUnicode_FromString(entry.second.c_str());

		PyDict_SetItem(dict_module_options, key, val);

		pyObjs.push_back(key);
		pyObjs.push_back(val);
	}

	// Add Sets to a 'sets' dictionary
	PyObject* sets_key = PyUnicode_FromString("sets");
	PyObject* fields_key = PyUnicode_FromString("fields");
	pyObjs.push_back(sets_key);
	pyObjs.push_back(fields_key);
	PyDict_SetItem(dict_meta, sets_key, pySetList);
	PyDict_SetItem(dict_meta, fields_key, pyFieldList);

	// Put dictionary in a tuple
	PyObject* pTuple = PyTuple_New(3);
	PyTuple_SetItem( pTuple, 0, dict_module_options);
	PyTuple_SetItem( pTuple, 1, dict_meta);
	PyTuple_SetItem( pTuple, 2, PyLong_FromLong(mode));
	Py_XINCREF(pTuple);

	// Call the initialize function of our python script
	PyObject* rslt = PyObject_CallObject(pInitFn, pTuple);

	if (rslt == NULL) {
		PyErr_PrintEx(0);
		return -1;
	}

	Py_XDECREF(pInitFn);
	Py_XDECREF(rslt);
	Py_XDECREF(pTuple);
	Py_XDECREF(dict_module_options);

	// Objects are no longer required
	for (PyObject* obj : pyObjs)
		Py_XDECREF(obj);

	mInitialized = 1;

//	PyGILState_Release(gstate);

	return 0;
}

string PythonWriter::PrepareData(Metadata* meta) {

	string file_format = "$FILENAME $CATEGORY\n";

	string output_line = file_format;

	return output_line;
}

int PythonWriter::Finalize() {

	// Call the finalize function of our python script
	PyObject* rslt = PyObject_CallObject(pFinalFn, NULL);

	if (rslt == NULL) {
		PyErr_PrintEx(0);
		return -1;
	} else {
		Py_XDECREF(rslt);
		Py_XDECREF(pFinalFn);
	}

	return 0;
}

string PythonWriter::CheckIntegrity(string file_name) {

	string result = "";

	PyObject* pTuple = PyTuple_New(1);
	PyObject* fname = PyUnicode_FromString(file_name.c_str());

	PyTuple_SetItem(pTuple, 0, fname);

	// Call the initialize function of our python script
	PyObject* rslt = PyObject_CallObject(pCheckIntegrityFn, pTuple);

	Py_XDECREF(pTuple);

	if (rslt == NULL) {
		PyErr_PrintEx(0);
		return "";
	}

	if (!rslt) {
		Py_XDECREF(rslt);
		LOG(ERROR) << "Call to Python function 'check_integrity' failed";
		return "";
	} else {
		PyObject* ascii_result = PyUnicode_AsASCIIString(rslt);
		result = PyBytes_AsString(ascii_result);
		Py_DECREF(ascii_result);		
	}

	Py_XDECREF(rslt);

	return result;
}

int PythonWriter::BeginWriting() {

	// Call the begin_writing function of our python script
	PyObject* rslt = PyObject_CallObject(pBeginWritingFn, NULL);

	if (rslt == NULL) {
		PyErr_PrintEx(0);
		return -1;
	}

	Py_XDECREF(rslt);

	return 0;
}

string PythonWriter::WriteData(Metadata* meta) {

	string result = "";

	if (!mInitialized) {
		LOG(ERROR) << "Must call Initialize() first";
		return "";
	}

    PyObject* pyMetaentry = PyList_New(0);
    PyObject* entry_dict = PyDict_New();

	PyObject* pTuple = PyTuple_New(1);

	for (MetadataEntry* e : meta->entries) {
	    PyObject* meta_dict = PyDict_New();

		AddToDict(meta_dict, PyUnicode_FromString("value_type"), PyUnicode_FromString(e->value_type.c_str()));

		// We need to convert a vector to python array. This is either raw telemetry 
		// or raw image format
		if (e->value_type == METADATA_VALUE_TYPE_RAW) {

			PyObject* pyArr;
			npy_intp dims[1];

			// Check if we're in Byte or Float format
			if (e->dtype == "uint8") {
				dims[0] = e->uint8_raw_data.size();

				// Create new array
				pyArr = PyArray_SimpleNewFromData(1, dims, NPY_UINT8, &e->uint8_raw_data[0]);

			} else if (e->dtype == "float") {
				dims[0] = e->float_raw_data.size();
				
				// Create new array
				pyArr = PyArray_SimpleNewFromData(1, dims, NPY_DOUBLE, &e->float_raw_data[0]);

			} else {
				LOG(ERROR) << "Unsupported dtype '" << e->dtype << "' passed";
				return "";
			}

			// Add numpy array to dictionary	
			AddToDict(meta_dict, PyUnicode_FromString("data"), pyArr);

		// Data is a regular image. Pass as an array nontheless
		} else if (e->value_type == METADATA_VALUE_TYPE_IMAGE) {
			
			PyObject* pyArr;
			npy_intp dims[1] = {(npy_intp)e->image_data.size()};

			// Create new array
			pyArr = PyArray_SimpleNewFromData(1, dims, NPY_BYTE, &e->image_data[0]);

			// Add numpy array to dictionary
			AddToDict(meta_dict, PyUnicode_FromString("imagedata"), pyArr);

		// Numerical value passed, check in which format
		} else if (e->value_type == METADATA_VALUE_TYPE_NUMERIC) {

			if (e->dtype == "int") {
				AddToDict(meta_dict, PyUnicode_FromString("value"), PyLong_FromLong(e->int_value));
			} else if (e->dtype == "float") {
				AddToDict(meta_dict, PyUnicode_FromString("value"), PyFloat_FromDouble(e->float_value));
			} else {
				LOG(ERROR) << "Unsupported dtype '" << e->dtype << "' passed";
				return "";				
			}
		} else if (e->value_type == METADATA_VALUE_TYPE_STRING) {
//			cout << "." << endl;
//			cout << e->string_value[0].c_str() << endl;
			AddToDict(meta_dict, PyUnicode_FromString("value"), PyUnicode_FromString(e->string_value.c_str()));
		}

		if (e->id > -1) {
			AddToDict(meta_dict, PyUnicode_FromString("id"), PyLong_FromLong(e->id));
		}

		if (e->dtype != "") {
			AddToDict(meta_dict, PyUnicode_FromString("dtype"), PyUnicode_FromString(e->dtype.c_str()));
		}

		if (e->file_uri != "") {
			AddToDict(meta_dict, PyUnicode_FromString("file_uri"), PyUnicode_FromString(e->file_uri.c_str()));
		}

		if (e->filename != "") {
			AddToDict(meta_dict, PyUnicode_FromString("filename"), PyUnicode_FromString(e->filename.c_str()));
		}

		PyList_Append(pyMetaentry, meta_dict);
		Py_XDECREF(meta_dict);
	}

	AddToDict(entry_dict, PyUnicode_FromString("set"), PyUnicode_FromString(meta->setname.c_str()));
	AddToDict(entry_dict, PyUnicode_FromString("entries"), pyMetaentry);

	// Store the 2nd and 3rd argument
	PyTuple_SetItem(pTuple, 0, entry_dict);

	// Call the initialize function of our python script
	PyObject* rslt = PyObject_CallObject(pWriteFn, pTuple);

	Py_XDECREF(pTuple);
	Py_XDECREF(pyMetaentry);

	if (rslt == NULL) {
		PyErr_PrintEx(0);
		return "";
	}

	if (!rslt) {
		Py_XDECREF(rslt);
		LOG(ERROR) << "Call to Python function 'write_data' failed";
		return "";
	} else {
		PyObject* ascii_result = PyUnicode_AsASCIIString(rslt);
		result = PyBytes_AsString(ascii_result);
		Py_DECREF(ascii_result);		
	}

	Py_XDECREF(rslt);

	return result;
}

int PythonWriter::EndWriting() {

	// Call the end_writing function of our python script
	PyObject* rslt = PyObject_CallObject(pEndWritingFn, NULL);

	if (rslt == NULL) {
		PyErr_PrintEx(0);
		return -1;
	}

	Py_XDECREF(rslt);

	return 0;
}

void PythonWriter::AddToDict(PyObject* dict, PyObject* key, PyObject* val) {

	PyDict_SetItem(dict, key, val);
	Py_XDECREF(key);
	Py_XDECREF(val);
}

#endif
