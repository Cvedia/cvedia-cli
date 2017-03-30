#ifndef _CAFFEIMAGEDATA_HPP
#define _CAFFEIMAGEDATA_HPP

using namespace std;

#include "datawriter.hpp"
#include "api.hpp"

class CaffeImageDataWriter: public IDataWriter {

public:
	CaffeImageDataWriter(string export_name, map<string, string> options);
	CaffeImageDataWriter() {};
	~CaffeImageDataWriter();

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

	WriterStats mCsvStats;
	DatasetMetadata* mDataset;

	bool mInitialized;

	string mVerifyFilename;
	ifstream mVerifyFile;

	map<string,ofstream> mSetFile;
	map<string, int> mCategoryLookup;
	int mLastCategoryId = {};
};

#endif
