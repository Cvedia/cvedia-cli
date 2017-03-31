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
#include "caffeimagedata.hpp"

CaffeImageDataWriter::CaffeImageDataWriter(string export_name, map<string, string> options) {

	LOG(INFO) << "Initializing CaffeImageDataWriter";

	mModuleOptions = options;
	mExportName = export_name;

	if (mModuleOptions["base_dir"] != "") {
		mBaseDir = mModuleOptions["base_dir"];
	} else {
		mBaseDir = "";
	}

	mLastCategoryId = 0;

	mBasePath = mBaseDir;

	if (mBasePath == "")
		mBasePath = "./";
	else
		mBasePath += "/";

	mBasePath += mExportName + "/";

}

CaffeImageDataWriter::~CaffeImageDataWriter() {

}

WriterStats CaffeImageDataWriter::GetStats() {

	return mCsvStats;
}

void CaffeImageDataWriter::ClearStats() {

	// Clear the stats structure
	mCsvStats = {};
}

bool CaffeImageDataWriter::CanHandle(string support) {

	if (support == "resume")
		return true;
	if (support == "integrity")
		return true;
	if (support == "blobs")
		return false;

	return false;
}

int CaffeImageDataWriter::Initialize(DatasetMetadata* dataset_meta, int mode) {

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

string CaffeImageDataWriter::CheckIntegrity(string file_name) {

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

int CaffeImageDataWriter::BeginWriting() {
	
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

string CaffeImageDataWriter::WriteData(Metadata* meta) {

	map<int,string> output_values;
	string output_line;

	MetadataEntry* source = NULL;
	MetadataEntry* ground = NULL;

	// Find the source and ground entries in the list
	for (MetadataEntry *entry : meta->entries) {

		if (entry->id == 0)
			source = entry;
		else if (entry->id == 1)
			ground = entry;
	}

	if (source != NULL && (source->value_type == METADATA_VALUE_TYPE_IMAGE || source->value_type == METADATA_VALUE_TYPE_RAW)) {
		output_values[source->id] = source->file_uri;
	} else {
		LOG(ERROR) << "CaffeImageDataWriter::WriteData() First field needs to be of type Image or Raw";
		return "";
	}

	if (ground != NULL && ground->value_type == METADATA_VALUE_TYPE_STRING) {

		int category_id = 0;
		// Convert the category to an integer in the lookup table.
		if (mCategoryLookup.count(ground->string_value) == 1) {
			category_id = mCategoryLookup[ground->string_value];
		} else {
			// Doesnt exist yet, create a new entry
			category_id = mLastCategoryId;
			mCategoryLookup[ground->string_value] = category_id;
			mLastCategoryId++;			
		}

		output_values[ground->id] = to_string(category_id);

	} else if (ground != NULL && ground->value_type == METADATA_VALUE_TYPE_NUMERIC) {

		if (ground->dtype == "float") {
			output_values[ground->id] += to_string(ground->float_value);
		} else if (ground->dtype == "int") {
			output_values[ground->id] += to_string(ground->int_value);
		}
	} else {
		LOG(ERROR) << "CaffeImageDataWriter::WriteData() Second field needs to be String or Numeric";		
		return "";
	}

	// Output all fields in the correct order
	for (DatasetMapping* mapping : mDataset->mapping_by_id)	{

		output_line += (output_line != "" ? " " : "") + output_values[mapping->id];
	}

	mSetFile[meta->setname] << output_line << endl;

	return "file=" + meta->setname + ".txt;hash=" + md5(output_line);
}

int CaffeImageDataWriter::EndWriting() {
	
	// Close all open files
	for (DatasetSet s : mDataset->sets) {
		mSetFile[s.set_name].close();
	}

	return 0;
}

int CaffeImageDataWriter::Finalize() {

	return 0;
}
