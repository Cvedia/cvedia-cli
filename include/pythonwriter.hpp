#ifndef _PYTHONWRITER_HPP
#define _PYTHONWRITER_HPP

using namespace std;

#include "config.hpp"

#ifdef HAVE_PYTHON

#include "Python.h"
#include "datawriter.hpp"
#include "api.hpp"

class PythonWriter: public IDataWriter {

public:
	PythonWriter(string export_name, map<string, string> options);
	PythonWriter() {};
	~PythonWriter();

	WriterStats GetStats();
	void ClearStats();
	
	virtual int BeginWriting(DatasetMetadata* dataset_meta);
	string WriteData(Metadata* meta);
	virtual int EndWriting(DatasetMetadata* dataset_meta);
	virtual string VerifyData(string file_name, DatasetMetadata* dataset_meta);

	virtual int Initialize(DatasetMetadata* dataset_meta);
	virtual int Finalize();

	string mBasePath;
	string mBaseDir;
	string mExportName;

	int mImagesWritten = 0;

	map<string, string> mModuleOptions;

private:

	PyGILState_STATE gstate;

	PyObject* pInitFn;
	PyObject* pWriteFn;
	PyObject* pFinalFn;

	virtual bool ValidateData(vector<Metadata* > meta);
	virtual string PrepareData(Metadata* meta);
	string WriteImageData(string filename, vector<uint8_t> image_data);
	void AddToDict(PyObject* dict, PyObject* key, PyObject* val);

	WriterStats mCsvStats;

	bool mInitialized;

	bool mCreateTrainFile;
	bool mCreateTestFile;
	bool mCreateValFile;

	// The TFRecord files
	ofstream mTrainFile;
	ofstream mTestFile;
	ofstream mValidateFile;
};

#endif

#endif
