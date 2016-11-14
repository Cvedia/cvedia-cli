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
};

class IDataReader {

public:
	virtual ~IDataReader() {};

	virtual ReaderStats GetStats() = 0;
	virtual void RequestUrl(string url) = 0;
};


#endif