#ifndef _API_HPP
#define _API_HPP

using namespace std;

#define API_VERSION "1"
#define CLI_VERSION "1"

// API Calls defined
#define API_FUNCTION_HELLO			"welcome.php"
#define API_FUNCTION_COUNT			"count.php"
#define API_FUNCTION_FETCH_BATCH	"fetch.php?size=$BATCHSIZE&offset=$BATCHID"

#define METADATA_VALUE_TYPE_IMAGE	"image"
#define METADATA_VALUE_TYPE_RAW		"raw"
#define METADATA_VALUE_TYPE_LABEL	"label"
#define METADATA_VALUE_TYPE_NUMERIC	"numeric"

#define METADATA_TYPE_SOURCE		"source"
#define METADATA_TYPE_GROUND		"ground"

struct MetadataEntry{

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

	vector<uint8_t> image_data;
	vector<uint8_t> uint8_raw_data;
	vector<float> float_raw_data;

	// Optional fields follow
	map<string, vector<string>> meta_fields;
};

struct Metadata{

	int type;

	vector<MetadataEntry* > entries;
};

#include "datawriter.hpp"

int InitializeApi();
int GetTotalDatasetSize(map<string,string> options);
vector<Metadata* > FetchBatch(map<string,string> options, int batch_idx);
vector<Metadata* > ParseFeed(const char * feed);

#endif
