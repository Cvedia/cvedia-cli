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
#include "hdf5writer.hpp"
#include "H5Cpp.h"

using namespace H5;

Hdf5Writer::Hdf5Writer(string export_name, map<string, string> options) {

	WriteDebugLog("Initializing Hdf5Writer");

	H5::Exception::dontPrint();

	mModuleOptions = options;
	mExportName = export_name;
}

Hdf5Writer::~Hdf5Writer() {

	if (mTrainFile.is_open()) {
		mTrainFile.close();
		if (mTrainH5 != NULL) {
			mTrainH5->close();
			delete mTrainH5;
		}
	}
	if (mTestFile.is_open()) {
		mTestFile.close();
		if (mTestH5 != NULL) {
			mTestH5->close();
			delete mTestH5;
		}
	}
	if (mValidateFile.is_open()) {
		mValidateFile.close();
		if (mValidateH5 != NULL) {
			mValidateH5->close();
			delete mValidateH5;
		}
	}
}

WriterStats Hdf5Writer::GetStats() {

	return mCsvStats;
}

void Hdf5Writer::ClearStats() {

	// Clear the stats structure
	mCsvStats = {};
}

int Hdf5Writer::Initialize() {

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

	if (mCreateTrainFile) {
		mTrainFile.open(mBasePath + "train.txt", ios::out | ios::trunc);
		if (!mTrainFile.is_open()) {
			WriteErrorLog(string("Failed to open: " + mBasePath + "train.txt").c_str());
			return -1;			
		}

		try {
			mTrainH5 = new H5File(mBasePath + "train.h5", H5F_ACC_TRUNC);
		} catch (H5::Exception e) {
			WriteErrorLog(string("Failed to create: " + mBasePath + "train.h5").c_str());
			WriteErrorLog(e.getCDetailMsg());
			return -1;						
		}
	}

	if (mCreateTestFile) {
		mTestFile.open(mBasePath + "test.txt", ios::out | ios::trunc);
		if (!mTestFile.is_open()) {
			WriteErrorLog(string("Failed to open: " + mBasePath + "test.txt").c_str());
			return -1;			
		}

		try {
			mTestH5 = new H5File(mBasePath + "test.h5", H5F_ACC_TRUNC);
		} catch (H5::Exception e) {
			WriteErrorLog(string("Failed to create: " + mBasePath + "test.h5").c_str());
			WriteErrorLog(e.getCDetailMsg());
			return -1;						
		}
	}

	if (mCreateValFile) {
		mValidateFile.open(mBasePath + "validate.txt", ios::out | ios::trunc);
		if (!mValidateFile.is_open()) {
			WriteErrorLog(string("Failed to open: " + mBasePath + "validate.txt").c_str());
			return -1;			
		}

		try {
			mValidateH5 = new H5File(mBasePath + "validate.h5", H5F_ACC_TRUNC);
		} catch (H5::Exception e) {
			WriteErrorLog(string("Failed to create: " + mBasePath + "validate.h5").c_str());
			WriteErrorLog(e.getCDetailMsg());
			return -1;						
		}
	}

	if (!mCreateTrainFile && !mCreateTestFile && !mCreateValFile) {
		WriteErrorLog("Must enable at least 1 of : create_train_file, create_test_file, create_validate_file");
		return -1;
	}

	mInitialized = true;

	return 0;
}

bool Hdf5Writer::ValidateData(vector<Metadata* > meta) {

	return true;
}

string Hdf5Writer::PrepareData(Metadata* meta) {

	string file_format = "$FILENAME $CATEGORY\n";

	string output_line = file_format;
/*
	output_line = ReplaceString(output_line, "$FILENAME", meta->file_uri);

	if (meta->meta_fields["category"].size() > 0) {

		// Save a current copy of the line
		string tmp_line = "";

		for (string cat: meta->meta_fields["category"]) {

			tmp_line += ReplaceString(output_line, "$CATEGORY", "\"" + cat + "\"");
		}

		output_line = tmp_line;
	} else {
		WriteErrorLog("Hdf5Writer::PrepareData() Missing 'category' field in input");
		return "";
	}
*/
	return output_line;
}

int Hdf5Writer::Finalize() {

	return 0;
}

int Hdf5Writer::WriteData(Metadata* meta) {

	if (!mInitialized) {
		WriteErrorLog("Must call Initialize() first");
		return -1;
	}

//	string file_uri = WriteImageData(meta->filename, meta->image_data);
//	meta->file_uri = file_uri;

	string output_line = PrepareData(meta);
	if (output_line == "")
		return -1;

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

string Hdf5Writer::WriteImageData(string filename, vector<uint8_t> image_data) {

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