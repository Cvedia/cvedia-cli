#ifndef _DATAREADER_HPP
#define _DATAREADER_HPP

struct ReadRequest {

	string url;
	string id;
	vector<uint8_t> read_data;

	int status;
};

struct ReaderStats {

	int num_reads_success;
	int num_reads_empty;
	int num_reads_error;
	int num_reads_completed;	// Total of the above

	long bytes_read;
};

class IDataReader {

public:
	virtual ~IDataReader() {};

	virtual ReaderStats GetStats() = 0;
	virtual void ClearStats() = 0;
	virtual void SetNumThreads(int num_threads) = 0;
	virtual ReadRequest* RequestUrl(string url) = 0;
	virtual void QueueUrl(string id, string url) = 0;
};


#endif