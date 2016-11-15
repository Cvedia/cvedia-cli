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

#include "optionparser.h"
#include "cvedia.hpp"
#include "curlreader.hpp"
#include "csvwriter.hpp"

using namespace std;
using namespace rapidjson;

int gDebug 		= 1;

int gBatchSize 	= 256;
int gDownloadThreads = 100;

string gBaseDir = "";
string gExportName = "";

enum  optionIndex { UNKNOWN, HELP, DIR, NAME, BATCHSIZE, THREADS };
const option::Descriptor usage[] =
{
	{UNKNOWN, 0,"" , ""    ,option::Arg::None, "USAGE: cvedia [options]\n\n"
	                                         "Options:" },
	{HELP,    0,"" , "help",option::Arg::None, "  --help  \tPrint usage and exit." },
	{DIR,    0,"d", "dir",option::Arg::Required, "  --dir=<path>, -d <path>  \tBase path for storing exported data" },
	{NAME,    0,"n", "name",option::Arg::Required, "  --name=<arg>, -n <arg>  \tName used for storing data on disk" },
	{BATCHSIZE,    0,"b", "batch-size",option::Arg::Required, "  --batch-size=<num>, -b <num>  \tNumber of images to retrieve in a single batch (default: 256)." },
	{THREADS,    0,"t", "threads",option::Arg::Required, "  --threads=<num>, -t <num>  \tNumber of download threads (default: 100)." },
	{UNKNOWN, 0,"" ,  ""   ,option::Arg::None, "\nExamples:\n"
	                                         "  cvedia -n test_export\n" },
	{0,0,0,0,0,0}
};

int main(int argc, char* argv[]) {
	
	argc-=(argc>0); argv+=(argc>0); // skip program name argv[0] if present
	option::Stats  stats(usage, argc, argv);
	option::Option options[stats.options_max], buffer[stats.buffer_max];
	option::Parser parse(usage, argc, argv, options, buffer);

	if (parse.error())
		return 1;

	if (options[HELP] || argc == 0 || !options[NAME]) {
		option::printUsage(std::cout, usage);
		return 0;
	}

	if (options[BATCHSIZE].count() == 1) {
		gBatchSize = atoi(options[BATCHSIZE].arg);
	}

	if (options[DIR].count() == 1) {
		gBaseDir = options[DIR].arg;
	}

	if (options[NAME].count() == 1) {
		gExportName = options[NAME].arg;
	}

	if (options[THREADS].count() == 1) {
		gDownloadThreads = atoi(options[THREADS].arg);
	}

	StartExport("");
}

int StartExport(string export_code) {

	time_t seconds;

	map<string,string> options;
	options["base_dir"] = gBaseDir;
	options["create_test_file"] = "1";
	options["create_train_file"] = "1";

	IDataReader *p_reader = new CurlReader();
	IDataWriter *p_writer = new CsvWriter(gExportName, options);
	
	if (p_writer->Initialize() != 0) {
		WriteErrorLog("Failed to initialize CsvWriter");
		return -1;
	}

	p_reader->SetNumThreads(gDownloadThreads);

	ReadRequest *req = p_reader->RequestUrl("https://jsonplaceholder.typicode.com/photos");

	// TODO: Temporary code instead of API
	ifstream api_results("../tmp/api.txt");
	if (api_results == NULL) {
		WriteErrorLog("Failed to read from API");
		return -1;
	}

	string api_data((istreambuf_iterator<char>(api_results)),
				istreambuf_iterator<char>());

	vector<string> api_lines = split(api_data, '\n');
	// END

	// Fetch basic stats on export
	int num_batches = ceil(api_lines.size() / gBatchSize);

	for (int batch_idx = 0; batch_idx < num_batches; batch_idx++) {

		WriteDebugLog(string("Starting download for batch #" + to_string(batch_idx)).c_str());

		seconds = time(NULL);

		// Queue up all requests in this batch
		for (int i = 0; i < gBatchSize; i++) {
			int line_number = batch_idx * gBatchSize + i;

			p_reader->QueueUrl(to_string(i), api_lines[line_number]);
		}

		ReaderStats stats = p_reader->GetStats();

		// Loop until all downloads are finished
		while (stats.num_reads_completed < gBatchSize) {

			stats = p_reader->GetStats();

			if (time(NULL) != seconds) {
				cout << batch_idx << " " << stats.num_reads_success << " " << stats.num_reads_empty << " " << stats.num_reads_error << endl;

				seconds = time(NULL);
			}

			usleep(100);
		}

		// Update stats for last time
		stats = p_reader->GetStats();

		WriteDebugLog(string("Downloaded " + to_string(stats.bytes_read) + " bytes").c_str());
		WriteDebugLog("Syncing batch to disk...");

		p_reader->ClearStats();
	}

	return 0;
}

void split(const string &s, char delim, vector<string> &elems) {
    stringstream ss;
    ss.str(s);
    string item;
    while (getline(ss, item, delim)) {
        elems.push_back(item);
    }
}

vector<string> split(const string &s, char delim) {
    vector<string> elems;
    split(s, delim, elems);
    return elems;
}
