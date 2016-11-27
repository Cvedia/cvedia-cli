#ifndef _HDF5WRITER_HPP
#define _HDF5WRITER_HPP

using namespace std;

#include "H5Cpp.h"
#include "datawriter.hpp"
#include "api.hpp"

using namespace H5;


class Hdf5Writer: public IDataWriter {

public:
	Hdf5Writer(string export_name, map<string, string> options);
	Hdf5Writer() {};
	~Hdf5Writer();

	int WriteData(Metadata* meta);
	string WriteImageData(string filename, vector<uint8_t> image_data);

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

	WriterStats mCsvStats;

	bool mInitialized;

	bool mCreateTrainFile;
	bool mCreateTestFile;
	bool mCreateValFile;

	// The txt indices for the H5 files
	ofstream mTrainFile;
	ofstream mTestFile;
	ofstream mValidateFile;

	// The actual H5 files
	H5File *mTrainH5;
	H5File *mTestH5;
	H5File *mValidateH5;
};

#endif
