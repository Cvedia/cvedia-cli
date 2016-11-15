#ifndef _CSVWRITER_HPP
#define _CSVWRITER_HPP

using namespace std;

#include "datawriter.hpp"

class CsvWriter: public IDataWriter {

public:
	CsvWriter(string export_name, map<string, string> options);
	~CsvWriter();

	int WriteData(WriteRequest* req);

	WriterStats GetStats();
	void ClearStats();
	
private:

	virtual bool ValidateRequest(WriteRequest* req);
	string ReplaceString(string subject, const string& search, const string& replace);


	int OpenFile(string file);
	int Initialize();

	deque<WriteRequest* > mRequests;

	WriterStats mCsvStats;

	string mBaseDir;
	string mExportName;

	bool mInitialized;

	bool mCreateTrainFile;
	bool mCreateTestFile;
	bool mCreateValFile;

	string mFileFormat;

	ofstream mTrainFile;
	ofstream mTestFile;
	ofstream mValidateFile;
};

#endif
