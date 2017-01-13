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

int PythonWriter::Initialize(DatasetMetadata* dataset_meta) {

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

	// Find the write function
	pWriteFn = PyObject_GetAttrString(pModule ,"write_data");
	if (pWriteFn == NULL) {
		PyErr_PrintEx(0);
        LOG(ERROR) << "Could not find 'write_data' def in Python script";
		return -1;
	}

	// Find the finalize function
	pFinalFn = PyObject_GetAttrString(pModule ,"finalize");
	if (pFinalFn == NULL) {
		PyErr_PrintEx(0);
        LOG(ERROR) << "Could not find 'finalize' def in Python script";
		return -1;
	}
	
	// Hold on to these for later
	Py_XINCREF(pWriteFn);
	Py_XINCREF(pFinalFn);

    Py_XDECREF(pModule);

	mBasePath = mModuleOptions["working_dir"];

    PyObject* dict = PyDict_New();
    vector<PyObject* > pyObjs;

    // Convert our std::map to a python dictionary
    for (auto const& entry : mModuleOptions) {
    	PyObject* key = PyUnicode_FromString(entry.first.c_str());
    	PyObject* val = PyUnicode_FromString(entry.second.c_str());

		PyDict_SetItem(dict, key, val);

		pyObjs.push_back(key);
		pyObjs.push_back(val);
	}

	// Put dictionary in a tuple
	PyObject* pTuple = PyTuple_New(1);
	PyTuple_SetItem( pTuple, 0, dict);
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
	Py_XDECREF(dict);

	// Objects are no longer required
	for (PyObject* obj : pyObjs)
		Py_XDECREF(obj);

	mInitialized = 1;

//	PyGILState_Release(gstate);

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

int PythonWriter::WriteData(Metadata* meta) {

	if (!mInitialized) {
		LOG(ERROR) << "Must call Initialize() first";
		return -1;
	}

	MetadataEntry* source = NULL;

	for (MetadataEntry* e : meta->entries) {
		if (e->meta_type == METADATA_TYPE_SOURCE)
			source = e;		
	}

	if (mModuleOptions.count("images-external")) {
		string file_uri = WriteImageData(source->filename, source->image_data);
		source->file_uri = file_uri;
	}

    PyObject* dict = PyDict_New();

    PyObject* pyListSource = PyList_New(0);
    PyObject* pyListGround = PyList_New(0);

	PyObject* pTuple = PyTuple_New(3);

	AddToDict(dict, PyUnicode_FromString("type"), PyUnicode_FromString(meta->type.c_str()));
	PyTuple_SetItem(pTuple, 0, dict);

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
				return -1;
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
				return -1;				
			}
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

		if (e->meta_type == METADATA_TYPE_SOURCE)
			PyList_Append(pyListSource, meta_dict);
		else if (e->meta_type == METADATA_TYPE_GROUND)
			PyList_Append(pyListGround, meta_dict);
		else {
			LOG(ERROR) << "Unsupported meta_type encountered: " << e->meta_type;
			return -1;			
		}
	
		Py_XDECREF(meta_dict);
	}

	// Store the 2nd and 3rd argument
	PyTuple_SetItem(pTuple, 1, pyListSource);
	PyTuple_SetItem(pTuple, 2, pyListGround);

	// Call the initialize function of our python script
	PyObject* rslt = PyObject_CallObject(pWriteFn, pTuple);

	Py_XDECREF(pTuple);

	if (rslt == NULL) {
		PyErr_PrintEx(0);
		return -1;
	}

	if (!PyObject_IsTrue(rslt)) {
		Py_XDECREF(rslt);
		LOG(ERROR) << "Call to Python function 'write_data' failed";
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

string PythonWriter::WriteImageData(string filename, vector<uint8_t> image_data) {

	string path = mBasePath + "data/";

	int dir_err = mkdir(path.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
	if (dir_err == -1 && errno != EEXIST) {
		LOG(ERROR) << "Could not create directory: " << path;
		return "";
	}

	// Must be at least 7 chars long for directory splitting (3 chars + 4 for extension)
	if (filename.size() > 7 && mModuleOptions.count("images-same-dir") == 0) {

		for (int i = 0; i < 3; ++i) {
			path += filename.substr(i,1) + "/";

			int dir_err = mkdir(path.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
			if (dir_err == -1 && errno != EEXIST) {
				LOG(ERROR) << "Could not create directory: " << path;
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
