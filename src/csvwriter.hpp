#ifndef _CSVWRITER_HPP
#define _CSVWRITER_HPP

using namespace std;

#include "datawriter.hpp"

class CsvWriter: public IDataWriter {

public:
	CsvWriter();
	~CsvWriter();

	int WriteData(WriteRequest* req);

	WriterStats GetStats();
	void ClearStats();
	
private:

	virtual bool ValidateRequest(WriteRequest* req);

	deque<WriteRequest* > mRequests;

	WriterStats mCsvStats;
};

#endif
