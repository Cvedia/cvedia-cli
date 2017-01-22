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
	
	virtual bool CanHandle(string support);

	virtual int Initialize(DatasetMetadata* dataset_meta, int mode);

	virtual int BeginWriting();
	string WriteData(Metadata* meta);
	virtual int EndWriting();

	virtual string CheckIntegrity(string file_name);

	virtual int Finalize();

	string mBasePath;
	string mBaseDir;
	string mExportName;

	int mImagesWritten = 0;

	map<string, string> mModuleOptions;

private:

	PyGILState_STATE gstate;

	PyObject* pInitFn;
	PyObject* pCanHandleFn;
	PyObject* pBeginWritingFn;
	PyObject* pWriteFn;
	PyObject* pEndWritingFn;
	PyObject* pCheckIntegrityFn;
	PyObject* pFinalFn;

	virtual string PrepareData(Metadata* meta);
	string WriteImageData(string filename, vector<uint8_t> image_data);
	void AddToDict(PyObject* dict, PyObject* key, PyObject* val);

	WriterStats mCsvStats;

	bool mInitialized;
};

#endif

#endif
