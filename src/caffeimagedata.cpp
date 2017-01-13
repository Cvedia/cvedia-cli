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

#include "easylogging++.h"
#include "config.hpp"
#include "cvedia.hpp"
#include "api.hpp"
#include "csvwriter.hpp"
#include "caffeimagedata.hpp"

CaffeImageDataWriter::CaffeImageDataWriter(string export_name, map<string, string> options) {

	LOG(INFO) << "Initializing CaffeImageDataWriter";

	mModuleOptions = options;
	mExportName = export_name;
}

bool CaffeImageDataWriter::ValidateData(vector<Metadata* > meta) {

	return true;
}

int CaffeImageDataWriter::Finalize() {

	// Sync out lookup table to disk

	string path = mBasePath + "categories.txt";

	ofstream category_file;
	category_file.open(path, ios::out | ios::trunc);

	for (auto& kv : mCategoryLookup) {

		category_file << "\"" << kv.first << "\" " << kv.second << endl;
	}

	category_file.close();

	return 0;
}

string CaffeImageDataWriter::PrepareData(Metadata* meta) {

	MetadataEntry* source = NULL;
	MetadataEntry* ground = NULL;

	for (MetadataEntry* e : meta->entries) {
		if (e->meta_type == METADATA_TYPE_SOURCE)
			source = e;		
		if (e->meta_type == METADATA_TYPE_GROUND)
			ground = e;
	}

	string file_format = "$FILENAME $CATEGORYID\n";

	string output_line = file_format;
	output_line = ReplaceString(output_line, "$FILENAME", source->file_uri);

	if (ground->meta_fields["category"].size() > 0) {

		// Save a current copy of the line
		string tmp_line = "";

		for (string cat: ground->meta_fields["category"]) {

			int category_id = 0;

			// Convert the category to an integer in the lookup table.
			if (mCategoryLookup.count(cat) == 1) {
				category_id = mCategoryLookup[cat];
			} else {
				// Doesnt exist yet, create a new entry
				category_id = mLastCategoryId;
				mCategoryLookup[cat] = category_id;
				mLastCategoryId++;			
			}

			tmp_line += ReplaceString(output_line, "$CATEGORYID", to_string(category_id));
		}

		output_line = tmp_line;
	} else {
		LOG(ERROR) << "CaffeImageDataWriter::PrepareData() Missing 'category' field in input";
		return "";
	}

	return output_line;
}

