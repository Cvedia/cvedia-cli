#ifndef _DATAREADER_HPP
#define _DATAREADER_HPP

struct ReadRequest {

	string url;
	string id;
	int retry_count;
	vector<uint8_t> read_data;

	int status;
};

struct ReaderStats {

	unsigned int num_reads_success;
	unsigned int num_reads_empty;
	unsigned int num_reads_error;
	unsigned int num_reads_completed;	// Total of the above

	long bytes_read;
};

class IDataReader {

public:
	virtual ~IDataReader() {};

	virtual ReaderStats GetStats() = 0;
	virtual void ClearStats() = 0;
	virtual void SetNumThreads(int num_threads) = 0;
	virtual void QueueUrl(string id, string url) = 0;
};


#endif