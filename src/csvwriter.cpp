#include <deque>
#include <map>
#include <vector>
#include <string>
#include <fstream>
#include <stdio.h>
#include <errno.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

using namespace std;

#include "cvedia.hpp"
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
}

WriterStats CsvWriter::GetStats() {

	return mCsvStats;
}

void CsvWriter::ClearStats() {

	// Clear the stats structure
	mCsvStats = {};
}

int CsvWriter::Initialize() {

	string base_path = mBaseDir;

	if (base_path == "")
		base_path = "./";
	else
		base_path += "/";

	base_path += mExportName + "/";

	int dir_err = mkdir(base_path.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
	if (dir_err == -1 && errno != EEXIST) {
		WriteErrorLog(string("Could not create directory: " + base_path).c_str());
		return -1;
	}

	if (mCreateTrainFile) {
		mTrainFile.open(base_path + "train.txt", ios::out | ios::trunc);
		if (!mTrainFile.is_open()) {
			WriteErrorLog(string("Failed to open: " + base_path + "train.txt").c_str());
			return -1;			
		}
	}
	if (mCreateTestFile) {
		mTestFile.open(base_path + "test.txt", ios::out | ios::trunc);
		if (!mTestFile.is_open()) {
			WriteErrorLog(string("Failed to open: " + base_path + "test.txt").c_str());
			return -1;			
		}
	}
	if (mCreateValFile) {
		mValidateFile.open(base_path + "validate.txt", ios::out | ios::trunc);
		if (!mValidateFile.is_open()) {
			WriteErrorLog(string("Failed to open: " + base_path + "validate.txt").c_str());
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

bool CsvWriter::ValidateRequest(WriteRequest* req) {

	return true;
}

int CsvWriter::OpenFile(string file) {

	return 0;
}

int CsvWriter::WriteData(WriteRequest* req) {

	if (!mInitialized) {
		WriteErrorLog("Must call Initialize() first");
		return -1;
	}

	if (!ValidateRequest(req)) {
		return -1;
	}

	string output_line = mFileFormat;
	output_line = ReplaceString(output_line, "$FILENAME", req->filename);
	output_line = ReplaceString(output_line, "$CATEGORY", req->category);

	if (req->type == DATA_TRAIN) {
		if (!mCreateTrainFile) {
			WriteErrorLog("Training file not specified but data contains DATA_TRAIN");			
			return -1;
		}

		mTrainFile << output_line;
	} else if (req->type == DATA_TEST) {
		if (!mCreateTestFile) {
			WriteErrorLog("Test file not specified but data contains DATA_TEST");			
			return -1;
		}

		mTestFile << output_line;
	} else if (req->type == DATA_VALIDATE) {
		if (!mCreateValFile) {
			WriteErrorLog("Validate file not specified but data contains DATA_VALIDATE");			
			return -1;
		}

		mValidateFile << output_line;
	} else {
		WriteErrorLog(string("API returned unsupported file type: " + to_string(req->type)).c_str());			
		return -1;
	}

	return 0;
}

string CsvWriter::ReplaceString(string subject, const string& search, const string& replace) {
	
	size_t pos = 0;

	while ((pos = subject.find(search, pos)) != string::npos) {
	     subject.replace(pos, search.length(), replace);
	     pos += replace.length();
	}

	return subject;
}