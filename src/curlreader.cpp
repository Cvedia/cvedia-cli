#include <cstring>
#include <cstdlib>
#include <deque>
#include <map>
#include <vector>
#include <string>
#include <iostream>
#include <sstream>
#include <deque>
#include <mutex>
#include <thread>
#include <condition_variable>
#include <getopt.h>
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

using namespace std;

#include "easylogging++.h"
#include "cvedia.hpp"
#include "curlreader.hpp"

CurlReader::CurlReader() {

	LOG(INFO) << "Initializing CurlReader";

	mThreadsMax = 100;

	// Initialize a multi stack
	mMultiHandle = curl_multi_init();

	// Clear the stats structure
	mCurlStats = {};

	mWorkerThread = thread(&CurlReader::WorkerThread, this);
}

CurlReader::~CurlReader() {
}

ReadRequest* CurlReader::RequestUrl(string url) {

	ReadRequest* req = new ReadRequest;

	req->url = url;

	// Setup curl object
	CURL *curl_handle = curl_easy_init();
	curl_easy_setopt(curl_handle, CURLOPT_URL, req->url.c_str());
	curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, CurlReader::WriteCallback);
	curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, (void *)req);
	curl_easy_setopt(curl_handle, CURLOPT_PRIVATE, (void *)req);
	curl_easy_setopt(curl_handle, CURLOPT_TIMEOUT, 20L);

	// Execute a single synchronous fetch
	int err = curl_easy_perform(curl_handle);
	if (!err) {

		req->status = 0;

		curl_easy_cleanup(curl_handle);

		return req;
	} else {
		LOG(ERROR) << "Could not communicate with API: " << url;
	}

	// Fail
	curl_easy_cleanup(curl_handle);

	return NULL;
}

void CurlReader::QueueUrl(string id, string url) {

	ReadRequest* request = new ReadRequest;

	if (id == "") {
		LOG(ERROR) << "Empty id passed to QueueUrl";
		return;
	}

	request->id = id;
	request->url = url;

	mRequestsMutex.lock();
	{
		// Add request to the queue
		mRequests.push_back(request);
	}
	mRequestsMutex.unlock();

	mRequestsAdded++;
}

ReaderStats CurlReader::GetStats() {

	return mCurlStats;
}

void CurlReader::ClearData() {

	mRequests.clear();
	mResponses.clear();
}

map<string, ReadRequest* > CurlReader::GetAllData() {

	return mResponses;
}

void CurlReader::ClearStats() {

	// Clear the stats structure
	mCurlStats = {};
}

void CurlReader::SetNumThreads(int num_threads) {

	LOG(INFO) << "Number of download threads set to " << to_string(num_threads);
	mThreadsMax = num_threads;
}

size_t CurlReader::WriteCallback(void *data, size_t size, size_t nmemb, void *userp) {

	size_t realsize = size * nmemb;

	// Cast user pointer to request structure
	ReadRequest* req = (ReadRequest* )userp;

	// Get size of currently allocated data
	size_t data_size = req->read_data.size();

	// Resize vector to accomodate new data	
	req->read_data.resize(data_size + realsize);

	// Copy new data into request
	memcpy(&(req->read_data[data_size]), data, realsize);

	return realsize;
}

void CurlReader::WorkerThread() {
	
	unique_lock<mutex> lck(mNewRequestsMutex);
	
	do {
		// Get the requests mutex
		mRequestsMutex.lock();
		{
			int req_added = 0;
			
			// Add more work to CURL as long as we're not exceeding the maximum thread count
			while (!mRequests.empty() && req_added < (mThreadsMax - mThreadsRunning)) {
				
				ReadRequest* req = mRequests.front();
				mRequests.pop_front();
				
				CURL *curl_handle = curl_easy_init();
				curl_easy_setopt(curl_handle, CURLOPT_URL, req->url.c_str());
				curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, CurlReader::WriteCallback);
				curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, (void *)req);
				curl_easy_setopt(curl_handle, CURLOPT_PRIVATE, (void *)req);
				curl_easy_setopt(curl_handle, CURLOPT_TIMEOUT, 20L);
				
				curl_multi_add_handle(mMultiHandle, curl_handle);
				 
				req_added++;
			}
		}
		mRequestsMutex.unlock();
		
		curl_multi_perform(mMultiHandle, &mThreadsRunning);
		
		struct timeval timeout;
		int rc; // select() return code
		CURLMcode mc; // curl_multi_fdset() return code

		fd_set fdread;
		fd_set fdwrite;
		fd_set fdexcep;

		int maxfd = -1;

		long curl_timeo = -1;

		FD_ZERO(&fdread);
		FD_ZERO(&fdwrite);
		FD_ZERO(&fdexcep);

		// set a suitable timeout to play around with
		timeout.tv_sec = 1;
		timeout.tv_usec = 0;

		curl_multi_timeout(mMultiHandle, &curl_timeo);
		
		if (curl_timeo >= 0) {
			timeout.tv_sec = curl_timeo / 1000;
			if(timeout.tv_sec > 1)
				timeout.tv_sec = 1;
			else
				timeout.tv_usec = (curl_timeo % 1000) * 1000;
		}

		// get file descriptors from the transfers 
		mc = curl_multi_fdset(mMultiHandle, &fdread, &fdwrite, &fdexcep, &maxfd);
	
		if (mc != CURLM_OK)
		{
			LOG(ERROR) << "curl_multi_fdset() failed, code " << to_string(mc);
			break;
		}
		
		if(maxfd == -1) {
			struct timeval wait = { 0, 100 * 1000 }; // 100ms
			rc = select(0, NULL, NULL, NULL, &wait);
		}
		else {
			rc = select(maxfd+1, &fdread, &fdwrite, &fdexcep, &timeout);
		}

		CURLMsg* p_msg;
		int msgs_left;
		
		// See how the transfers went
		while ((p_msg = curl_multi_info_read(mMultiHandle, &msgs_left))) {

			ReadRequest* req;

			curl_easy_getinfo(p_msg->easy_handle, CURLINFO_PRIVATE, &req);
			
			if (p_msg->msg == CURLMSG_DONE) {
				
				if (req->read_data.size() > 0) {

					req->status = 0;

					mCurlStats.bytes_read += req->read_data.size();

					mCurlStats.num_reads_success++;
				} else {
					req->status = -1;

					mCurlStats.num_reads_empty++;
				}
			} else {
				req->status = -1;

				LOG(ERROR) << "Error downloading file" << endl;
				mCurlStats.num_reads_error++;
			}
			
			mResponsesMutex.lock();
			{
				mResponses[req->id] = req;
			}
			mResponsesMutex.unlock();

			mCurlStats.num_reads_completed++;

			curl_multi_remove_handle(mMultiHandle, p_msg->easy_handle);
			curl_easy_cleanup(p_msg->easy_handle);
		}      
	} while (1);
}
