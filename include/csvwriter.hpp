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

	string WriteData(Metadata* meta);
	string WriteImageData(string filename, vector<uint8_t> image_data);

	WriterStats GetStats();
	void ClearStats();
	
	virtual int BeginWriting(DatasetMetadata* dataset_meta);
	virtual int EndWriting(DatasetMetadata* dataset_meta);
	virtual int Initialize(DatasetMetadata* dataset_meta);
	virtual string VerifyData(string file_name, DatasetMetadata* dataset_meta);
	virtual int Finalize();

	string mBasePath;
	string mBaseDir;
	string mExportName;

	int mImagesWritten = 0;

	map<string, string> mModuleOptions;

private:

	virtual bool ValidateData(vector<Metadata* > meta);

	WriterStats mCsvStats;
	DatasetMetadata* mDataset;

	bool mInitialized;

	string mVerifyFilename;
	ifstream mVerifyFile;

	map<string,ofstream> mSetFile;
};

#endif
