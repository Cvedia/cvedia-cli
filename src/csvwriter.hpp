#ifndef _CSVWRITER_HPP
#define _CSVWRITER_HPP

using namespace std;

#include "datawriter.hpp"
#include "api.hpp"

class CsvWriter: public IDataWriter {

public:
	CsvWriter(string export_name, map<string, string> options);
	~CsvWriter();

	int WriteData(Metadata* meta);
	string WriteImageData(string filename, vector<uint8_t> image_data);

	WriterStats GetStats();
	void ClearStats();
	
private:

	virtual bool ValidateData(Metadata* meta);
	virtual string PrepareData(Metadata* meta);

	int OpenFile(string file);
	int Initialize();

	WriterStats mCsvStats;

	string mBaseDir;
	string mExportName;

	bool mInitialized;

	bool mCreateTrainFile;
	bool mCreateTestFile;
	bool mCreateValFile;

	string mFileFormat;
	string mBasePath;

	ofstream mTrainFile;
	ofstream mTestFile;
	ofstream mValidateFile;
};

#endif
