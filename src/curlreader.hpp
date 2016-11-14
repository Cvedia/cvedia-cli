#ifndef _CURLREADER_HPP
#define _CURLREADER_HPP

using namespace std;

#include "datareader.hpp"

class CurlReader: public IDataReader {

public:
	CurlReader();
	~CurlReader();

	ReaderStats GetStats();
	void RequestUrl(string url);

private:
	void WorkerThread();

	static size_t WriteCallback(void *data, size_t size, size_t nmemb, void *userp);

	deque<ReadRequest* > mRequests;
	deque<ReadRequest* > mResponses;

	mutex mResponsesMutex;

	mutex mRequestsMutex;
	mutex mNewRequestsMutex;

	CURLM* mMultiHandle;

	int mThreadsRunning;
	int mThreadsMax;

	ReaderStats mCurlStats;

	thread mWorkerThread;
};

#endif
