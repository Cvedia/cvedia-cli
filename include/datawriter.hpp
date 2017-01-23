#ifndef _DATAWRITER_HPP
#define _DATAWRITER_HPP

#define DATA_TRAIN		0
#define DATA_TEST		1
#define DATA_VALIDATE	2

#define MODE_NEW		0
#define MODE_RESUME		1
#define MODE_VERIFY		2

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

	virtual int Initialize(DatasetMetadata* dataset_meta, int mode) = 0;

	virtual bool CanHandle(string support) = 0;

	virtual int BeginWriting() = 0;
	virtual string WriteData(Metadata* meta) = 0;
	virtual int EndWriting() = 0;

	virtual string CheckIntegrity(string file_name) = 0;

	virtual int Finalize() = 0;

	virtual WriterStats GetStats() = 0;
	virtual void ClearStats() = 0;

};

#endif