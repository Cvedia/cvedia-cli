#ifndef _DATAWRITER_HPP
#define _DATAWRITER_HPP

struct WriteRequest {

	string id;
	string filename;

	string category;

	int type;	// 0 = Training, 1 = Test

	vector<uint8_t> write_data;
	
};

struct WriterStats {

	long bytes_written;
};

class IDataWriter {

public:
	virtual ~IDataWriter() {};

	virtual int WriteData(WriteRequest* req) = 0;

	virtual WriterStats GetStats() = 0;
	virtual void ClearStats() = 0;

private:
	virtual bool ValidateRequest(WriteRequest* req) = 0;
};

#endif