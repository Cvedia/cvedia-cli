#ifndef _CAFFEIMAGEDATA_HPP
#define _CAFFEIMAGEDATA_HPP

using namespace std;

#include "csvwriter.hpp"
#include "api.hpp"

class CaffeImageDataWriter: public CsvWriter {

public:
	CaffeImageDataWriter(string export_name, map<string, string> options);

	int Finalize();

private:

	bool ValidateData(vector<Metadata* > meta);
	string PrepareData(Metadata* meta);

	map<string, int> mCategoryLookup;
	int mLastCategoryId = {};
};

#endif
