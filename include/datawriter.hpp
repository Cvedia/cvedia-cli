#ifndef _DATAWRITER_HPP
#define _DATAWRITER_HPP

#define DATA_TRAIN		0
#define DATA_TEST		1
#define DATA_VALIDATE	2

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

	virtual string WriteData(Metadata* meta) = 0;

	virtual int BeginWriting(DatasetMetadata* dataset_meta) = 0;
	virtual int EndWriting(DatasetMetadata* dataset_meta) = 0;
	virtual bool ValidateData(vector<Metadata* > meta) = 0;
	virtual string VerifyData(string file_name, DatasetMetadata* dataset_meta) = 0;
	
	virtual int Initialize(DatasetMetadata* dataset_meta, bool resume) = 0;
	virtual int Finalize() = 0;

	virtual WriterStats GetStats() = 0;
	virtual void ClearStats() = 0;

};

#endif