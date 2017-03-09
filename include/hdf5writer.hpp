#ifndef _HDF5WRITER_HPP
#define _HDF5WRITER_HPP

using namespace std;

#include "config.hpp"

#ifdef HAVE_HDF5

#include "H5Cpp.h"
#include "datawriter.hpp"
#include "api.hpp"

using namespace H5;


class Hdf5Writer: public IDataWriter {

public:
	Hdf5Writer(string export_name, map<string, string> options);
	Hdf5Writer() {};
	~Hdf5Writer();

	WriterStats GetStats();
	void ClearStats();

	virtual bool CanHandle(string support);

	virtual int Initialize(DatasetMetadata* dataset_meta, int mode);

	virtual int BeginWriting();
	virtual string WriteData(Metadata* meta);
	virtual int EndWriting();

	virtual string CheckIntegrity(string file_name);

	virtual int Finalize();

	string mBasePath;
	string mBaseDir;
	string mExportName;

	int mImagesWritten = 0;

	map<string, string> mModuleOptions;

private:
	int writeCount = 1;
	virtual string PrepareData(Metadata* meta);
	virtual string GenerateHash(Metadata* meta);

	const DataType& ConvertDtype(string dtype);
	void AppendEntry(DataSet* dataset, void* data_ptr, hsize_t data_size, const DataType& dtype);

	WriterStats mCsvStats;

	bool mInitialized;

	bool mCreateTrainFile;
	bool mCreateTestFile;
	bool mCreateValFile;

	DataSpace *mSpace;

	map<string, DataSet* > mDataSet;
	DatasetMetadata* mDatasetMetadata;

	hsize_t dataDim = 0;

	// The txt indices for the H5 files
	ofstream mTrainFile;
	ofstream mTestFile;
	ofstream mValidateFile;
	map<string, ofstream > mSetFile;

	string mVerifyFilename;
	ifstream mVerifyFile;

	// The actual H5 files
	map<string, H5File* > mH5File;
};

#endif	// HAVE_HDF5

#endif