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
#include "api.hpp"
#include "optionparser.h"
#include "cvedia.hpp"
#include "curlreader.hpp"
#include "csvwriter.hpp"

using namespace std;
using namespace rapidjson;

int InitializeApi() {

	// Execute welcome call to make sure API is valid
	string api_url = gApiUrl + string(API_FUNCTION_HELLO);

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

	WriteDebugLog("Failed to initialize Cvedia API");

	if (req != NULL)
		delete req;

	return -1;
}

int GetTotalDatasetSize(string export_code) {

	int count = 0;

	string api_url = gApiUrl + string(API_FUNCTION_COUNT);

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

vector<Metadata* > FetchBatch(string export_code, int batch_idx) {

	vector<Metadata* > meta_vector;

	string api_url = gApiUrl + string(API_FUNCTION_FETCH_BATCH);
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

vector<Metadata* > ParseFeed(const char * feed) {

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
				Metadata* meta_entry = new Metadata;

				// Iterate through all object members
				for (Value::ConstMemberIterator itr = obj.MemberBegin(); itr != obj.MemberEnd(); ++itr) {

					string key = itr->name.GetString();

					// Couple of key members that are required and processed seperately
					if (key == "filename") {
						meta_entry->filename = itr->value.GetString();
					} else if (key == "url") {
						meta_entry->url = itr->value.GetString();							
					} else if (key == "type") {
						meta_entry->type = itr->value.GetInt();
					} else {
						// Generic optional fields follow
						if (itr->value.IsArray()) {

							int value_size = itr->value.Size();

							for (int value_idx = 0; value_idx < value_size; ++value_idx) {
								meta_entry->meta_fields[key].push_back(itr->value[value_idx].GetString());
							}
						}
					}
				}

				// Store new Metadata in the output vector
				meta_vector.push_back(meta_entry);

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
