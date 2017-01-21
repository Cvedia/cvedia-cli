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

#include "md5.hpp"
#include "easylogging++.h"
#include "config.hpp"
#include "cvedia.hpp"
#include "api.hpp"
#include "csvwriter.hpp"

CsvWriter::CsvWriter(string export_name, map<string, string> options) {

	LOG(INFO) << "Initializing CsvWriter";

	mModuleOptions = options;
	mExportName = export_name;

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

bool CsvWriter::CanHandle(string support) {

	if (support == "resume")
		return true;
	if (support == "integrity")
		return true;
	if (support == "blobs")
		return true;

	return false;
}

int CsvWriter::Initialize(DatasetMetadata* dataset_meta, int mode) {

	mDataset = dataset_meta;

	// Clear the stats structure
	mCsvStats = {};

	if (mode == MODE_NEW) {

		int dir_err = mkdir(mBasePath.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
		if (dir_err == -1 && errno != EEXIST) {
			LOG(ERROR) << "Could not create directory: " << mBasePath;
			return -1;
		}

		// Remove the existing outputs
		for (DatasetSet s : dataset_meta->sets) {
			remove((mBasePath + s.set_name + ".txt").c_str());
		}
	}

	mInitialized = true;

	return 0;
}

string CsvWriter::CheckIntegrity(string file_name) {

	if (mVerifyFilename != file_name) {
		if (mVerifyFile.is_open())
			mVerifyFile.close();

		mVerifyFilename = file_name;
		mVerifyFile.open(mBasePath + file_name);
	}

	string dataline;

	if (!getline(mVerifyFile, dataline)) {
		return "result=eof";
	} else {

		return "hash=" + md5(dataline);
	}
}

int CsvWriter::BeginWriting() {
	
	// Open all output sets on disk in append mode
	for (DatasetSet s : mDataset->sets) {
		mSetFile[s.set_name].open(mBasePath + s.set_name + ".txt", ios::out | ios::app);
		if (!mSetFile[s.set_name].is_open()) {
			LOG(ERROR) << "Failed to open: " << mBasePath << s.set_name << ".txt";
			return -1;			
		}		
	}

	return 0;
}

string CsvWriter::WriteData(Metadata* meta) {

	map<int,string> output_values;
	string output_line;

	for (MetadataEntry* e : meta->entries) {

		if (e->value_type == METADATA_VALUE_TYPE_IMAGE || e->value_type == METADATA_VALUE_TYPE_RAW) {
			output_values[e->id] = e->file_uri;
		} else if (e->value_type == METADATA_VALUE_TYPE_STRING) {

			string tmp_line;

			for (string val: e->string_value) {

				tmp_line += (tmp_line == "" ? "\""  : ",") + val;
			}

			output_values[e->id] = tmp_line + "\"";
		}
	}

	// Output all fields in the correct order
	for (DatasetMapping* mapping : mDataset->mapping_by_id)	{

		output_line += (output_line != "" ? " " : "") + output_values[mapping->id];
	}

	mSetFile[meta->setname] << output_line << endl;

	return "file=" + meta->setname + ".txt;hash=" + md5(output_line);
}

int CsvWriter::EndWriting() {
	
	// Close all open files
	for (DatasetSet s : mDataset->sets) {
		mSetFile[s.set_name].close();
	}

	return 0;
}

int CsvWriter::Finalize() {

	return 0;
}

