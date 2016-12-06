#ifndef _TFRECORDSWRITER_HPP
#define _TFRECORDSWRITER_HPP

using namespace std;

#include "config.hpp"

#ifdef HAVE_TFRECORDS

#include "datawriter.hpp"
#include "api.hpp"

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
	string WriteImageData(string filename, vector<uint8_t> image_data);

	WriterStats mCsvStats;

	bool mInitialized;

	bool mCreateTrainFile;
	bool mCreateTestFile;
	bool mCreateValFile;

	// The TFRecord files
	ofstream mTrainFile;
	ofstream mTestFile;
	ofstream mValidateFile;
};

#endif

#endif
