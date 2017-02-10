#include <deque>
#include <map>
#include <vector>
#include <string>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdio.h>
#include <errno.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <dlfcn.h>

using namespace std;

#include "config.hpp"

#ifdef HAVE_HDF5

#include "md5.hpp"
#include "easylogging++.h"
#include "cvedia.hpp"
#include "api.hpp"
#include "hdf5writer.hpp"
#include "H5Cpp.h"

using namespace H5;


Hdf5Writer::Hdf5Writer(string export_name, map<string, string> options) {

	LOG(DEBUG) << "Initializing Hdf5Writer";

//	H5::Exception::dontPrint();

	mModuleOptions = options;
	mExportName = export_name;

	if (mModuleOptions["base_dir"] != "") {
		mBaseDir = mModuleOptions["base_dir"];
	} else {
		mBaseDir = "";
	}

	mBasePath = mBaseDir;

	if (mBasePath == "")
		mBasePath = "./";
	else if (mBasePath != "./")
		mBasePath += "/";


	mBasePath += mExportName + "/";
}

Hdf5Writer::~Hdf5Writer() {

	if (mTrainFile.is_open()) {
		mTrainFile.close();
		if (mH5File[METADATA_TRAIN] != NULL) {
			mH5File[METADATA_TRAIN]->close();
			delete mH5File[METADATA_TRAIN];
		}
	}
	if (mTestFile.is_open()) {
		mTestFile.close();
		if (mH5File[METADATA_TEST] != NULL) {
			mH5File[METADATA_TEST]->close();
			delete mH5File[METADATA_TEST];
		}
	}
	if (mValidateFile.is_open()) {
		mValidateFile.close();
		if (mH5File[METADATA_VALIDATE] != NULL) {
			mH5File[METADATA_VALIDATE]->close();
			delete mH5File[METADATA_VALIDATE];
		}
	}
}

WriterStats Hdf5Writer::GetStats() {

	return mCsvStats;
}

void Hdf5Writer::ClearStats() {

	// Clear the stats structure
	mCsvStats = {};
}

bool Hdf5Writer::CanHandle(string support) {

	if (support == "resume")
		return true;
	if (support == "integrity")
		return true;
	if (support == "blobs")
		return true;

	return false;
}


int Hdf5Writer::Initialize(DatasetMetadata* dataset_meta, int mode) {
	mDatasetMetadata = dataset_meta;

	// Clear the stats structure
	mCsvStats = {};

	if (mode == MODE_NEW) {

		int dir_err = mkdir(mBasePath.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
		if (dir_err == -1 && errno != EEXIST) {
			LOG(ERROR) << string("Could not create directory: " + mBasePath).c_str();
			return -1;
		}
	}

	return 0;
}

bool Hdf5Writer::ValidateData(vector<Metadata* > meta) {
	// Only perform initialization once
	if (!mInitialized) {


		Metadata *m = meta[0];
		DSetCreatPropList prop;


		for (MetadataEntry* e : m->entries) {

			int dataDim = 0;

			// Start the actual write
			if (e->value_type == METADATA_VALUE_TYPE_RAW) {
				if (e->dtype == "uint8")
					dataDim = e->uint8_raw_data.size();
				else if (e->dtype == "float")
					dataDim = e->float_raw_data.size();
				else {
					LOG(ERROR) << string("Unsupported dtype specified for METADATA_VALUE_TYPE_RAW: " + e->dtype).c_str();
					return false;
				}
			} else if (e->value_type == METADATA_VALUE_TYPE_NUMERIC) {
				if (e->dtype == "int")
					dataDim = 1;
				else if (e->dtype == "float")
					dataDim = 1;
				else {
					LOG(ERROR) << string("Unsupported dtype specified for METADATA_VALUE_TYPE_NUMERIC: " + e->dtype).c_str();
					return false;
				}
			} else {
				LOG(ERROR) << string("Unsupported value type specified: " + e->value_type).c_str();
				return false;
			}

			// We start with 0 entries and can scale endlessly
			hsize_t dims[2] 		= {0, dataDim};	
			hsize_t maxdims[2] 	= {H5S_UNLIMITED, dataDim};
			hsize_t chunk_dims[2] = {2, dataDim};
			mSpace = new DataSpace(2, dims, maxdims);
			prop.setChunk(2, chunk_dims);

			// Create the chunked dataset.  Note the use of pointer.
			for(auto const& file : mH5File) {
				mDataSet[file.first] 	= new DataSet(mH5File[file.first]->createDataSet(m->setname, (e->dtype == "uint8" ? PredType::NATIVE_UCHAR : PredType::NATIVE_FLOAT), *mSpace, prop));
			}
			LOG(DEBUG) << string("mDataDim = " + to_string(dataDim));

		}

		mInitialized = true;
	}

	return true;
}


string Hdf5Writer::PrepareData(Metadata* meta) {

	string file_format = "$FILENAME $CATEGORY\n";

	string output_line = file_format;

	return output_line;
}

int Hdf5Writer::Finalize() {

	return 0;
}

int Hdf5Writer::BeginWriting() {
		// Open all output sets on disk in append mode
	for (DatasetSet s : mDatasetMetadata->sets) {
		LOG(INFO) << mBasePath;
		LOG(INFO) << s.set_name;
		mSetFile[s.set_name].open(mBasePath + s.set_name + ".txt", ios::out | ios::app);
		if (!mSetFile[s.set_name].is_open()) {
			LOG(ERROR) << "Failed to open: " << mBasePath << s.set_name << ".txt";
			return -1;			
		}		

		try {
			mH5File[s.set_name] = new H5File(mBasePath + s.set_name + ".h5", H5F_ACC_TRUNC);
			mSetFile[s.set_name] << mBasePath << s.set_name +".h5" << endl;
		} catch (H5::Exception e) {
			LOG(ERROR) << "Failed to create: " + mBasePath + s.set_name + ".h5";
			LOG(ERROR) << e.getCDetailMsg();
			return -1;						
		}

	}

	return 0;
}


string Hdf5Writer::WriteData(Metadata* meta) {
	LOG(INFO) << "WriteData";

	if (!mInitialized) {
		LOG(ERROR) << "Must call Initialize() first";
		return "";
	}

	if(!mSetFile.count(meta->setname)){
		LOG(ERROR) << "Set file does not exist " << meta->setname;
	}

	void *ptr = NULL;
	string dtype;

	for (MetadataEntry* e : meta->entries) {

		void *tmp_ptr = NULL;
		if (e->value_type == METADATA_VALUE_TYPE_IMAGE || e->value_type == METADATA_VALUE_TYPE_RAW) {
			if (e->dtype == "uint8")
				tmp_ptr = &e->uint8_raw_data[0];
			else if (e->dtype == "float")
				tmp_ptr = &e->float_raw_data[0];
			// else {
			// 	e->dtype = "uint8";
			// 	tmp_ptr = &e->image_data[0];
			// }
		} else if (e->value_type == METADATA_VALUE_TYPE_NUMERIC) {

			if (e->dtype == "int")
				tmp_ptr = &e->int_value;
			else if (e->dtype == "float")
				tmp_ptr = &e->float_value;		
		}
		LOG(INFO) << "ValueType: " << e->value_type;

		dtype = e->dtype;
		ptr = tmp_ptr;

		// mDataSet[meta->setname][e->id] = 

	}

	// Start the actual write
	LOG(INFO) << "Meta setname: " << meta->setname;
	AppendEntry(mDataSet[meta->setname], ptr, dataDim, ConvertDtype(dtype));
	
	//must return md5 here
	return "";
}

int Hdf5Writer::EndWriting() {
	// Close all open files
	for (DatasetSet s : mDatasetMetadata->sets) {
		mSetFile[s.set_name].close();
	}

	return 0;
}


const DataType& Hdf5Writer::ConvertDtype(string dtype) {
	LOG(INFO) << "ConvertDtype: " << dtype;

	if (dtype == "uint8")
		return PredType::NATIVE_UCHAR;
	if (dtype == "float")
		return PredType::NATIVE_FLOAT;
	if (dtype == "int")
		return PredType::NATIVE_INT32;
	if (dtype == "")
		return PredType::NATIVE_UCHAR;
	LOG(ERROR) << string("Unsupported dtype passed for conversion: " + dtype).c_str();
	return PredType::NATIVE_UCHAR;
}

void Hdf5Writer::AppendEntry(DataSet* dataset, void* data_ptr, hsize_t data_size, const DataType& dtype) {
	LOG(INFO) << "HDF5 Append entry";

	// Get current extent and increase by 1
	DataSpace tmp_space = dataset->getSpace();
	LOG(INFO) << "HDF5 Append entry0";

	int num_dims = tmp_space.getSimpleExtentNdims();

	hsize_t sp_size[num_dims];
	LOG(INFO) << "HDF5 Append entry1";

	tmp_space.getSimpleExtentDims(sp_size, NULL);

	hsize_t new_dim[2] 	= {sp_size[0]+1, data_size};
	LOG(INFO) << "HDF5 Append entry2";

	// Extend the DataSet with 1 record
	dataset->extend(new_dim);

	// Find the end of the file and append new data. 
	DataSpace filespace = dataset->getSpace();

	// Define the hyperslab position, this is where we write to
	hsize_t offset[2] = {sp_size[0], 0};
	// We want to write a single entry 
	hsize_t startdim[2] = {1, data_size};
	LOG(INFO) << "HDF5 Append entry3";

	filespace.selectHyperslab(H5S_SELECT_SET, startdim, offset);
	
	DataSpace memspace(2, startdim, NULL);

	dataset->write(data_ptr, dtype, memspace, filespace);
	LOG(INFO) << "End HDF5 Append entry";
}

string Hdf5Writer::CheckIntegrity(string file_name) {

	LOG(INFO) << "Checking integrity of " << file_name;

	if (mVerifyFilename != file_name) {
		if (mVerifyFile.is_open())
			mVerifyFile.close();

		mVerifyFilename = file_name;
		mVerifyFile.open(mBasePath + file_name);
	}

	string dataline;

	if (!getline(mVerifyFile, dataline)) {
		return "result=eof";
	} else {

		return "hash=" + md5(dataline);
	}
}

#endif
