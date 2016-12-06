#include <deque>
#include <map>
#include <vector>
#include <string>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdio.h>
#include <errno.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

using namespace std;

#include "config.hpp"

#ifdef HAVE_TFRECORDS

#include "example.pb.h"
#include "cvedia.hpp"
#include "api.hpp"
#include "tfrecordswriter.hpp"

TfRecordsWriter::TfRecordsWriter(string export_name, map<string, string> options) {

	WriteDebugLog("Initializing TfRecordsWriter");

	mModuleOptions = options;
	mExportName = export_name;
}

TfRecordsWriter::~TfRecordsWriter() {

	if (mTrainFile.is_open()) {
		mTrainFile.close();
	}
	if (mTestFile.is_open()) {
		mTestFile.close();
	}
	if (mValidateFile.is_open()) {
		mValidateFile.close();
	}
}

WriterStats TfRecordsWriter::GetStats() {

	return mCsvStats;
}

void TfRecordsWriter::ClearStats() {

	// Clear the stats structure
	mCsvStats = {};
}

int TfRecordsWriter::Initialize() {

	// Clear the stats structure
	mCsvStats = {};

	mCreateTrainFile 	= false;
	mCreateTestFile 	= false;
	mCreateValFile 		= false;

	// Read all passed options
	if (mModuleOptions["create_train_file"] == "1")
		mCreateTrainFile = true;
	if (mModuleOptions["create_test_file"] == "1")
		mCreateTestFile = true;
	if (mModuleOptions["create_validate_file"] == "1")
		mCreateValFile = true;

	if (mModuleOptions["base_dir"] != "") {
		mBaseDir = mModuleOptions["base_dir"];
	} else {
		mBaseDir = "";
	}

	mBasePath = mBaseDir;

	if (mBasePath == "")
		mBasePath = "./";
	else
		mBasePath += "/";

	mBasePath += mExportName + "/";

	int dir_err = mkdir(mBasePath.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
	if (dir_err == -1 && errno != EEXIST) {
		WriteErrorLog(string("Could not create directory: " + mBasePath).c_str());
		return -1;
	}

	return 0;
}

bool TfRecordsWriter::ValidateData(vector<Metadata* > meta) {



	return true;
}


string TfRecordsWriter::PrepareData(Metadata* meta) {

	string file_format = "$FILENAME $CATEGORY\n";

	string output_line = file_format;

	return output_line;
}

int TfRecordsWriter::Finalize() {

	return 0;
}

int TfRecordsWriter::WriteData(Metadata* meta) {

	if (!mInitialized) {
		WriteErrorLog("Must call Initialize() first");
		return -1;
	}

	MetadataEntry* source = NULL;
	MetadataEntry* ground = NULL;

	for (MetadataEntry* e : meta->entries) {
		if (e->meta_type == METADATA_TYPE_SOURCE)
			source = e;		
		if (e->meta_type == METADATA_TYPE_GROUND)
			ground = e;
	}

	string file_uri = WriteImageData(source->filename, source->image_data);
	source->file_uri = file_uri;


	return 0;
}

string TfRecordsWriter::WriteImageData(string filename, vector<uint8_t> image_data) {

	string path = mBasePath + "data/";

	int dir_err = mkdir(path.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
	if (dir_err == -1 && errno != EEXIST) {
		WriteErrorLog(string("Could not create directory: " + path).c_str());
		return "";
	}

	// Must be at least 7 chars long for directory splitting (3 chars + 4 for extension)
	if (filename.size() > 7 && mModuleOptions.count("csv-same-dir") == 0) {

		for (int i = 0; i < 3; ++i) {
			path += filename.substr(i,1) + "/";

			int dir_err = mkdir(path.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
			if (dir_err == -1 && errno != EEXIST) {
				WriteErrorLog(string("Could not create directory: " + path).c_str());
				return "";
			}
		}
	}

	ofstream image_file;
	image_file.open(path + filename, ios::out | ios::trunc | ios::binary);
	image_file.write((char *)&image_data[0], image_data.size());
	image_file.close();		

	mImagesWritten++;
	
	return path + filename;
}

#endif
