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
#define API_FUNCTION_HELLO			"welcome.php"
#define API_FUNCTION_COUNT			"count.php"
#define API_FUNCTION_FETCH_BATCH	"fetch.php?size=$BATCHSIZE&offset=$BATCHID"

#define METADATA_VALUE_TYPE_IMAGE	"image"
#define METADATA_VALUE_TYPE_RAW		"raw"
#define METADATA_VALUE_TYPE_ARCHIVE	"archive"
#define METADATA_VALUE_TYPE_LABEL	"label"
#define METADATA_VALUE_TYPE_NUMERIC	"numeric"

#define METADATA_TYPE_SOURCE		"source"
#define METADATA_TYPE_GROUND		"ground"

#define METADATA_TRAIN				"train"
#define METADATA_TEST				"test"
#define METADATA_VALIDATE			"validate"

struct MetadataEntry{

	int id;

	string meta_type;
	string value_type;

	string file_uri;
	string filename;
	string url;
	string dtype;

	int int_value;
	float float_value;

	vector<vector<float>> float_array; 
	vector<vector<int>> int_array; 

	vector<string> label;

	// This metadata applies to all image or raw data vectors
	int data_channels;
	int data_width;
	int data_height;

	vector<uint8_t> image_data;	// Difference between image_data and uint8_raw_data is that image_data is PNG/JPEG compressed
	vector<uint8_t> uint8_raw_data;
	vector<float> float_raw_data;

	// Optional fields follow
	map<string, vector<string>> meta_fields;
};

struct Metadata{

	string type;

	vector<MetadataEntry* > entries;
};

#include "datawriter.hpp"

int InitializeApi();
int GetTotalDatasetSize(map<string,string> options);
vector<Metadata* > FetchBatch(map<string,string> options, int batch_idx);
vector<Metadata* > ParseFeed(const char* feed);
vector<Metadata* > ParseTarFeed(const char* feed);
Metadata* ParseDataEntry(const Value& entryObj, Metadata* meta_output);

#endif
