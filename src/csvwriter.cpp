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

int CsvWriter::Initialize(DatasetMetadata* dataset_meta, bool resume) {

	mDataset = dataset_meta;

	// Clear the stats structure
	mCsvStats = {};

	int dir_err = mkdir(mBasePath.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
	if (dir_err == -1 && errno != EEXIST) {
		LOG(ERROR) << "Could not create directory: " << mBasePath;
		return -1;
	}

	// Remove the existing outputs
	if (!resume) {
		for (DatasetSet s : dataset_meta->sets) {
			remove((mBasePath + s.set_name + ".txt").c_str());
		}
	}

	mInitialized = true;

	return 0;
}

int CsvWriter::BeginWriting(DatasetMetadata* dataset_meta) {
	
	// Open all output sets on disk in append mode
	for (DatasetSet s : dataset_meta->sets) {
		mSetFile[s.set_name].open(mBasePath + s.set_name + ".txt", ios::out | ios::app);
		if (!mSetFile[s.set_name].is_open()) {
			LOG(ERROR) << "Failed to open: " << mBasePath << s.set_name << ".txt";
			return -1;			
		}		
	}

	return 0;
}

int CsvWriter::EndWriting(DatasetMetadata* dataset_meta) {
	
	// Close all open files
	for (DatasetSet s : dataset_meta->sets) {
		mSetFile[s.set_name].close();
	}

	return 0;
}

bool CsvWriter::ValidateData(vector<Metadata* > meta) {

	return true;
}

int CsvWriter::Finalize() {

	return 0;
}

string CsvWriter::VerifyData(string file_name, DatasetMetadata* dataset_meta) {

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

string CsvWriter::WriteData(Metadata* meta) {

	map<int,string> output_values;

	if (!mInitialized) {
		LOG(ERROR) << "Must call Initialize() first";
		return "";
	}

	if (meta == NULL || meta->entries.size() == 0) {
		LOG(ERROR) << "No work provided for CsvWriter::WriteData";
		return "";
	}

	// Convert all images to paths
	for (MetadataEntry* e : meta->entries) {

		if (e->value_type == METADATA_VALUE_TYPE_IMAGE || e->value_type == METADATA_VALUE_TYPE_RAW) {

			string file_uri = "";
			if (e->value_type == METADATA_VALUE_TYPE_IMAGE)
				file_uri = WriteImageData(e->filename, &e->image_data[0], e->image_data.size());
			else if (e->value_type == METADATA_VALUE_TYPE_RAW && e->dtype == "float")
				file_uri = WriteImageData(e->filename, (uint8_t*)&e->float_raw_data[0], e->float_raw_data.size() * 4);
			else if (e->value_type == METADATA_VALUE_TYPE_RAW && e->dtype == "uint8")
				file_uri = WriteImageData(e->filename, &e->uint8_raw_data[0], e->uint8_raw_data.size());
			
			// We were unable to write the image to disk, fail this record
			if (file_uri == "") {
				return "";
			}
			e->file_uri = file_uri;
		}

		if (e->value_type == METADATA_VALUE_TYPE_IMAGE || e->value_type == METADATA_VALUE_TYPE_RAW) {
			output_values[e->id] = e->file_uri;
		} else if (e->value_type == METADATA_VALUE_TYPE_STRING) {

			string tmp_line;

			for (string val: e->string_value) {

				if (tmp_line == "")
					tmp_line = "\"";
				else
					tmp_line += ",";

				tmp_line += val;
			}

			tmp_line += "\"";

			output_values[e->id] = tmp_line;
		}

	}

	string output_line;

	// Output all fields in the correct order
	for (DatasetMapping* mapping : mDataset->mapping_by_id)	{

		if (output_line != "")
			output_line += " ";

		output_line += output_values[mapping->id];
	}

	mSetFile[meta->type] << output_line << endl;

	return "file=" + meta->type + ".txt;hash=" + md5(output_line);
}

string CsvWriter::WriteImageData(string filename, uint8_t* image_data, unsigned int len) {

	string path = mBasePath + "data/";

	int dir_err = mkdir(path.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
	if (dir_err == -1 && errno != EEXIST) {
		LOG(ERROR) << "Could not create directory: " << path;
		return "";
	}

	// Must be at least 7 chars long for directory splitting (3 chars + 4 for extension)
	if (filename.size() > 7 && mModuleOptions.count("images-same-dir") == 0) {

		for (int i = 0; i < 3; ++i) {
			path += filename.substr(i,1) + "/";

			int dir_err = mkdir(path.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
			if (dir_err == -1 && errno != EEXIST) {
				LOG(ERROR) << "Could not create directory: " << path;
				return "";
			}
		}
	}

	// extract the base from the filename
	string base_part = filename.substr(0,filename.find_first_of("."));
	
	MD5 c_md5 = MD5();
	c_md5.update(image_data, len);
	c_md5.finalize();
	string digest = c_md5.hexdigest();

	if (base_part != digest) {
		LOG(ERROR) << "Filename hash does not match contents for " << filename;
		return "";
	}

	ofstream image_file;
	image_file.open(path + filename, ios::out | ios::trunc | ios::binary);
	image_file.write((char *)image_data, len);
	image_file.close();		

	mImagesWritten++;
	
	return path + filename;
}
