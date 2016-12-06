#include "config.hpp"

#include <cstring>
#include <cstdlib>
#include <deque>
#include <map>
#include <vector>
#include <algorithm>
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
#include <sys/ioctl.h>

#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"

using namespace std;
using namespace rapidjson;

#include "md5.hpp"
#include "api.hpp"
#include "functions.hpp"
#include "optionparser.h"
#include "cvedia.hpp"
#include "curlreader.hpp"
#include "csvwriter.hpp"
#include "tfrecordswriter.hpp"
#include "hdf5writer.hpp"
#include "pythonwriter.hpp"
#include "caffeimagedata.hpp"


// Initialize global variables
int gDebug 		= 1;

int gBatchSize 	= 256;
int gDownloadThreads = 100;

int lTermWidth = 80;

string gBaseDir = "";
string gExportName = "";
string gApiUrl = "http://api.cvedia.com/";
string gOutputFormat = "csv";
string gJobID = "";

string gVersion = CLI_VERSION;
string gAPIVersion = API_VERSION;

void SigHandler(int s);
bool gInterrupted = false;

enum  optionIndex { UNKNOWN, HELP, JOB, DIR, NAME, API, OUTPUT, BATCHSIZE, THREADS, IMAGES_SAME_DIR, MOD_TFRECORDS_PER_SHARD, IMAGES_EXTERNAL};

int main(int argc, char* argv[]) {

	vector<string> supported_output;
	supported_output.push_back("CSV");
	supported_output.push_back("CaffeImageData");
#ifdef HAVE_HDF5
	supported_output.push_back("HDF5");
#endif
#ifdef HAVE_TFRECORDS
#ifdef HAVE_PYTHON
	supported_output.push_back("TFRecords");
#endif
#endif

	string output_string = JoinStringVector(supported_output, ",");

	option::Descriptor usage[] =
	{
		{UNKNOWN, 	0,"" , ""    ,option::Arg::None, string("CVEDIA-CLI v." + gVersion + " API Compatibility v." + gAPIVersion + "\n\nUSAGE: cvedia [options]\n\nOptions:").c_str() },
		{HELP,    	0,"" , "help",option::Arg::None, "  --help  \tPrint usage and exit." },
		{JOB,    	0,"j", "job",option::Arg::Required, "  --job=<id>, -j <id>  \tAPI Job ID" },
		{DIR,    	0,"d", "dir",option::Arg::Required, "  --dir=<path>, -d <path>  \tBase path for storing exported data (default: .)" },
		{NAME,    	0,"n", "name",option::Arg::Required, "  --name=<arg>, -n <arg>  \tName used for storing data on disk (defaults to jobid)" },
		{OUTPUT,   	0,"o", "output",option::Arg::Required, string("  --output=<module>, -o <module>  \tSupported modules are " + output_string + ". (default: CSV)").c_str() },
		{BATCHSIZE, 0,"b", "batch-size",option::Arg::Required, "  --batch-size=<num>, -b <num>  \tNumber of images to retrieve in a single batch (default: 256)." },
		{THREADS,   0,"t", "threads",option::Arg::Required, "  --threads=<num>, -t <num>  \tNumber of download threads (default: 100)." },
		{API,    	0,"", "api",option::Arg::Required, "  --api=<url>  \tREST API Connecting point (default: http://api.cvedia.com/)"  },
		{IMAGES_EXTERNAL,   0,"", "images-external",option::Arg::None, "  --images-external  \tStore the images as files on disk instead of inside the output format. This option might be overridden by the output module" },
		{IMAGES_SAME_DIR,   0,"", "images-same-dir",option::Arg::None, "  --images-same-dir  \tStore all images inside a single folder instead of tree structure." },
#ifdef HAVE_TFRECORDS
		{UNKNOWN, 	0,"" , ""    ,option::Arg::None, "\n             ##### TFRecords Module Options #####"},
		{MOD_TFRECORDS_PER_SHARD,   0,"", "tfrecords-entries-per-shard",option::Arg::None, "  --tfrecords-entries-per-shard  \tNumber of entries per shard. Set to 0 to disable sharding (default: 0)." },
#endif
		{UNKNOWN, 	0,"" ,  ""   ,option::Arg::None, "\nExamples:\n\tcvedia -j d41d8cd98f00b204e9800998ecf8427e\n\tcvedia -j d41d8cd98f00b204e9800998ecf8427e -n test_export --api=http://api.cvedia.com/\n" },
		{0,0,0,0,0,0}
	};

	struct winsize w;
	
	ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
	lTermWidth = w.ws_col;

	map<string,string> mod_options;
	
	argc-=(argc>0); argv+=(argc>0); // skip program name argv[0] if present
	option::Stats  stats(usage, argc, argv);
	option::Option options[stats.options_max], buffer[stats.buffer_max];
	option::Parser parse(usage, argc, argv, options, buffer);
	
	if (parse.error())
		return 1;
	
	if (options[HELP] || argc == 0 || !options[JOB]) {
		option::printUsage(std::cout, usage);
		return 0;
	}

	if (options[BATCHSIZE].count() == 1) {
		gBatchSize = atoi(options[BATCHSIZE].arg);
	}

	if (options[DIR].count() == 1) {
		gBaseDir = options[DIR].arg;
	}

	if (options[JOB].count() == 1) {
		gJobID = options[JOB].arg;
	}
	
	if (options[NAME].count() == 1) {
		gExportName = options[NAME].arg;
	} else { // fallback to jobid for name
		gExportName = gJobID;
	}
	
	if (options[API].count() == 1) {
		gApiUrl = options[API].arg;
	}
	
	if (options[THREADS].count() == 1) {
		gDownloadThreads = atoi(options[THREADS].arg);
	}
	
	if (options[IMAGES_SAME_DIR].count() == 1) {
		mod_options["images-same-dir"] = "1";
	}

	if (options[OUTPUT].count() == 1) {
		gOutputFormat = options[OUTPUT].arg;
	
		// Convert to lowercase
		std::transform(gOutputFormat.begin(), gOutputFormat.end(), gOutputFormat.begin(), ::tolower);
	}

	// Initialize the Curl library
	// We could've done this inside the curlreader but with those being created
	// and destroyed continually this is a better approach
	CURLcode res = curl_global_init(CURL_GLOBAL_NOTHING);
	if (res != 0) {
		cout << "curl_global_init(): " << curl_easy_strerror(res) << endl;
	}
	
	if (gBaseDir == "")
		gBaseDir = "./";
	else
		gBaseDir += "/";

	mod_options["base_dir"] = gBaseDir;
	mod_options["export_name"] = gExportName;
	mod_options["working_dir"] = gBaseDir + gExportName + "/";

	int dir_err = mkdir(mod_options["working_dir"].c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
	if (dir_err == -1 && errno != EEXIST) {
		WriteErrorLog(string("Could not create directory: " + mod_options["working_dir"]).c_str());
		return -1;
	}


	// Register our Interrupt handler
	struct sigaction sigIntHandler;

	sigIntHandler.sa_handler = SigHandler;
	sigemptyset(&sigIntHandler.sa_mask);
	sigIntHandler.sa_flags = 0;

	sigaction(SIGINT, &sigIntHandler, NULL);

	if (InitializeApi() == 0) {
		StartExport(mod_options);
	}
	
	return 1;
}

void SigHandler(int s) {

	if (gInterrupted == false) {
		gInterrupted = true;

		WriteLog("Ctrl-C received. Will finish current batch before exiting. Press Ctrl-C again to terminate immediately.");
	} else {
		WriteLog("Terminating immediately.");
		exit(1);
	}
}

int StartExport(map<string,string> options) {

	time_t seconds;

	options["create_test_file"] = "1";
	options["create_train_file"] = "1";
	options["create_validate_file"] = "1";

	CurlReader *p_reader = new CurlReader();

	IDataWriter *p_writer = NULL;

	if (gOutputFormat == "csv") {
		p_writer = new CsvWriter(gExportName, options);
	} else if (gOutputFormat == "caffeimagedata") {
		p_writer = new CaffeImageDataWriter(gExportName, options);
#ifdef HAVE_HDF5
	} else if (gOutputFormat == "hdf5") {
		p_writer = new Hdf5Writer(gExportName, options);
#endif
#ifdef HAVE_PYTHON
	} else if (gOutputFormat == "tfrecords") {
		p_writer = new PythonWriter(gExportName, options);
#endif
	} else {
		WriteErrorLog(string("Unsupported output module specified: " + gOutputFormat).c_str());
	}

	if (p_writer->Initialize() != 0) {
		WriteErrorLog(string("Failed to initialize " + gOutputFormat).c_str());
		return -1;
	}

	p_reader->SetNumThreads(gDownloadThreads);

	int batch_size = GetTotalDatasetSize(options);
	WriteDebugLog(string("Total expected dataset size is " + to_string(batch_size)).c_str());

	// Fetch basic stats on export
	int num_batches = ceil(batch_size / (float)gBatchSize);

	for (int batch_idx = 0; batch_idx < num_batches && gInterrupted == false; batch_idx++) {

		unsigned int queued_downloads = 0;

		vector<Metadata* > meta_data = FetchBatch(options, batch_idx);

		if (meta_data.size() == 0) {
			WriteDebugLog(string("No metadata return by API, end of dataset?").c_str());
			return -1;
		}

		WriteDebugLog(string("Starting download for batch #" + to_string(batch_idx)).c_str());

		seconds = time(NULL);

		// Queue up all requests in this batch
		for (Metadata* m : meta_data) {
			for (MetadataEntry* entry : m->entries) {

				// Download all images/raw/archives for both source and groundtruth
				if (entry->url != "") {
					p_reader->QueueUrl(md5(entry->url), entry->url);
					queued_downloads++;
				}
			}
		}

		ReaderStats stats = p_reader->GetStats();

		// Loop until all downloads are finished
		while (stats.num_reads_completed < queued_downloads) {

			stats = p_reader->GetStats();

			if (time(NULL) != seconds) {
				DisplayProgressBar(stats.num_reads_completed / (float)queued_downloads, stats.num_reads_completed, queued_downloads);

//				cout << batch_idx << " " << stats.num_reads_success << " " << stats.num_reads_empty << " " << stats.num_reads_error << endl;

				seconds = time(NULL);
			}

			usleep(100);
		}

		// Display the 100% complete
		DisplayProgressBar(1, stats.num_reads_completed, queued_downloads);
		cout << endl;

		// Update stats for last time
		stats = p_reader->GetStats();

		WriteDebugLog(string("Downloaded " + to_string(stats.bytes_read) + " bytes").c_str());
		WriteDebugLog("Syncing batch to disk...");

		p_reader->ClearStats();

		map<string, ReadRequest* > responses = p_reader->GetAllData();

		for (Metadata* m : meta_data) {
			// Find the request for a specific filename

			for (MetadataEntry* entry : m->entries) {

				if (entry->url != "") {	// Did we download something ?
					ReadRequest* req = responses[md5(entry->url)];

					if (req != NULL && req->read_data.size() > 0) {

						// We found the download for a piece of metadata
						if (entry->value_type == METADATA_VALUE_TYPE_IMAGE)
							entry->image_data = req->read_data;
						else if (entry->value_type == METADATA_VALUE_TYPE_RAW) {
							if (entry->dtype == "uint8")
								entry->uint8_raw_data = req->read_data;
							else if (entry->dtype == "float") {

								unsigned int rsize = req->read_data.size() / 4;
								if (rsize * 4 != req->read_data.size()) {
									WriteDebugLog(string("Raw data with dtype float is not divisible by 4. Is the data in float format?").c_str());
								}

								for (unsigned int ridx = 0; ridx < rsize; ridx++) {
									entry->float_raw_data.push_back(((float *)&req->read_data)[ridx]);								
								}
							} else {
								WriteErrorLog(string("Unsupported dtype for METADATA_TYPE_RAW: " + entry->dtype).c_str());
							}
						} else {
							WriteErrorLog(string("Encountered download for unsupported source METADATA_VALUE_TYPE: " + entry->value_type).c_str());
							return -1;
						}
					} else {
						WriteDebugLog(string("No download results for: " + entry->url).c_str());
					}
				}
			}	// for (MetaDataEntry* entry : m->entries)
		}	// for (Metadata* m : meta_data)

		bool is_valid = p_writer->ValidateData(meta_data);
		if (!is_valid) {
			WriteErrorLog(string("DataWriter returned falsed on ValidateData").c_str());
			return -1;
		}

		for (Metadata* m : meta_data) {
			p_writer->WriteData(m);
		}

		p_reader->ClearData();

		// We are completely done with the response data
		for (auto& kv : responses) {
			delete kv.second;
		}

		// Flush any other data from memory to disk
		p_writer->Finalize();

		responses.clear();
	}

	return 0;
}

void DisplayProgressBar(float progress, int cur_value, int max_value) {
	cout << "[";
	int bar_width = lTermWidth - (to_string(cur_value).length() + to_string(max_value).length() + 6);
	int pos = bar_width * progress;
	
	for (int i = 0; i < bar_width; ++i) {
		if (i < pos) cout << "=";
		else if (i == pos) cout << ">";
		else cout << " ";
	}
	
	cout << "] [" << to_string(cur_value) << "/" << to_string(max_value) << "]\r";
	cout.flush();
}

string JoinStringVector(const vector<string>& vec, const char* delim)
{
	string out = "";

	for (string s : vec) 
	{
		if (!out.empty())
			out += string(delim) + " ";

		out += s;
	}

	return out;
}
