#ifndef _TFRECORDSWRITER_HPP
#define _TFRECORDSWRITER_HPP

using namespace std;

#ifdef HAVE_TFRECORDS

#include "H5Cpp.hpp"
#include "datawriter.hpp"
#include "api.hpp"

using namespace H5;

class TfRecordsWriter: public IDataWriter {

public:
	TfRecordsWriter(string export_name, map<string, string> options);
	TfRecordsWriter() {};
	~TfRecordsWriter();

	int WriteData(Metadata* meta);

	WriterStats GetStats();
	void ClearStats();
	
	virtual int Initialize();
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

	DataSet *mDataSet[3];
	DataSet *mLabelSet[3];

	hsize_t mSourceDataDim = 0;
	hsize_t mGroundDataDim = 0;

	// The txt indices for the H5 files
	ofstream mTrainFile;
	ofstream mTestFile;
	ofstream mValidateFile;

	// The actual H5 files
	H5File *mH5File[3] = {};
};

#endif

#endif
