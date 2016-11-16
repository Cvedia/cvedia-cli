#ifndef _API_HPP
#define _API_HPP

using namespace std;

// API Calls defined
#define API_FUNCTION_HELLO			"?mode=welcome"
#define API_FUNCTION_COUNT			"?mode=count"
#define API_FUNCTION_FETCH_BATCH	"?batchsize=$BATCHSIZE&batchid=$BATCHID"

struct Metadata{

	// Not all fields will be filled in. This highly depends on the metadata available
	// But the following are the mandatory ones
	string file_uri;
	string filename;
	string url;
	int type;

	vector<uint8_t> image_data;

	// Optional fields follow
	map<string, vector<string>> meta_fields;
};

#include "datawriter.hpp"

int InitializeApi();
int GetTotalDatasetSize(string export_code);
vector<Metadata* > FetchBatch(string export_code, int batch_idx);
vector<Metadata* > ParseFeed(const char * feed);

#endif