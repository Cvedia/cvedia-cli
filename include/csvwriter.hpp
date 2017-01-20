#ifndef _CSVWRITER_HPP
#define _CSVWRITER_HPP

using namespace std;

#include "datawriter.hpp"
#include "api.hpp"

class CsvWriter: public IDataWriter {

public:
	CsvWriter(string export_name, map<string, string> options);
	CsvWriter() {};
	~CsvWriter();

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
};

#endif
