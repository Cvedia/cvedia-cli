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

#include "api.hpp"
#include "easylogging++.h"
#include "globals.hpp"
#include "md5.hpp"
#include "optionparser.h"
#include "cvedia.hpp"
#include "curlreader.hpp"
#include "csvwriter.hpp"

using namespace std;
using namespace rapidjson;

extern DatasetMetadata* gDatasetMeta;
extern string gWorkingDir;
deque<vector<Metadata* >> feed_readahead;
mutex feed_mutex;
thread th_readahead;
bool gTerminateReadahead;

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

				LOG(INFO) << "API version: " << api_version;
				LOG(INFO) << motd;
				delete req;
				return 0;
			}
		}
	}

	LOG(FATAL) << "Failed to initialize Cvedia API (" << api_url << ")";

	if (req != NULL)
		delete req;

	return -1;
}

/**
    Returns the metadata describing the specified Job ID

    @param job_id the jobid to retrieve
    @return DatasetMetadata struct containing all details
*/
DatasetMetadata* GetDatasetMetadata(string job_id) {

	int count = 0;

	DatasetMetadata* meta = new DatasetMetadata;

	string api_url = gApiUrl + gAPIVersion + "/" + job_id + "/" + string(API_FUNCTION_COUNT);

	LOG(INFO) << "Fetching dataset metadata at " << api_url;

	ReadRequest* req = CurlReader::RequestUrl(api_url);

	if (req != NULL && req->read_data.size() > 0) {

		Document d;
		string data_str( req->read_data.begin(), req->read_data.end() );
		LOG(INFO) << "Before Parse";

		if (d.Parse(data_str.c_str()).HasParseError()) {
			LOG(ERROR) << "Failed to parse API Response. Does the JobID exist?";
			return NULL;
		}
		LOG(INFO) << "After Parse";

		if (d.HasMember("count")) {
			LOG(INFO) << "hasmember count";
			Value& count_val = d["count"];

			count = count_val.GetInt();
			LOG(INFO) << "Get count";
		} else {
			LOG(ERROR) << "Could not get stats for JobID. Does it exist?";
			return NULL;
		}
		LOG(INFO) << "Lalalalalal";

		meta->count = count;

		if (d.HasMember("mapping")) {

			LOG(DEBUG) << "Loading output mapping";

			Value& mapping = d["mapping"];

			if (mapping.IsArray()) {

				int map_size = mapping.Size();

				for (int map_idx = 0; map_idx < map_size; ++map_idx) {
					const Value& obj = mapping[map_idx];

					DatasetMapping* mapping_entry = new DatasetMapping;

					// Iterate through all object members
					for (Value::ConstMemberIterator itr = obj.MemberBegin(); itr != obj.MemberEnd(); ++itr) {

						string key = itr->name.GetString();

						if (key == "name") {
							mapping_entry->name = itr->value.GetString();
							// Add the mapping entry to our name lookup table
							meta->mapping_by_name[mapping_entry->name] = mapping_entry;
						} else if (key == "id") {
							mapping_entry->id = itr->value.GetInt();							
							// Add the mapping entry to our input id table
							meta->mapping_by_id.push_back(mapping_entry);
						} else if (key == "fieldid") {
							mapping_entry->field_id = itr->value.GetInt();
							// Add the mapping entry to our field_id lookup table
							meta->mapping_by_field_id[mapping_entry->field_id] = mapping_entry;
						}
					}
				}
			}

		} else {
			LOG(ERROR) << "Missing output mapping";			
			return NULL;
		}

		if (d.HasMember("splits")) {

			LOG(DEBUG) << "Loading output sets";

			Value& sets = d["splits"];

			if (sets.IsObject()) {

				// Iterate through all object members
				for (Value::ConstMemberIterator itr = sets.MemberBegin(); itr != sets.MemberEnd(); ++itr) {

					string key = itr->name.GetString();
					LOG(DEBUG) << "Key: " << key;
					LOG(DEBUG) << "Getint";
					float perc = itr->value.GetInt();
					LOG(DEBUG) << "Aftr Getint";

					DatasetSet s;

					s.set_name = key;
					s.set_perc = perc;

					meta->sets.push_back(s);
				}
			}

		} else {
			LOG(ERROR) << "Missing output sets";			
			return NULL;
		}		
	} else {
		LOG(ERROR) << "No data returned by API call";
	}

	return meta;
}

void StartFeedThread(map<string,string> options, int batch_idx) {
	
	gTerminateReadahead = false;

	LOG(INFO) << "Starting feed readahead thread";
	th_readahead = thread(ReadaheadBatch, options, batch_idx);	
}

void StopFeedThread() {

	gTerminateReadahead = true;

	LOG(DEBUG) << "Waiting for feed readahead thread to exit";
	th_readahead.join();
}

void ReadaheadBatch(map<string,string> options, int batch_idx) {

	do {

		vector<Metadata* > feed = FetchBatch(options, batch_idx);

		if (feed.size() == 0) {
			LOG(DEBUG) << "Feed reader finished at batch_idx " << batch_idx;
			return;
		}

		feed_mutex.lock();
		{
			// Loop until we have space to fetch more feeds
			do {
				if (feed_readahead.size() < 4)
					break;
				feed_mutex.unlock();
				sleep(1);

				feed_mutex.lock();
			} while(gTerminateReadahead == false);

			feed_readahead.push_back(feed);
		}
		feed_mutex.unlock();

		batch_idx++;
	} while(gTerminateReadahead == false);
}

vector<Metadata* > FetchBatch(map<string,string> options, int batch_idx) {

	vector<Metadata* > meta_vector;

	string api_url = gApiUrl + gAPIVersion + "/" + gJobID + "/" + string(API_FUNCTION_FETCH_BATCH);
	LOG(DEBUG) << "ApiUrl: " << api_url;
	api_url = ReplaceString(api_url, "$BATCHSIZE", to_string(gBatchSize));
	if (options.count("api_random") == 1) {
		api_url = ReplaceString(api_url, "$OFFSET", "0");		
		api_url += "&random";
	} else {
		api_url = ReplaceString(api_url, "$OFFSET", to_string(batch_idx*gBatchSize));		
	}

	LOG(DEBUG) << "Fetching batch at " << api_url;

	ReadRequest *req = CurlReader::RequestUrl(api_url);
	LOG(DEBUG) << "RequestUrl done ";
	if(req == NULL){
		LOG(DEBUG) << "Req is Null";
	}
	if (req != NULL && req->read_data.size() > 0) {
		LOG(DEBUG) << "ReadingData ";

		string data_str( req->read_data.begin(), req->read_data.end() );
		LOG(DEBUG) << "Before ParseFeed ";
		meta_vector = ParseFeed(data_str.c_str());
		LOG(DEBUG) << "ParseFeed done ";
	}

	LOG(DEBUG) << "Received " << req->read_data.size() << " bytes from API";

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

				meta_record->skip_record = false;

				// Iterate through all object members
				for (Value::ConstMemberIterator itr = obj.MemberBegin(); itr != obj.MemberEnd(); ++itr) {

					string key = itr->name.GetString();

					// Couple of key members that are required and processed seperately
					if (key == "set") {
						meta_record->setname = itr->value.GetString();
					} else if (key == "data") {

						if (itr->value.IsArray()) {

							int data_cnt = itr->value.Size();
							for (int data_idx = 0; data_idx < data_cnt; ++data_idx) {

								const Value &entryObj = itr->value[data_idx];

								meta_record = ParseDataEntry(entryObj, meta_record);
								assert(meta_record != NULL);

							} // End of loop over "data": []
						}
					} else if (key == "hash") {
						meta_record->hash = itr->value.GetString();

					} else {
						LOG(ERROR) << "Unsupported fields found in body: " << key;
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
	LOG(ERROR) << "ParseFeed() Error parsing JSON data";
	meta_vector.clear();
	return meta_vector;
}

string WriteImageData(string filename, uint8_t* image_data, unsigned int len, bool dir_tree) {

	string path = gWorkingDir + "data/";

	int dir_err = mkdir(path.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
	if (dir_err == -1 && errno != EEXIST) {
		LOG(ERROR) << "Could not create directory: " << path;
		return "";
	}

	// Must be at least 7 chars long for directory splitting (3 chars + 4 for extension)
	if (filename.size() > 7 && dir_tree == true) {

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

	return path + filename;
}

vector<MetadataEntry* > ParseTarFeed(const char* feed) {

	vector<MetadataEntry* > meta_vector;

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

			meta_vector.insert(meta_vector.end(), meta_record->entries.begin(), meta_record->entries.end());
			delete meta_record;
		}

	} else {
		goto invalid;
	}

	return meta_vector;

	invalid:
	LOG(ERROR) << "ParseTarFeed() Error parsing JSON data";
	meta_vector.clear();
	return meta_vector;
}

Metadata* ParseDataEntry(const Value &entryObj, Metadata* meta_output) {

	MetadataEntry* meta_entry = new MetadataEntry();

	meta_entry->field_id = -1;
	meta_entry->id = -1;

	// These keys are checked before the rest since they influence the
	// parsing of the data
	if (entryObj["type"].IsString()) {
		meta_entry->value_type = entryObj["type"].GetString();
	} else {									
		LOG(ERROR) << "Metadata entry does not specify a 'type'";
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
		} else if (key == "fieldid") {
			meta_entry->field_id = entry_itr->value.GetInt();
			// Translate field_id to an output field
			meta_entry->id = gDatasetMeta->mapping_by_field_id[meta_entry->field_id]->id;
		} else if (key == "width") {
			meta_entry->data_width = entry_itr->value.GetInt();
		} else if (key == "height") {
			meta_entry->data_height = entry_itr->value.GetInt();
		} else if (key == "name") {
			meta_entry->field_name = entry_itr->value.GetString();
			// Translate field_name to an output field
			meta_entry->id = gDatasetMeta->mapping_by_name[meta_entry->field_name]->id;
		} else if (key == "value") {
			if (meta_entry->value_type == METADATA_VALUE_TYPE_STRING) {

				if (entry_itr->value.IsArray()) {

					int value_size = entry_itr->value.Size();

					for (int value_idx = 0; value_idx < value_size; ++value_idx) {
						meta_entry->string_value.push_back(entry_itr->value[value_idx].GetString());
					}
				}
			} else if (meta_entry->value_type == METADATA_VALUE_TYPE_NUMERIC) {
				if (meta_entry->dtype == "") {
					LOG(ERROR) << "Missing 'dtype' for METADATA_VALUE_TYPE_NUMERIC";
					goto invalid;
				}

				if (meta_entry->dtype == "int") {
					meta_entry->int_value = entry_itr->value.GetInt();
				} else if (meta_entry->dtype == "float") {
					meta_entry->float_value = entry_itr->value.GetFloat();
				} else {
					LOG(ERROR) << "Invalid 'dtype' found for METADATA_VALUE_TYPE_NUMERIC: " << meta_entry->dtype;
					goto invalid;												
				}

			} else {
				LOG(ERROR) << "Found 'value' for unsupported METADATA_TYPE: " << meta_entry->value_type;
				goto invalid;										
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

	// Record not linked to any output. Impossible, unless it's an archive
	// which has its metadata in a file_id.diz
	if (meta_entry->id == -1 && meta_entry->value_type != METADATA_VALUE_TYPE_ARCHIVE) {
		LOG(ERROR) << "meta_entry has no valid ID";
		goto invalid;
	}

	// No filename was passed by API. Lets generate one
	if (meta_entry->filename == "") {
		meta_entry->filename = md5(meta_entry->url);

		if (meta_entry->dtype == "jpeg")
			meta_entry->filename += ".jpg";
		else if (meta_entry->dtype == "png")
			meta_entry->filename += ".png";
	}

	meta_output->entries.push_back(meta_entry);

	return meta_output;

	invalid:
	return NULL;
}
