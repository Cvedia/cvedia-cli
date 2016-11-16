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

#include "cvedia.hpp"
#include "api.hpp"
#include "csvwriter.hpp"

CsvWriter::CsvWriter(string export_name, map<string, string> options) {

	WriteDebugLog("Initializing CsvWriter");

	// Clear the stats structure
	mCsvStats = {};

	mCreateTrainFile 	= false;
	mCreateTestFile 	= false;
	mCreateValFile 		= false;

	// Read all passed options
	if (options["create_train_file"] == "1")
		mCreateTrainFile = true;
	if (options["create_test_file"] == "1")
		mCreateTestFile = true;
	if (options["create_validate_file"] == "1")
		mCreateValFile = true;

	if (options["base_dir"] != "") {
		mBaseDir = options["base_dir"];
	} else {
		mBaseDir = "";
	}

	mExportName = export_name;

	mInitialized = false;
}

CsvWriter::~CsvWriter() {

	if (mTrainFile.is_open())
		mTrainFile.close();
	if (mTestFile.is_open())
		mTestFile.close();
	if (mValidateFile.is_open())
		mValidateFile.close();
}

WriterStats CsvWriter::GetStats() {

	return mCsvStats;
}

void CsvWriter::ClearStats() {

	// Clear the stats structure
	mCsvStats = {};
}

int CsvWriter::Initialize() {

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

	if (mCreateTrainFile) {
		mTrainFile.open(mBasePath + "train.txt", ios::out | ios::trunc);
		if (!mTrainFile.is_open()) {
			WriteErrorLog(string("Failed to open: " + mBasePath + "train.txt").c_str());
			return -1;			
		}
	}
	if (mCreateTestFile) {
		mTestFile.open(mBasePath + "test.txt", ios::out | ios::trunc);
		if (!mTestFile.is_open()) {
			WriteErrorLog(string("Failed to open: " + mBasePath + "test.txt").c_str());
			return -1;			
		}
	}
	if (mCreateValFile) {
		mValidateFile.open(mBasePath + "validate.txt", ios::out | ios::trunc);
		if (!mValidateFile.is_open()) {
			WriteErrorLog(string("Failed to open: " + mBasePath + "validate.txt").c_str());
			return -1;			
		}
	}

	if (!mCreateTrainFile && !mCreateTestFile && !mCreateValFile) {
		WriteErrorLog("Must enable at least 1 of : create_train_file, create_test_file, create_validate_file");
		return -1;
	}

	mFileFormat = "$FILENAME $CATEGORY\n";

	mInitialized = true;

	return 0;
}

bool CsvWriter::ValidateData(Metadata* meta) {

	return true;
}

int CsvWriter::OpenFile(string file) {

	return 0;
}

int CsvWriter::WriteData(Metadata* meta) {

	if (!mInitialized) {
		WriteErrorLog("Must call Initialize() first");
		return -1;
	}

	if (!ValidateData(meta)) {
		return -1;
	}

	WriteImageData(meta->filename, meta->image_data);

	string output_line = mFileFormat;
	output_line = ReplaceString(output_line, "$FILENAME", meta->filename);
//	output_line = ReplaceString(output_line, "$CATEGORY", meta->category);

	if (meta->type == DATA_TRAIN) {
		if (!mCreateTrainFile) {
			WriteErrorLog("Training file not specified but data contains DATA_TRAIN");			
			return -1;
		}

		mTrainFile << output_line;
	} else if (meta->type == DATA_TEST) {
		if (!mCreateTestFile) {
			WriteErrorLog("Test file not specified but data contains DATA_TEST");			
			return -1;
		}

		mTestFile << output_line;
	} else if (meta->type == DATA_VALIDATE) {
		if (!mCreateValFile) {
			WriteErrorLog("Validate file not specified but data contains DATA_VALIDATE");			
			return -1;
		}

		mValidateFile << output_line;
	} else {
		WriteErrorLog(string("API returned unsupported file type: " + to_string(meta->type)).c_str());			
		return -1;
	}

	return 0;
}

int CsvWriter::WriteImageData(string filename, vector<uint8_t> image_data) {

	string path = mBasePath;

	// Must be at least 7 chars long for directory splitting (3 chars + 4 for extension)
	if (filename.size() > 7) {

		for (int i = 0; i < 3; ++i) {
			path += filename.substr(i,1) + "/";

			int dir_err = mkdir(path.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
			if (dir_err == -1 && errno != EEXIST) {
				WriteErrorLog(string("Could not create directory: " + path).c_str());
				return -1;
			}
		}

		ofstream image_file;
		image_file.open(path + filename, ios::out | ios::trunc | ios::binary);
		image_file.write((char *)&image_data[0], image_data.size());
		image_file.close();
	} else {
		path += "data/";

		int dir_err = mkdir(path.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
		if (dir_err == -1 && errno != EEXIST) {
			WriteErrorLog(string("Could not create directory: " + path).c_str());
			return -1;
		}

		// Store directly inside a data folder
		ofstream image_file;
		image_file.open(path + filename, ios::out | ios::trunc | ios::binary);
		image_file.write((char *)&image_data[0], image_data.size());
		image_file.close();		
	}

	return 0;
}