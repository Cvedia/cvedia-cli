#include <cstring>
#include <cstdlib>
#include <deque>
#include <map>
#include <vector>
#include <string>
#include <fstream>
#include <iostream>
#include <sstream>
#include <deque>
#include <mutex>
#include <thread>
#include <condition_variable>
#include <fcntl.h>
#include <stdio.h>
#include <errno.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <assert.h>
#include <math.h>
#include <sys/types.h>
#include <unistd.h>
#include <curl/curl.h>

#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"

#include "globals.hpp"
#include "md5.hpp"
#include "api.hpp"
#include "optionparser.h"
#include "cvedia.hpp"
#include "curlreader.hpp"
#include "csvwriter.hpp"

using namespace std;
using namespace rapidjson;

int InitializeApi() {

	// Execute welcome call to make sure API is valid
	string api_url = gApiUrl + gAPIVersion + "/" + string(API_FUNCTION_HELLO);

	ReadRequest *req = CurlReader::RequestUrl(api_url);

	if (req != NULL && req->read_data.size() > 0) {

		Document d;
		string data_str( req->read_data.begin(), req->read_data.end() );
		d.Parse(data_str.c_str());

		if (d.IsObject()) {

			if (d["version"].IsString() && d["motd"].IsString()) {
				string api_version = d["version"].GetString();
				string motd = d["motd"].GetString();

				delete req;
				return 0;
			}
		}
	}

	WriteDebugLog(string("Failed to initialize Cvedia API (" + api_url + ")").c_str());

	if (req != NULL)
		delete req;

	return -1;
}

int GetTotalDatasetSize(map<string,string> options) {

	int count = 0;

	string api_url = gApiUrl + gAPIVersion + "/" + gJobID + "/" + string(API_FUNCTION_COUNT);

	WriteDebugLog(string("Fetching dataset size at " + api_url).c_str());

	ReadRequest *req = CurlReader::RequestUrl(api_url);

	if (req != NULL && req->read_data.size() > 0) {

		Document d;
		string data_str( req->read_data.begin(), req->read_data.end() );
		d.Parse(data_str.c_str());

		Value& count_val = d["count"];

		count = count_val.GetInt();
	} else {
		WriteDebugLog(string("No data returned by API call").c_str());
	}

	return count;
}

vector<Metadata* > FetchBatch(map<string,string> options, int batch_idx) {

	vector<Metadata* > meta_vector;

	string api_url = gApiUrl + gAPIVersion + "/" + gJobID + "/" + string(API_FUNCTION_FETCH_BATCH);
	api_url = ReplaceString(api_url, "$BATCHSIZE", to_string(gBatchSize));
	api_url = ReplaceString(api_url, "$BATCHID", to_string(batch_idx));

	WriteDebugLog(string("Fetching batch at " + api_url).c_str());

	ReadRequest *req = CurlReader::RequestUrl(api_url);

	if (req != NULL && req->read_data.size() > 0) {

		string data_str( req->read_data.begin(), req->read_data.end() );
		meta_vector = ParseFeed(data_str.c_str());
	}

	return meta_vector;
}

vector<Metadata* > ParseFeed(const char* feed) {

	vector<Metadata* > meta_vector;

	Document api_doc;
	api_doc.Parse(feed);

	// Check if feed is contained in an array
	if (api_doc.IsArray()) {

		int total_entries = api_doc.Size();

		// Loop through all objects in the array
		for (int entry_idx = 0; entry_idx < total_entries; ++entry_idx) {

			const Value& obj = api_doc[entry_idx];

			// Yes we have an object
			if (obj.IsObject()) {

				// Ok, object found in an array. so far so good. Lets start the metadata construction
				Metadata* meta_record = new Metadata;

				// Iterate through all object members
				for (Value::ConstMemberIterator itr = obj.MemberBegin(); itr != obj.MemberEnd(); ++itr) {

					string key = itr->name.GetString();

					// Couple of key members that are required and processed seperately
					if (key == "type") {
						meta_record->type = itr->value.GetString();
					} else if (key == "data") {

						if (itr->value.IsArray()) {

							int data_cnt = itr->value.Size();
							for (int data_idx = 0; data_idx < data_cnt; ++data_idx) {

								const Value &entryObj = itr->value[data_idx];

								meta_record = ParseDataEntry(entryObj, meta_record);
								assert(meta_record != NULL);

							} // End of loop over "data": []
						}

					} else {
						WriteErrorLog(string("Unsupported fields found in body: " + key).c_str());
						goto invalid;
					}
				}

				// Store new Metadata in the output vector
				meta_vector.push_back(meta_record);

			} else {
				goto invalid;
			}
		}

	} else {
		goto invalid;
	}

	return meta_vector;

	invalid:
	WriteDebugLog("ParseFeed() Error parsing JSON data");
	meta_vector.clear();
	return meta_vector;
}

vector<Metadata* > ParseTarFeed(const char* feed) {

	vector<Metadata* > meta_vector;
	map<int, Metadata* > meta_map;	// Used to link the corresponding ground to source through their Id field

	Document api_doc;
	api_doc.Parse(feed);

	// Check if feed is contained in an array
	if (api_doc.IsArray()) {

		int total_entries = api_doc.Size();

		// Loop through all objects in the array
		for (int entry_idx = 0; entry_idx < total_entries; ++entry_idx) {

			const Value& obj = api_doc[entry_idx];

			Metadata* meta_record = new Metadata;
			meta_record = ParseDataEntry(obj, meta_record);

			// We only check the first entry. contains: {"source","ground"} is not allowed for TAR metadata
			MetadataEntry* entry = meta_record->entries[0];

			assert(entry != NULL);

			// Havent seen this Id yet
			if (meta_map.count(entry->id) == 0) {
				meta_map[entry->id] = meta_record;
				// Store new Metadata in the output vector
				meta_vector.push_back(meta_record);
			} else {
				// We already have this Id, delete the new meta_record and add to vector<> only
				meta_map[entry->id]->entries.push_back(entry);
				delete meta_record;
			}
		}

	} else {
		goto invalid;
	}

	return meta_vector;

	invalid:
	WriteDebugLog("ParseTarFeed() Error parsing JSON data");
	meta_vector.clear();
	return meta_vector;
}

Metadata* ParseDataEntry(const Value &entryObj, Metadata* meta_output) {

	MetadataEntry* meta_entry = new MetadataEntry();

	bool contains_source = false;
	bool contains_groundtruth = false;

	// These keys are checked before the rest since they influence the
	// parsing of the data
	if (entryObj["type"].IsString()) {
		meta_entry->value_type = entryObj["type"].GetString();
	} else {									
		WriteErrorLog("Metadata entry does not specify a 'type'");
		goto invalid;
	}

	if (entryObj.HasMember("dtype") && entryObj["dtype"].IsString()) {
		meta_entry->dtype = entryObj["dtype"].GetString();
	}

	// Iterate through all object members
	for (Value::ConstMemberIterator entry_itr = entryObj.MemberBegin(); entry_itr != entryObj.MemberEnd(); ++entry_itr) {

		string key = entry_itr->name.GetString();

		if (key == "filename") {
			meta_entry->filename = entry_itr->value.GetString();
		} else if (key == "url") {
			meta_entry->url = entry_itr->value.GetString();
		} else if (key == "type") {
			// Already set above
		} else if (key == "dtype") {
			meta_entry->dtype = entry_itr->value.GetString();
		} else if (key == "channels") {
			meta_entry->data_channels = entry_itr->value.GetInt();
		} else if (key == "id") {
			meta_entry->id = entry_itr->value.GetInt();
		} else if (key == "width") {
			meta_entry->data_width = entry_itr->value.GetInt();
		} else if (key == "height") {
			meta_entry->data_height = entry_itr->value.GetInt();
		} else if (key == "value") {
			if (meta_entry->value_type == METADATA_VALUE_TYPE_LABEL) {

				if (entry_itr->value.IsArray()) {

					int value_size = entry_itr->value.Size();

					for (int value_idx = 0; value_idx < value_size; ++value_idx) {
						meta_entry->label.push_back(entry_itr->value[value_idx].GetString());
					}
				}
			} else if (meta_entry->value_type == METADATA_VALUE_TYPE_NUMERIC) {
				if (meta_entry->dtype == "") {
					WriteErrorLog(string("Missing 'dtype' for METADATA_VALUE_TYPE_NUMERIC").c_str());
					goto invalid;
				}

				if (meta_entry->dtype == "int") {
					meta_entry->int_value = entry_itr->value.GetInt();
				} else if (meta_entry->dtype == "float") {
					meta_entry->float_value = entry_itr->value.GetFloat();
				} else {
					WriteErrorLog(string("Invalid 'dtype' found for METADATA_VALUE_TYPE_NUMERIC: " + meta_entry->dtype).c_str());
					goto invalid;												
				}

			} else {
				WriteErrorLog(string("Found 'value' for unsupported METADATA_TYPE: " + meta_entry->value_type).c_str());
				goto invalid;										
			}
		} else if (key == "contains") {
			if (entry_itr->value.IsArray()) {

				int value_size = entry_itr->value.Size();
				for (int value_idx = 0; value_idx < value_size; ++value_idx) {
					string strval = entry_itr->value[value_idx].GetString();

					// Remember what this entry is used for
					if (strval == "source") {
						contains_source = true;
					} else if (strval == "groundtruth") {
						contains_groundtruth = true;
					}
				}
			}
		} else {
			// Generic optional fields follow
			if (entry_itr->value.IsArray()) {

				int value_size = entry_itr->value.Size();

				for (int value_idx = 0; value_idx < value_size; ++value_idx) {
					meta_entry->meta_fields[key].push_back(entry_itr->value[value_idx].GetString());
				}
			}
		}
	}	// End of object member iteration

	// One must be specified at a minimum unless you're an archive
	if (!contains_source && !contains_groundtruth && meta_entry->value_type != METADATA_VALUE_TYPE_ARCHIVE) {
		WriteDebugLog("Metadata entry does indicate source or groundtruth usage!");
		return NULL;
	}

	// No filename was passed by API. Lets generate one
	if (meta_entry->filename == "") {
		meta_entry->filename = md5(meta_entry->url);

		if (meta_entry->dtype == "jpeg")
			meta_entry->filename += ".jpg";
		else if (meta_entry->dtype == "png")
			meta_entry->filename += ".png";
	}

	// Found a combined record. probably an archive
	if (contains_source && contains_groundtruth) {

		meta_entry->meta_type = METADATA_TYPE_SOURCE;
		meta_output->entries.push_back(meta_entry);

		// Create a copy since both source and ground need this data
		MetadataEntry* meta_entry2 = new MetadataEntry();
		memcpy(meta_entry2, meta_entry, sizeof(MetadataEntry));
		meta_entry2->meta_type = METADATA_TYPE_GROUND;
		meta_output->entries.push_back(meta_entry2);
	} else {

		// Only 1 'contains' is set
		if (contains_source)
			meta_entry->meta_type = METADATA_TYPE_SOURCE;
		else if (contains_groundtruth)
			meta_entry->meta_type = METADATA_TYPE_GROUND;

		meta_output->entries.push_back(meta_entry);
	}

	return meta_output;

	invalid:
	return NULL;
}