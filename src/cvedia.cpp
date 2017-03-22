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
#include <archive.h>
#include <archive_entry.h> 
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>

#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"

using namespace std;
using namespace rapidjson;

#include "md5.hpp"
#include "api.hpp"
#include "functions.hpp"
#include "easylogging++.h"
#include "optionparser.h"
#include "metadb.hpp"
#include "cvedia.hpp"
#include "curlreader.hpp"
#include "csvwriter.hpp"
#include "hdf5writer.hpp"
#include "pythonwriter.hpp"
#include "imagemean.hpp"
#include "caffeimagedata.hpp"
#include "pythonmodules.hpp"

extern bool gTerminateReadahead;

INITIALIZE_EASYLOGGINGPP

// Initialize global variables
int gDebug 		= 1;

int gBatchSize 			= 256;
int gDownloadThreads 	= 100;
int gIterations 		= 1;

vector<export_module> gModules;

DatasetMetadata* gDatasetMeta;

int lTermWidth = 80;

bool gVerifyApi = false;
bool gVerifyLocal = false;
bool gSubtractMean = false;
int gGenerateMean = MEAN_NONE;

ImageMean* gMean = NULL;


string gBaseDir = "";
string gExportName = "";
string gWorkingDir = "";
el::Level gLogLevel;
string gApiUrl = "http://api.cvedia.com/";
string gOutputFormat = "csv";
string gJobID = "";

string gVersion = CLI_VERSION;
string gAPIVersion = API_VERSION;

int gResume = MODE_NEW;

void SigHandler(int s);
bool gInterrupted = false;

enum  optionIndex { UNKNOWN, HELP, JOB, DIR, NAME, API, OUTPUT, BATCHSIZE, THREADS, ITERATIONS, LOGLEVEL, VERIFYAPI, VERIFYLOC, GLOBALMEAN, CHANNELMEAN, PIXELMEAN, SUBTRACTMEAN, SAVEMEAN, RESUME, 
	IMAGES_SAME_DIR, MOD_TFRECORDS_PER_SHARD, IMAGES_EXTERNAL};

int main(int argc, char* argv[]) {

	vector<string> supported_output;
	vector<option::Descriptor> module_vec;

	supported_output.push_back("CSV");
	supported_output.push_back("CaffeImageData");
#ifdef HAVE_HDF5
	supported_output.push_back("HDF5");
#endif
#ifdef HAVE_PYTHON

	PythonModules::LoadPythonCore();

	gModules = PythonModules::ListModules("./python/");

	for (export_module module : gModules) {

		// Add module to the list of supported output modules
		supported_output.push_back(module.module_name);

		string str_mod_help = string("\n\n\t##### " + module.module_name + " Module Options #####\n");
		char* mod_help = new char[str_mod_help.length()+1]();
		strcpy(mod_help, str_mod_help.c_str());

		module_vec.push_back({UNKNOWN, 	0,"" , ""    ,option::Arg::None, mod_help});

		for (export_module_param param : module.module_params) {

			param.help = string(param.example + "  \t" + param.description);

			// Convert the string() object to const char*. Converting them to c_str()
			// directly into the Descriptor struct does not guarantuee their continued existence.
			// These pointers are not freed
			char* option = new char[param.option.length()+1]();
			strcpy(option, param.option.c_str());
			char* help = new char[param.help.length()+1]();
			strcpy(help, param.help.c_str());

			module_vec.push_back({UNKNOWN,    	0,"", option, (param.required == true ? option::Arg::Required : option::Arg::None), help });
		}
	}

#endif

	string output_string = JoinStringVector(supported_output, ",");

	vector<option::Descriptor> usage_vec;

	string version = string("CVEDIA-CLI v." + gVersion + " API Compatibility v." + gAPIVersion + "\n\nUSAGE: cvedia [options]\n\nOptions:");
	string strmods = string("  --output=<module>, -o <module>  \tSupported modules are " + output_string + ". (default: CSV)");

	usage_vec.push_back({UNKNOWN, 	0,"" , ""    ,option::Arg::None, version.c_str() });
	usage_vec.push_back({HELP,    	0,"" , "help",option::Arg::None, "  --help  \tPrint usage and exit." });
	usage_vec.push_back({JOB,    	0,"j", "job",option::Arg::Required, "  --job=<id>, -j <id>  \tAPI Job ID" });
	usage_vec.push_back({LOGLEVEL,	0,"l", "log-level",option::Arg::Required, "  --log-level=<level>, -l <level>  \tSpecify the log level (fatal,error,warning,debug,info,trace)" });
	usage_vec.push_back({DIR,    	0,"d", "dir",option::Arg::Required, "  --dir=<path>, -d <path>  \tBase path for storing exported data (default: .)" });
	usage_vec.push_back({NAME,    	0,"n", "name",option::Arg::Required, "  --name=<arg>, -n <arg>  \tName used for storing data on disk (defaults to jobid)" });
	usage_vec.push_back({RESUME,   	0,"" , "resume",option::Arg::None, "  --resume  \tResume downloading an existing job." });
	usage_vec.push_back({ITERATIONS,0,"i", "iterations",option::Arg::Required, "  --iterations=<num>, -i <num>  \tNumber of times to download the specified Job ID. This can be used in conjunction with jobs that have a random augmentation (default: 1)" });
	usage_vec.push_back({VERIFYAPI,	0,"" , "verify-api",option::Arg::None, "  --verify-api  \tVerify the state of an exported dataset with the Cvedia API." });
	usage_vec.push_back({VERIFYLOC,	0,"" , "verify-local",option::Arg::None, "  --verify-local  \tPerform integrity check between an exported dataset and the local Meta Db" });
	usage_vec.push_back({OUTPUT,   	0,"o", "output",option::Arg::Required, strmods.c_str() });
	usage_vec.push_back({BATCHSIZE, 0,"b", "batch-size",option::Arg::Required, "  --batch-size=<num>, -b <num>  \tNumber of images to retrieve in a single batch (default: 256)." });
	usage_vec.push_back({THREADS,   0,"t", "threads",option::Arg::Required, "  --threads=<num>, -t <num>  \tNumber of download threads (default: 100)." });
	usage_vec.push_back({API,    	0,"", "api",option::Arg::Required, "  --api=<url>  \tREST API Connecting point (default: http://api.cvedia.com/)"  });
	usage_vec.push_back({IMAGES_EXTERNAL,   0,"", "images-external",option::Arg::None, "  --images-external  \tStore images/blobs as files on disk. This is default for output formats that dont support binary storage" });
	usage_vec.push_back({IMAGES_SAME_DIR,   0,"", "images-same-dir",option::Arg::None, "  --images-same-dir  \tStore all images inside a single folder instead of tree structure." });
	usage_vec.push_back({UNKNOWN,   0,"", "",option::Arg::None, "\n\n\t##### Image Normalization #####\n" });
	usage_vec.push_back({GLOBALMEAN,	0,"" , "generate-global-mean",option::Arg::None, "  --generate-global-mean  \tGenerate a global mean approximation using random online sampling." });
	usage_vec.push_back({CHANNELMEAN,	0,"" , "generate-channel-mean",option::Arg::None, "  --generate-channel-mean  \tGenerate a per channel mean approximation using random online sampling." });
	usage_vec.push_back({PIXELMEAN,	0,"" , "generate-pixel-mean",option::Arg::None, "  --generate-pixel-mean  \tGenerate a mean image approximation using random online sampling." });
	usage_vec.push_back({SUBTRACTMEAN,	0,"" , "subtract-mean",option::Arg::None, "  --subtract-mean  \tSubtract the generated mean from all images." });
	usage_vec.push_back({SAVEMEAN,	0,"" , "save-mean",option::Arg::None, "  --save-mean=<format>  \tWrite the generated mean to disk (image, csv, raw)" });

	// Insert the python modules with their options
	for (option::Descriptor desc : module_vec) {
		// Have to manually insert them, .insert() is not able to assign the structure
		usage_vec.push_back({(unsigned int)usage_vec.size(), desc.type, desc.shortopt, desc.longopt, desc.check_arg, desc.help});
	}

	usage_vec.push_back({UNKNOWN, 	0,"" ,  ""   ,option::Arg::None, "\nExamples:\n\tcvedia -j d41d8cd98f00b204e9800998ecf8427e\n\tcvedia -j d41d8cd98f00b204e9800998ecf8427e -n test_export --api=http://api.cvedia.com/\n" });
	usage_vec.push_back({0,0,0,0,0,0});

	option::Descriptor *usage = &usage_vec[0]; 

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
	
	if (options[RESUME].count() == 1) {
		gResume = MODE_RESUME;
	}

	if (options[VERIFYAPI].count() == 1) {
		gVerifyApi = true;
	}


	if (options[VERIFYLOC].count() == 1) {
		gVerifyLocal = true;
	}
	
	if (options[GLOBALMEAN].count() == 1) {
		gGenerateMean = MEAN_GLOBAL;
	}

	if (options[CHANNELMEAN].count() == 1) {
		gGenerateMean = MEAN_CHANNEL;
	}

	if (options[PIXELMEAN].count() == 1) {
		gGenerateMean = MEAN_PIXEL;
	}

	if (options[SUBTRACTMEAN].count() == 1) {
		gSubtractMean = true;
	}

	if (options[THREADS].count() == 1) {
		gDownloadThreads = atoi(options[THREADS].arg);
	}
	
	if (options[ITERATIONS].count() == 1) {
		gIterations = atoi(options[ITERATIONS].arg);
	}

	if (options[IMAGES_EXTERNAL].count() == 1) {
		mod_options["images-external"] = "1";
	}

	if (options[IMAGES_SAME_DIR].count() == 1) {
		mod_options["images-same-dir"] = "1";
	}

	if (options[OUTPUT].count() == 1) {
		gOutputFormat = options[OUTPUT].arg;
	
		// Convert to lowercase
		std::transform(gOutputFormat.begin(), gOutputFormat.end(), gOutputFormat.begin(), ::tolower);
	}

	if (options[LOGLEVEL].count() == 1) {
		string loglevel = options[LOGLEVEL].arg;

		if (loglevel == "trace") {
			gLogLevel = el::Level::Trace;
		} else if (loglevel == "debug") {
			gLogLevel = el::Level::Debug;
		} else if (loglevel == "fatal") {
			gLogLevel = el::Level::Fatal;
		} else if (loglevel == "error") {
			gLogLevel = el::Level::Error;
		} else if (loglevel == "warning") {
			gLogLevel = el::Level::Warning;
		} else if (loglevel == "verbose") {
			gLogLevel = el::Level::Verbose;
		} else if (loglevel == "info") {
			gLogLevel = el::Level::Info;
		}
	} else {
		gLogLevel = el::Level::Info;
	}

	ConfigureLogger();

	// Initialize the Curl library
	// We could've done this inside the curlreader but with those being created
	// and destroyed continually this is a better approach
	CURLcode res = curl_global_init(CURL_GLOBAL_NOTHING);
	if (res != 0) {
		LOG(ERROR) << "curl_global_init(): " << curl_easy_strerror(res);
	}
	
	if (gBaseDir == "")
		gBaseDir = "./";
	else
		gBaseDir += "/";

	gWorkingDir = gBaseDir + gExportName + "/";

	mod_options["base_dir"] = gBaseDir;
	mod_options["export_name"] = gExportName;
	mod_options["working_dir"] = gWorkingDir;

	int dir_err = mkdir(mod_options["working_dir"].c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
	if (dir_err == -1 && errno != EEXIST) {
		LOG(ERROR) << "Could not create directory: " << mod_options["working_dir"];
		return -1;
	}


	// Register our Interrupt handler
	struct sigaction sigIntHandler;

	sigIntHandler.sa_handler = SigHandler;
	sigemptyset(&sigIntHandler.sa_mask);
	sigIntHandler.sa_flags = 0;

	sigaction(SIGINT, &sigIntHandler, NULL);

	if (InitializeApi() == 0) {

		if (gGenerateMean != MEAN_NONE) {
			GenerateMeanOnline(mod_options);
		} if (gVerifyApi == true) {
			VerifyApi(mod_options);
			return 1;
		} else if (gVerifyLocal == true) {
			VerifyLocal(mod_options);
			return 1;
		} else {
		}

		StartExport(mod_options);
	}
	
	return 1;
}

void SigHandler(int s) {

	if (gInterrupted == false) {
		gInterrupted = true;

		cout << "Ctrl-C received. Will finish current batch before exiting. Press Ctrl-C again to terminate immediately.\n";
	} else {
		cout << "Terminating immediately.\n";
		exit(1);
	}
}

int StartExport(map<string,string> options) {

	MetaDb* p_mdb = new MetaDb();

	if (gResume == MODE_NEW) {
		int rc = p_mdb->NewDb(options["working_dir"] + "/" + gJobID + ".db");

		if (rc == -1)
			return -1;

		p_mdb->InsertMeta("api_version", gAPIVersion);
		p_mdb->InsertMeta("cli_version", gVersion);
		p_mdb->InsertMeta("name", gExportName);
	}
	else {
		int rc = p_mdb->LoadDb(options["working_dir"] + "/" + gJobID + ".db");
		if (rc == 0) {
			LOG(ERROR) << "Could not open resume state";
		}
	}

	time_t seconds;

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
		LOG(ERROR) << "Unsupported output module specified: " << gOutputFormat;
	}

	gDatasetMeta = GetDatasetMetadata(gJobID);
	if (gDatasetMeta == NULL) {
		LOG(ERROR) << "Failed to fetch metadata for dataset";
		return -1;		
	}

	int batch_size = gDatasetMeta->count;

	// Initialize the writer module. This can fail in many ways including specifying
	// a non-supported combination of output fields
	if (p_writer->Initialize(gDatasetMeta, gResume) != 0) {
		LOG(ERROR) << "Failed to initialize " << gOutputFormat;
		return -1;
	}

	bool can_resume = p_writer->CanHandle("resume");

	if (!can_resume && gResume == true) {
		LOG(ERROR) << "Resume not supported by output module";
		return -1;
	}

	LOG(INFO) << "Exporting the following sets:";

	for (DatasetSet s : gDatasetMeta->sets) {
		LOG(INFO) << s.set_name << " (" << s.set_perc << "%)";
	}

	LOG(INFO) << "Using the following fields:";
	for (DatasetMapping* mapping : gDatasetMeta->mapping_by_id)	{
		LOG(INFO) << mapping->id << ": " << mapping->name;
	}

	for (int iter=0;iter<gIterations&&gInterrupted==false;iter++) {

		CurlReader *p_reader = new CurlReader();
		p_reader->SetNumThreads(gDownloadThreads);


		LOG(INFO) << "Iteration #" << to_string(iter+1);
		LOG(INFO) << "Total expected dataset size is " << to_string(batch_size);

		// Fetch basic stats on export
		int num_batches = ceil(batch_size / (float)gBatchSize);

		StartFeedThread(options, 0, iter);
		
		for (int batch_idx = 0; batch_idx < num_batches && gInterrupted == false; batch_idx++) {

			unsigned int queued_downloads = 0;

			vector<Metadata* > meta_data;

			feed_mutex.lock();
			if (batch_idx > 0 && feed_readahead.size() == 0) {
				LOG(DEBUG) << "Readahead feed empty.. waiting for data.";
			}
			if (gTerminateReadahead == true) {
				LOG(INFO) << "Reached end of feed but not all batches returned.";
				break;
			}

			feed_mutex.unlock();

			do {

				feed_mutex.lock();
				{
					// New data is ready
					if (feed_readahead.size() > 0) {
						meta_data = feed_readahead.front();
						feed_readahead.pop_front();
						feed_mutex.unlock();
						break;
					}
				}
				feed_mutex.unlock();

				// Wait for new data to come in
				usleep(100000);

			} while(1);

	//		vector<Metadata* > meta_data = FetchBatch(options, batch_idx);

			if (meta_data.size() == 0) {
				LOG(WARNING) << "No metadata returned by API, end of dataset?";
				return -1;
			}

			LOG(DEBUG) << "Starting download for batch #" << to_string(batch_idx);

			seconds = time(NULL);

			// Queue up all requests in this batch
			for (Metadata* m : meta_data) {

				if (!p_mdb->ContainsHash(m->hash, "api_hash")) {
					for (MetadataEntry* entry : m->entries) {

						if (entry->url != "") {
							p_reader->QueueUrl(md5(entry->url), entry->url);
							queued_downloads++;
						}
					}
				} else {
					m->skip_record = true;
					LOG(TRACE) << m->hash << " is already in hashes db";
				}
			}

			ReaderStats stats = p_reader->GetStats();

			// Loop until all downloads are finished
			while (stats.num_reads_completed < queued_downloads) {

				stats = p_reader->GetStats();

				if (time(NULL) != seconds) {
					DisplayProgressBar(stats.num_reads_completed / (float)queued_downloads, stats.num_reads_completed, queued_downloads);

					seconds = time(NULL);
				}

				usleep(100);
			}

			// Display the 100% complete
			DisplayProgressBar(1, stats.num_reads_completed, queued_downloads);
			cout << endl;

			// Update stats for last time
			stats = p_reader->GetStats();

			LOG(INFO) << "Downloaded " << to_string(stats.bytes_read) << " bytes";
			LOG(INFO) << "Syncing batch to disk...";

			p_reader->ClearStats();

			// Downloading finished, gather all cURL responses
			map<string, ReadRequest* > responses = p_reader->GetAllData();

			vector<Metadata* > meta_data_unpacked = UnpackMetadata(meta_data, responses);

			meta_data.clear();

			WriteMetadata(meta_data_unpacked, p_writer, p_mdb, options);

			p_reader->ClearData();

			// We are completely done with the response data
			for (auto& kv : responses) {
				delete kv.second;
			}

			responses.clear();
		}

		StopFeedThread();

		delete p_reader;
	}

	// Call finalize on the writer function
	p_writer->Finalize();

	return 0;
}

int GenerateMeanOnline(map<string,string> options) {

	time_t seconds;

	CurlReader *p_reader = new CurlReader();

	p_reader->SetNumThreads(gDownloadThreads);

	gDatasetMeta = GetDatasetMetadata(gJobID);
	if (gDatasetMeta == NULL) {
		LOG(ERROR) << "Failed to fetch metadata for dataset";
		return -1;		
	}

	gMean = new ImageMean(MEAN_GLOBAL);

	options["api_random"] = 1;

	StartFeedThread(options, 0, 0);
	
	int iteration = 1;

	while (gInterrupted == false && iteration < 4) {

		unsigned int queued_downloads = 0;

		vector<Metadata* > meta_data;

		do {

			feed_mutex.lock();
			{
				// New data is ready
				if (feed_readahead.size() > 0) {
					meta_data = feed_readahead.front();
					feed_readahead.pop_front();
					feed_mutex.unlock();
					break;
				}
			}
			feed_mutex.unlock();

			// Wait for new data to come in
			usleep(100000);

		} while(1);

		if (meta_data.size() == 0) {
			LOG(WARNING) << "No metadata returned by API, end of dataset?";
			return -1;
		}

		seconds = time(NULL);

		// Queue up all requests in this batch
		for (Metadata* m : meta_data) {

			for (MetadataEntry* entry : m->entries) {

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
				DisplayProgress("Iter #" + to_string(iteration) + " ", stats.num_reads_completed / (float)queued_downloads, stats.num_reads_completed, queued_downloads, "");

				seconds = time(NULL);
			}

			usleep(100);
		}

		// Display the 100% complete
		DisplayProgress("Iter #" + to_string(iteration) + " ", 1, stats.num_reads_completed, queued_downloads, "");
		cout << endl;

		// Update stats for last time
		stats = p_reader->GetStats();

		LOG(DEBUG) << "Downloaded " << to_string(stats.bytes_read) << " bytes";

		p_reader->ClearStats();

		// Downloading finished, gather all cURL responses
		map<string, ReadRequest* > responses = p_reader->GetAllData();

		vector<Metadata* > meta_data_unpacked = UnpackMetadata(meta_data, responses);

		bool bcontinue = GenerateImageMean(meta_data_unpacked, gMean, options);

		p_reader->ClearData();

		// We are completely done with the response data
		for (auto& kv : responses) {
			delete kv.second;
		}

		responses.clear();

		if (gGenerateMean == MEAN_GLOBAL)
			LOG(INFO) << "Global mean: " << gMean->GetGlobalMean();
		if (gGenerateMean == MEAN_CHANNEL) {
			vector<float> chan_mean = gMean->GetChannelMean();
			string means = "";
			for (unsigned int i=0; i<chan_mean.size(); i++) {
				means += " " + to_string((int)round(chan_mean[i])) + "(" + to_string(chan_mean[i]) + ")";
			}
			LOG(INFO) << "Channel mean:" << means;
		}

		iteration++;
	}

	StopFeedThread();

	return 0;
}

bool GenerateImageMean(vector<Metadata* >& meta_data, ImageMean* mean, map<string,string> options) {

	time_t seconds = time(NULL);

//	unsigned int to_write = meta_data.size();
	int cur_write = 0;

	// Iterate over the metadata a final time while calling the ImageMean module
	for (Metadata* m : meta_data) {

		if (time(NULL) != seconds) {
//			DisplayProgress("", cur_write / (float)to_write, cur_write, to_write, "");

			seconds = time(NULL);
		}

		for (MetadataEntry* e : m->entries) {
			if (e->value_type == METADATA_VALUE_TYPE_IMAGE || e->value_type == METADATA_VALUE_TYPE_RAW) {

				cv::Mat img;

				if (e->value_type == METADATA_VALUE_TYPE_IMAGE) {
					img = cv::imdecode(e->image_data, CV_LOAD_IMAGE_UNCHANGED);
				}
				else if (e->value_type == METADATA_VALUE_TYPE_RAW && e->dtype == "float") {
					img = cv::Mat(e->data_height, e->data_width, CV_32FC(e->data_channels), &e->float_raw_data[0]);
				}
				else if (e->value_type == METADATA_VALUE_TYPE_RAW && e->dtype == "uint8") {
					img = cv::Mat(e->data_height, e->data_width, CV_8UC(e->data_channels), &e->uint8_raw_data[0]);
				}

				mean->AddImage(img);
			} 
		}

		cur_write++;
	}

//	DisplayProgress("", 1, cur_write, to_write, "");
//	cout << endl;

	return true;
}

bool WriteMetadata(vector<Metadata* > meta_data, IDataWriter *p_writer, MetaDb* p_mdb, map<string,string> options) {

	time_t seconds = time(NULL);

	bool can_store_blobs = p_writer->CanHandle("blobs");
	int to_write = meta_data.size();
	int cur_write = 0;

	if (p_writer->BeginWriting() != 0) {
		LOG(ERROR) << "BeginWriting() failed ";
		return false;
	}

	// Iterate over the metadata a final time while calling the WriteData on the storage module
	for (Metadata* m : meta_data) {

		if (time(NULL) != seconds) {
			DisplayProgressBar(cur_write / (float)to_write, cur_write, to_write);

			seconds = time(NULL);
		}

		// Check if we need to subtract an image mean. This would change TYPE_IMAGE to TYPE_RAW
		if (gSubtractMean == true) {

			for (MetadataEntry* e : m->entries) {
				
				cv::Mat img;

				// Convert Image and Raw types to an opencv Mat
				if (e->value_type == METADATA_VALUE_TYPE_IMAGE) {
					img = cv::imdecode(e->image_data, CV_LOAD_IMAGE_UNCHANGED);
				}
				else if (e->value_type == METADATA_VALUE_TYPE_RAW && e->dtype == "float") {
					img = cv::Mat(e->data_height, e->data_width, CV_8UC(e->data_channels), (uint8_t*)&e->float_raw_data[0]);
				}
				else if (e->value_type == METADATA_VALUE_TYPE_RAW && e->dtype == "uint8") {
					img = cv::Mat(e->data_height, e->data_width, CV_32FC(e->data_channels), &e->uint8_raw_data[0]);
				}

				img = gMean->SubtractMean(img);
			}
		}

		// If the module doesnt support blobs or images-external is forced we first store
		// the data on disk and only pass the file_uri to the output modules
		if (options["images-external"] != "" || !can_store_blobs) {
			// Loop through individual fields to find any image/raw to store on disk
			for (MetadataEntry* e : m->entries) {
				if (e->value_type == METADATA_VALUE_TYPE_IMAGE || e->value_type == METADATA_VALUE_TYPE_RAW) {

					string file_uri = "";
					if (e->value_type == METADATA_VALUE_TYPE_IMAGE)
						file_uri = WriteImageData(e->filename, &e->image_data[0], e->image_data.size(), true);
					else if (e->value_type == METADATA_VALUE_TYPE_RAW && e->dtype == "float")
						file_uri = WriteImageData(e->filename, (uint8_t*)&e->float_raw_data[0], e->float_raw_data.size() * 4, true);
					else if (e->value_type == METADATA_VALUE_TYPE_RAW && e->dtype == "uint8")
						file_uri = WriteImageData(e->filename, &e->uint8_raw_data[0], e->uint8_raw_data.size(), true);
					
					// We were unable to write the image to disk, fail this record
					if (file_uri == "") {
						m->skip_record = true;
					}

					e->file_uri = file_uri;
				}
			}
		} else if( can_store_blobs ) {
			for (MetadataEntry* e : m->entries) {
				if (e->value_type == METADATA_VALUE_TYPE_IMAGE ) {
					cv::Mat img;
					img = cv::imdecode(e->image_data, CV_LOAD_IMAGE_UNCHANGED);
					if(img.isContinuous()){
						e->image_data.assign(img.datastart, img.dataend);
					}  else {
						for (int i = 0; i < img.rows; ++i) {
							e->image_data.insert(e->image_data.end(), img.ptr<uchar>(i), img.ptr<uchar>(i)+img.cols);
						}
					}
					// img.row(0).copyTo(e->float_raw_data);
				}
			}
		}

		if (!m->skip_record) {
			string writer_res = p_writer->WriteData(m);

			// Tokenize the output from the writer. Key=Value;Key=Value...
			map<string, string> keyval_map;

			for (const string& tag : split(writer_res, ';')) {
				auto key_val = split(tag, '=');
				keyval_map.insert(std::make_pair(key_val[0], key_val[1]));
			}

			string record_hash = keyval_map["hash"];
			if (record_hash == "") {
				LOG(ERROR) << "WriteData returned empty hash";
				return false;
			}

			// Store the filename in the meta db. 
			string file_name = keyval_map["file"];
			int file_id = 0;

			if (file_name != "") {
				// Generate a fileid or return an existing one
				file_id = p_mdb->GetFileId(file_name);
			}

			// Insert the API and record hash into the meta db
			p_mdb->InsertHash(file_id, m->hash, record_hash);
		}

		cur_write++;
	}

	if (p_writer->EndWriting() != 0) {
		LOG(ERROR) << "EndWriting() failed ";
		return false;
	}

	DisplayProgressBar(1, cur_write, to_write);
	cout << endl;

	return true;
}

vector<Metadata* > UnpackMetadata(vector<Metadata* >& meta_data, map<string, ReadRequest* >& responses) {

	// Same as the original meta_data vector except archives have been unpacked
	// and their 'archive' entries removed. This all makes sure archives get 
	// expanded in the same place inside the vector
	vector<Metadata* > meta_data_unpacked;

	for (Metadata* m : meta_data) {
		// Find the request for a specific filename

		if (m->skip_record)
			continue;

		vector<MetadataEntry* > new_entries;

		for (MetadataEntry* entry : m->entries) {

			bool skip_meta_entry = false;

			if (entry->url != "") {	// Did we download something ?
				ReadRequest* req = responses[md5(entry->url)];

				if (entry->value_type == METADATA_VALUE_TYPE_IMAGE || entry->value_type == METADATA_VALUE_TYPE_RAW) {
					int r = ReadRequestToMetadataEntry(req, entry);

					if (r == -1)
						return meta_data_unpacked;

				} else if (entry->value_type == METADATA_VALUE_TYPE_ARCHIVE) {
					// Archive found, start unpacking
					struct archive_entry* a_entry;
					struct archive* ar = archive_read_new();
					archive_read_support_filter_all(ar);
					archive_read_support_format_all(ar);

					int r = archive_read_open_memory(ar, &req->read_data[0], req->read_data.size());
					if (r == ARCHIVE_FATAL) {
						LOG(DEBUG) << "Skipping broken archive at: " << entry->url;

						archive_read_close(ar);
						archive_read_free(ar);

						skip_meta_entry = true;
						m->skip_record = true;	// Skip this record
						continue;
					}

					// Save the file_id.diz for the end so we know all images have been loaded
					struct ReadRequest* file_id = NULL;

					// Go over all the files inside the archive
					while (archive_read_next_header(ar, &a_entry) == ARCHIVE_OK) {

						const uint8_t* buff;
						size_t size;
						off_t offset;

						string file_name(archive_entry_pathname(a_entry));

						struct ReadRequest* new_req = new ReadRequest();

						new_req->url = entry->url;
						new_req->id = "";
						new_req->status = 200;

						// Read all data for the entry retrieved aboved
						for (;;) {
							r = archive_read_data_block(ar, (const void **)&buff, &size, &offset);

							if (r == ARCHIVE_EOF) {
								if (file_name == "file_id.diz") {
									// Save this for later, we still need to extract more images
									file_id = new_req;

								} else {
									// Store file as if we got it from cURL
									responses[md5(file_name)] = new_req;
								}
								break;
							}

							// Insert data in the request buffer
							new_req->read_data.insert(new_req->read_data.end(),&buff[0],&buff[size]);
						}
					}

					if (file_id == NULL) {
						LOG(ERROR) << entry->url << " did not contain a file_id.diz";

						skip_meta_entry = true;
						m->skip_record = true;	// Skip this record

						continue;
					}

					// Make sure we NULL terminate the text file
					file_id->read_data.push_back('\0');

					skip_meta_entry = true;

					// Parse the Metadata records contained in this file. Entries here link
					// to the download through their 'filename'. Entries with the same 'id'
					// are merged as entries in a Metadata struct
					vector<MetadataEntry* > tar_meta = ParseTarFeed((const char* )&file_id->read_data[0]);

					for (MetadataEntry* tar_entry : tar_meta) {
						// Assign the downloaded data
						ReadRequest* tar_data = responses[md5(tar_entry->filename)];

						int r = ReadRequestToMetadataEntry(tar_data, tar_entry);
						if (r == -1)	// Failed
							return meta_data_unpacked;
					}

					// Delete the TAR file entry
					m->entries.erase(remove(m->entries.begin(), m->entries.end(), entry),m->entries.end());

					// Iterate all converted tar entries and add as new records
					for (MetadataEntry* tar_entry : tar_meta) {

						Metadata* new_record = new Metadata();
						new_record->setname = m->setname;
						new_record->hash = m->hash;

						// Insert the main fields that were supplied in the field						
						new_record->entries.insert(new_record->entries.end(), m->entries.begin(), m->entries.end());
						// Insert the unpacked image
						new_record->entries.push_back(tar_entry);
/*
						for (MetadataEntry* e : new_record->entries) {
							cout << "id: " << e->id << " fieldid: " << e->field_id << " metatype: " << e->meta_type << " valuetype: " << e->value_type << endl;
						}

						cout << endl;
*/
						// Add entry as a new reocrd
						meta_data_unpacked.push_back(new_record);
					}

					// We dont need to safe this download
					delete file_id;

					archive_read_close(ar);
					archive_read_free(ar);

				} else {
					LOG(ERROR) << "Encountered download for unsupported source METADATA_VALUE_TYPE: " << entry->value_type;
					return meta_data_unpacked;
				}
			}

//			if (!skip_meta_entry)
//				new_entries.push_back(entry);

		}	// for (MetaDataEntry* entry : m->entries)
/*
		if (!m->skip_record) {
			m->entries = new_entries;
			meta_data_unpacked.push_back(m);
		}
*/
	}	// for (Metadata* m : meta_data)

	return meta_data_unpacked;
}

int VerifyLocal(map<string,string> options) {

	LOG(INFO) << "Starting local dataset verification";

	MetaDb* p_mdb = new MetaDb();

	int rc = p_mdb->LoadDb(options["working_dir"] + "/" + gJobID + ".db");
	if (rc == 0) {
		LOG(ERROR) << "Could not open resume state";
		return -1;
	}

	gDatasetMeta = GetDatasetMetadata(gJobID);
	if (gDatasetMeta == NULL) {
		LOG(ERROR) << "Failed to fetch metadata for dataset";
		return -1;		
	}

	vector<string> files = p_mdb->GetFileList();

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
		LOG(ERROR) << "Unsupported output module specified: " << gOutputFormat;
	}

	int found_in_metadb = 0;		// Hash records in the storage output that match with the Meta Db
	int not_found_in_metadb = 0;	// Hash records that are not found in the Meta Db

	if (p_writer->Initialize(gDatasetMeta, MODE_VERIFY) != 0) {
		LOG(ERROR) << "Failed to initialize " << gOutputFormat;
		return -1;
	}

	for (string file : files) {
		LOG(INFO) << file;

		string result = "";

		do {
			// Tokenize the output from the writer. Key=Value;Key=Value...
			map<string, string> keyval_map;

			string writer_res = p_writer->CheckIntegrity(file);

			for (const string& tag : split(writer_res, ';')) {
				auto key_val = split(tag, '=');
				keyval_map.insert(std::make_pair(key_val[0], key_val[1]));
			}

			string record_hash = keyval_map["hash"];
			result = keyval_map["result"];

			if (record_hash != "") {
				LOG(TRACE) << "Checking for record hash " << record_hash;

				if (p_mdb->ContainsHash(record_hash, "record_hash")) {
					found_in_metadb++;
				} else {
					not_found_in_metadb++;
				}
			} else if(result != "eof"){
				LOG(ERROR) << "CheckIntegrity did not return a hash or EOF";
				break;
			}

		} while (result != "eof");
	}

	LOG(INFO) << "Records found in Meta Db: " << found_in_metadb;
	LOG(INFO) << "Records not found in Meta Db: " << not_found_in_metadb;

	return 0;
}

int VerifyApi(map<string,string> options) {

	LOG(INFO) << "Starting dataset verification with API";

	MetaDb* p_mdb = new MetaDb();

	int rc = p_mdb->LoadDb(options["working_dir"] + "/" + gJobID + ".db");
	if (rc == 0) {
		LOG(ERROR) << "Could not open resume state";
		return -1;
	}

	gDatasetMeta = GetDatasetMetadata(gJobID);
	if (gDatasetMeta == NULL) {
		LOG(ERROR) << "Failed to fetch metadata for dataset";
		return -1;		
	}

	time_t seconds = time(NULL);

	int batch_size = gDatasetMeta->count;

	LOG(INFO) << "Total expected dataset size is " << to_string(batch_size);

	// Fetch basic stats on export
	int num_batches = ceil(batch_size / (float)gBatchSize);

	int found_local = 0;		// Hash records returned by the API that are found locally
	int not_found_local = 0;	// Hash records that are not found locally
	int not_seen_remote = 0;	// Hash records stored locally but not returned by API

	int batch_idx = 0;

	for (;batch_idx < num_batches && gInterrupted == false; batch_idx++) {

		vector<Metadata* > meta_data = FetchBatch(options, batch_idx, 0);

		if (meta_data.size() == 0) {
			LOG(WARNING) << "No metadata returned by API, end of dataset?";
			break;
		}

		for (Metadata* m : meta_data) {

			if (p_mdb->ContainsHash(m->hash, "api_hash")) {
				found_local++;
			} else {
				not_found_local++;
			}
		}

		if (time(NULL) != seconds) {
			DisplayProgressBar(batch_idx / (float)num_batches, batch_idx, num_batches);

			seconds = time(NULL);
		}
	}

	DisplayProgressBar(1, batch_idx, num_batches);
	cout << endl;

	LOG(INFO) << "Found locally: " << found_local;
	LOG(INFO) << "Not found locally: " << not_found_local;
	LOG(INFO) << "Not seen remote: " << not_seen_remote;

	return 0;
}

int ReadRequestToMetadataEntry(ReadRequest* req, MetadataEntry* entry) {

	if (req != NULL && req->read_data.size() > 0) {

		// We found the download for a piece of metadata
		if (entry->value_type == METADATA_VALUE_TYPE_IMAGE) {
			entry->image_data = req->read_data;

		} else if (entry->value_type == METADATA_VALUE_TYPE_RAW) {
			if (entry->dtype == "uint8")
				entry->uint8_raw_data = req->read_data;
			else if (entry->dtype == "float") {

				unsigned int rsize = req->read_data.size() / 4;
				if (rsize * 4 != req->read_data.size()) {
					LOG(ERROR) << "Raw data with dtype float is not divisible by 4. Is the data in float format?";
				}

				for (unsigned int ridx = 0; ridx < rsize; ridx++) {
					entry->float_raw_data.push_back(((float *)&req->read_data)[ridx]);								
				}
			} else {
				LOG(ERROR) << "Unsupported dtype for METADATA_TYPE_RAW: " << entry->dtype;
				return -1;
			}
		}
	} else {
		LOG(ERROR) << entry->filename + " was not downloaded?";
		return -1;
	}

	return 0;
}

void ConfigureLogger() {

	el::Configurations config;

	config.set(el::Level::Global, el::ConfigurationType::Format, "%datetime %level %loc] %msg");

	if (gLogLevel == el::Level::Debug)
		config.set(el::Level::Trace, el::ConfigurationType::Enabled, "false");
	else if (gLogLevel == el::Level::Info)
	{
		config.set(el::Level::Trace, el::ConfigurationType::Enabled, "false");
		config.set(el::Level::Debug, el::ConfigurationType::Enabled, "false");
	}
	else if (gLogLevel == el::Level::Warning)
	{
		config.set(el::Level::Trace, el::ConfigurationType::Enabled, "false");
		config.set(el::Level::Debug, el::ConfigurationType::Enabled, "false");
		config.set(el::Level::Info, el::ConfigurationType::Enabled, "false");
	}
	else if (gLogLevel == el::Level::Error)
	{
		config.set(el::Level::Trace, el::ConfigurationType::Enabled, "false");
		config.set(el::Level::Debug, el::ConfigurationType::Enabled, "false");
		config.set(el::Level::Info, el::ConfigurationType::Enabled, "false");
		config.set(el::Level::Warning, el::ConfigurationType::Enabled, "false");
	}
	else if (gLogLevel == el::Level::Fatal)
	{
		config.set(el::Level::Trace, el::ConfigurationType::Enabled, "false");
		config.set(el::Level::Debug, el::ConfigurationType::Enabled, "false");
		config.set(el::Level::Info, el::ConfigurationType::Enabled, "false");
		config.set(el::Level::Warning, el::ConfigurationType::Enabled, "false");
		config.set(el::Level::Error, el::ConfigurationType::Enabled, "false");
	}
	else if (gLogLevel == el::Level::Unknown)
	{
		config.set(el::Level::Trace, el::ConfigurationType::Enabled, "false");
		config.set(el::Level::Debug, el::ConfigurationType::Enabled, "false");
		config.set(el::Level::Info, el::ConfigurationType::Enabled, "false");
		config.set(el::Level::Warning, el::ConfigurationType::Enabled, "false");
		config.set(el::Level::Error, el::ConfigurationType::Enabled, "false");
		config.set(el::Level::Fatal, el::ConfigurationType::Enabled, "false");
	}

	el::Loggers::reconfigureAllLoggers(config);
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

void DisplayProgress(string prefix, float progress, int cur_value, int max_value, string suffix) {
	
	cout << prefix << round(100 * progress) << "% (" << cur_value << "/" << max_value << ")" << suffix << "\r";

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

void split(const std::string &s, char delim, std::vector<std::string> &elems) {
    std::stringstream ss;
    ss.str(s);
    std::string item;
    while (std::getline(ss, item, delim)) {
        elems.push_back(item);
    }
}

std::vector<std::string> split(const std::string &s, char delim) {
    std::vector<std::string> elems;
    split(s, delim, elems);
    return elems;
}