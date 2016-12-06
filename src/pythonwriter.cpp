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

#include "example.pb.h"
#include "cvedia.hpp"
#include "api.hpp"
#include "pythonwriter.hpp"

PythonWriter::PythonWriter(string export_name, map<string, string> options) {

	WriteDebugLog("Initializing PythonWriter");

	mModuleOptions = options;
	mExportName = export_name;
}

PythonWriter::~PythonWriter() {

//	PyGILState_Release(gstate);

	if (mTrainFile.is_open()) {
		mTrainFile.close();
	}
	if (mTestFile.is_open()) {
		mTestFile.close();
	}
	if (mValidateFile.is_open()) {
		mValidateFile.close();
	}
}

WriterStats PythonWriter::GetStats() {

	return mCsvStats;
}

void PythonWriter::ClearStats() {

	// Clear the stats structure
	mCsvStats = {};
}

int PythonWriter::Initialize() {

	dlopen("libpython3.5m.so", RTLD_NOW | RTLD_GLOBAL);
	Py_Initialize();
	import_array();

	ifstream script_file("python/tfrecordswriter.py");
	if (script_file == NULL) {
        WriteErrorLog(string("Error reading script file python/tfrecordswriter.py").c_str());
		return -1;
	}

	string script_data((istreambuf_iterator<char>(script_file)),
				istreambuf_iterator<char>());

	// Load the python entry points for later execution
	PyObject* pCompiledFn = Py_CompileString(script_data.c_str() , "" , Py_file_input);
	if (pCompiledFn == NULL) {
		PyErr_PrintEx(0);
        WriteErrorLog("Error running Py_CompileString");
		return -1;
	}

	PyObject* pModule = PyImport_ExecCodeModule("cvedia" , pCompiledFn);
	if (pModule == NULL) {
		PyErr_PrintEx(0);
        WriteErrorLog("Python error");
        return -1;
	}

    Py_XDECREF(pCompiledFn);

    // Find the initialize function
	pInitFn = PyObject_GetAttrString(pModule ,"initialize");
	if (pInitFn == NULL) {
		PyErr_PrintEx(0);
        WriteErrorLog("Could not find 'initialize' def in Python script");
		return -1;
	}

	// Find the write function
	pWriteFn = PyObject_GetAttrString(pModule ,"write_data");
	if (pWriteFn == NULL) {
		PyErr_PrintEx(0);
        WriteErrorLog("Could not find 'write_data' def in Python script");
		return -1;
	}

	// Find the finalize function
	pFinalFn = PyObject_GetAttrString(pModule ,"finalize");
	if (pFinalFn == NULL) {
		PyErr_PrintEx(0);
        WriteErrorLog("Could not find 'finalize' def in Python script");
		return -1;
	}

    Py_XDECREF(pModule);

	mBasePath = mModuleOptions["working_dir"];

    PyObject* dict = PyDict_New();
    vector<PyObject* > pyObjs;

    // Convert our std::map to a python dictionary
    for(auto const& entry : mModuleOptions) {
    	PyObject* key = PyUnicode_FromString(entry.first.c_str());
    	PyObject* val = PyUnicode_FromString(entry.second.c_str());

		PyDict_SetItem(dict, key, val);

		pyObjs.push_back(key);
		pyObjs.push_back(val);
	}

	// Put dictionary in a tuple
	PyObject* pTuple = PyTuple_New(1);
	PyTuple_SetItem( pTuple, 0, dict);
	Py_INCREF(pTuple);

	// Call the initialize function of our python script
	PyObject* rslt = PyObject_CallObject(pInitFn, pTuple);
	Py_XDECREF(pInitFn);
	Py_XDECREF(rslt);
	Py_XDECREF(pTuple);
	Py_XDECREF(dict);

	// Objects are no longer required
	for (PyObject* obj : pyObjs)
		Py_XDECREF(obj);

	mInitialized = 1;

	return 0;
}

bool PythonWriter::ValidateData(vector<Metadata* > meta) {

	return true;
}


string PythonWriter::PrepareData(Metadata* meta) {

	string file_format = "$FILENAME $CATEGORY\n";

	string output_line = file_format;

	return output_line;
}

int PythonWriter::Finalize() {

	return 0;
}

int PythonWriter::WriteData(Metadata* meta) {

	if (!mInitialized) {
		WriteErrorLog("Must call Initialize() first");
		return -1;
	}

	MetadataEntry* source = NULL;
	MetadataEntry* ground = NULL;

	for (MetadataEntry* e : meta->entries) {
		if (e->meta_type == METADATA_TYPE_SOURCE)
			source = e;		
		if (e->meta_type == METADATA_TYPE_GROUND)
			ground = e;
	}

	if (mModuleOptions.count("images-external")) {
		string file_uri = WriteImageData(source->filename, source->image_data);
		source->file_uri = file_uri;
	}

    PyObject* dict = PyDict_New();
    vector<PyObject* > pyObjs;

    // Convert our Metadata to a python dictionary
	PyObject* key = NULL;
	PyObject* val = NULL; 

	key = PyUnicode_FromString("type");
	val = PyUnicode_FromString(to_string(meta->type).c_str());

	PyDict_SetItem(dict, key, val);

	pyObjs.push_back(key);
	pyObjs.push_back(val);

	// Put dictionary in a tuple
	PyObject* pTuple = PyTuple_New(1);
	PyTuple_SetItem( pTuple, 0, dict);
	Py_INCREF(pTuple);

	// Call the initialize function of our python script
	PyObject* rslt = PyObject_CallObject(pWriteFn, pTuple);
	Py_XDECREF(rslt);
	Py_XDECREF(pTuple);
	Py_XDECREF(dict);

	// Objects are no longer required
	for (PyObject* obj : pyObjs)
		Py_XDECREF(obj);

	return 0;
}

string PythonWriter::WriteImageData(string filename, vector<uint8_t> image_data) {

	string path = mBasePath + "data/";

	int dir_err = mkdir(path.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
	if (dir_err == -1 && errno != EEXIST) {
		WriteErrorLog(string("Could not create directory: " + path).c_str());
		return "";
	}

	// Must be at least 7 chars long for directory splitting (3 chars + 4 for extension)
	if (filename.size() > 7 && mModuleOptions.count("images-same-dir") == 0) {

		for (int i = 0; i < 3; ++i) {
			path += filename.substr(i,1) + "/";

			int dir_err = mkdir(path.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
			if (dir_err == -1 && errno != EEXIST) {
				WriteErrorLog(string("Could not create directory: " + path).c_str());
				return "";
			}
		}
	}

	ofstream image_file;
	image_file.open(path + filename, ios::out | ios::trunc | ios::binary);
	image_file.write((char *)&image_data[0], image_data.size());
	image_file.close();		

	mImagesWritten++;
	
	return path + filename;
}

#endif
