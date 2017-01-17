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

	string WriteData(Metadata* meta);

	WriterStats GetStats();
	void ClearStats();
	
	virtual string VerifyData(string file_name, DatasetMetadata* dataset_meta);
	
	virtual int Initialize(DatasetMetadata* dataset_meta);
	virtual int Finalize();

	string mBasePath;
	string mBaseDir;
	string mExportName;

	int mImagesWritten = 0;

	map<string, string> mModuleOptions;

private:

	virtual bool ValidateData(vector<Metadata* > meta);
	virtual string PrepareData(Metadata* meta);

	const DataType& ConvertDtype(string dtype);
	void AppendEntry(DataSet* dataset, void* data_ptr, hsize_t data_size, const DataType& dtype);

	WriterStats mCsvStats;

	bool mInitialized;

	bool mCreateTrainFile;
	bool mCreateTestFile;
	bool mCreateValFile;

	DataSpace *mSourceSpace;
	DataSpace *mGroundSpace;

	map<string, DataSet* > mDataSet;
	map<string, DataSet* > mLabelSet;

	hsize_t mSourceDataDim = 0;
	hsize_t mGroundDataDim = 0;

	// The txt indices for the H5 files
	ofstream mTrainFile;
	ofstream mTestFile;
	ofstream mValidateFile;

	// The actual H5 files
	map<string, H5File* > mH5File;
};

#endif	// HAVE_HDF5

#endif