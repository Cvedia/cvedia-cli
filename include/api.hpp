#ifndef _API_HPP
#define _API_HPP

#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"

using namespace std;
using namespace rapidjson;

#define API_VERSION "1"
#define CLI_VERSION "1"

// API Calls defined
#define API_FUNCTION_HELLO			"welcome"
#define API_FUNCTION_COUNT			"meta"
#define API_FUNCTION_FETCH_BATCH	"fetch?size=$BATCHSIZE&offset=$OFFSET"

#define METADATA_VALUE_TYPE_IMAGE	"image"
#define METADATA_VALUE_TYPE_RAW		"raw"
#define METADATA_VALUE_TYPE_ARCHIVE	"archive"
#define METADATA_VALUE_TYPE_STRING	"string"
#define METADATA_VALUE_TYPE_NUMERIC	"numeric"

#define METADATA_TYPE_SOURCE		"source"
#define METADATA_TYPE_GROUND		"ground"

#define METADATA_TRAIN				"train"
#define METADATA_TEST				"test"
#define METADATA_VALIDATE			"validate"

struct DatasetMapping {
	string name;
	int id;
	int field_id;
};

struct DatasetSet {
	string set_name;
	float set_perc;
};

struct DatasetMetadata {

	int count;

	vector<DatasetMapping* > mapping_by_id;
	map<string, DatasetMapping* > mapping_by_name;
	map<int, DatasetMapping* > mapping_by_field_id;

	vector<DatasetSet> sets;
};

struct MetadataEntry {

	int id;
	int field_id;

	string meta_type;
	string value_type;

	string file_uri;
	string filename;
	string url;
	string dtype;

	string field_name;

	int int_value;
	float float_value;

	vector<vector<float>> float_array; 
	vector<vector<int>> int_array; 

	vector<string> string_value;

	// This metadata applies to all image or raw data vectors
	int data_channels;
	int data_width;
	int data_height;

	vector<unsigned char> image_data;	// Difference between image_data and uint8_raw_data is that image_data is PNG/JPEG compressed
	vector<uint8_t> uint8_raw_data;
	vector<float> float_raw_data;

	// Optional fields follow
	map<string, vector<string>> meta_fields;
};

struct Metadata{

	string setname;
	string hash;
	bool skip_record;

	vector<MetadataEntry* > entries;
};

#include "datawriter.hpp"

extern deque<vector<Metadata* >> feed_readahead;
extern mutex feed_mutex;

int InitializeApi();
string WriteImageData(string filename, uint8_t* image_data, unsigned int len, bool dir_tree);
DatasetMetadata* GetDatasetMetadata(string job_id);
vector<Metadata* > FetchBatch(map<string,string> options, int batch_idx);
vector<Metadata* > ParseFeed(const char* feed);
vector<MetadataEntry* > ParseTarFeed(const char* feed);
Metadata* ParseDataEntry(const Value& entryObj, Metadata* meta_output);
void ReadaheadBatch(map<string,string> options, int batch_idx);
void StartFeedThread(map<string,string> options, int batch_idx);
void StopFeedThread();

#endif
