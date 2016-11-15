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

#include "cvedia.hpp"
#include "csvwriter.hpp"

CsvWriter::CsvWriter() {

	WriteDebugLog("Initializing CsvWriter");

	// Clear the stats structure
	mCsvStats = {};
}

CsvWriter::~CsvWriter() {
}

WriterStats CsvWriter::GetStats() {

	return mCsvStats;
}

void CsvWriter::ClearStats() {

	// Clear the stats structure
	mCsvStats = {};
}

bool CsvWriter::ValidateRequest(WriteRequest* req) {

	return true;
}

int CsvWriter::WriteData(WriteRequest* req) {

	return 0;
}