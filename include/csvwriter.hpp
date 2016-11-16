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

	int WriteData(Metadata* meta);
	string WriteImageData(string filename, vector<uint8_t> image_data);

	WriterStats GetStats();
	void ClearStats();
	
	virtual int Initialize();
	virtual int Finalize();

	string mBasePath;
	string mBaseDir;
	string mExportName;

	map<string, string> mModuleOptions;

private:

	virtual bool ValidateData(vector<Metadata* > meta);
	virtual string PrepareData(Metadata* meta);

	WriterStats mCsvStats;

	bool mInitialized;

	bool mCreateTrainFile;
	bool mCreateTestFile;
	bool mCreateValFile;

	ofstream mTrainFile;
	ofstream mTestFile;
	ofstream mValidateFile;
};

#endif
