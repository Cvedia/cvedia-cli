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

	// Clear the stats structure
	mCsvStats = {};

	if (mode == MODE_NEW) {

		mCreateTrainFile 	= false;
		mCreateTestFile 	= false;
		mCreateValFile 		= false;

		// Read all passed options
		if (mModuleOptions["create_train_file"] == "1")
			mCreateTrainFile = true;
		if (mModuleOptions["create_test_file"] == "1")
			mCreateTestFile = true;
		if (mModuleOptions["create_validate_file"] == "1")
			mCreateValFile = true;

		if (mModuleOptions["base_dir"] != "") {
			mBaseDir = mModuleOptions["base_dir"];
		} else {
			mBaseDir = "";
		}

		mBasePath = mBaseDir;

		if (mBasePath == "")
			mBasePath = "./";
		else
			mBasePath += "/";

		mBasePath += mExportName + "/";

		int dir_err = mkdir(mBasePath.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
		if (dir_err == -1 && errno != EEXIST) {
			LOG(ERROR) << string("Could not create directory: " + mBasePath).c_str();
			return -1;
		}

		if (mCreateTrainFile) {
			mTrainFile.open(mBasePath + "train.txt", ios::out | ios::trunc);
			if (!mTrainFile.is_open()) {
				LOG(ERROR) << string("Failed to open: " + mBasePath + "train.txt").c_str();
				return -1;			
			}

			try {
				mH5File[METADATA_TRAIN] = new H5File(mBasePath + "train.h5", H5F_ACC_TRUNC);
				mTrainFile << mBasePath << "train.h5" << endl;
			} catch (H5::Exception e) {
				LOG(ERROR) << string("Failed to create: " + mBasePath + "train.h5").c_str();
				LOG(ERROR) << e.getCDetailMsg();
				return -1;						
			}
		}

		if (mCreateTestFile) {
			mTestFile.open(mBasePath + "test.txt", ios::out | ios::trunc);
			if (!mTestFile.is_open()) {
				LOG(ERROR) << string("Failed to open: " + mBasePath + "test.txt").c_str();
				return -1;			
			}

			try {
				mH5File[METADATA_TEST] = new H5File(mBasePath + "test.h5", H5F_ACC_TRUNC);
				mTestFile << mBasePath << "test.h5" << endl;
			} catch (H5::Exception e) {
				LOG(ERROR) << string("Failed to create: " + mBasePath + "test.h5").c_str();
				LOG(ERROR) << e.getCDetailMsg();
				return -1;						
			}
		}

		if (mCreateValFile) {
			mValidateFile.open(mBasePath + "validate.txt", ios::out | ios::trunc);
			if (!mValidateFile.is_open()) {
				LOG(ERROR) << string("Failed to open: " + mBasePath + "validate.txt").c_str();
				return -1;			
			}

			try {
				mH5File[METADATA_VALIDATE] = new H5File(mBasePath + "validate.h5", H5F_ACC_TRUNC);
				mValidateFile << mBasePath << "validate.h5" << endl;			
			} catch (H5::Exception e) {
				LOG(ERROR) << string("Failed to create: " + mBasePath + "validate.h5").c_str();
				LOG(ERROR) << e.getCDetailMsg();
				return -1;						
			}
		}

		if (!mCreateTrainFile && !mCreateTestFile && !mCreateValFile) {
			LOG(ERROR) << "Must enable at least 1 of : create_train_file, create_test_file, create_validate_file";
			return -1;
		}
	}

	return 0;
}

bool Hdf5Writer::ValidateData(vector<Metadata* > meta) {

	// Only perform initialization once
	if (!mInitialized) {

		DSetCreatPropList source_prop;
		DSetCreatPropList ground_prop;

		Metadata *m = meta[0];

		string sourceDtype;
		string groundDtype;

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

			if (e->meta_type == METADATA_TYPE_SOURCE) {
				mSourceDataDim = dataDim;
				sourceDtype = e->dtype;
			}
			if (e->meta_type == METADATA_TYPE_GROUND) {
				mGroundDataDim = dataDim;
				groundDtype = e->dtype;
			}
		}

		LOG(DEBUG) << string("mSourceDataDim = " + to_string(mSourceDataDim) + "  mGroundDataDim = " + to_string(mGroundDataDim)).c_str();

		// We start with 0 entries and can scale endlessly
		hsize_t source_dims[2] 		= {0, mSourceDataDim};	
		hsize_t source_maxdims[2] 	= {H5S_UNLIMITED, mSourceDataDim};
		hsize_t ground_dims[2] 		= {0, mGroundDataDim};	
		hsize_t ground_maxdims[2] 	= {H5S_UNLIMITED, mGroundDataDim};

		// Used for chunked access of the h5 file
		hsize_t source_chunk_dims[2] 		= {2, mSourceDataDim};
		hsize_t ground_chunk_dims[2] 		= {2, mGroundDataDim};
		
		mSourceSpace = new DataSpace(2, source_dims, source_maxdims);
		mGroundSpace = new DataSpace(2, ground_dims, ground_maxdims);

		source_prop.setChunk(2, source_chunk_dims);
		ground_prop.setChunk(2, ground_chunk_dims);

		// Create the chunked dataset.  Note the use of pointer.
	    for(auto const& file : mH5File) {

			mDataSet[file.first] 	= new DataSet(mH5File[file.first]->createDataSet("data", (sourceDtype == "uint8" ? PredType::NATIVE_UCHAR : PredType::NATIVE_FLOAT), *mSourceSpace, source_prop));
			mLabelSet[file.first] 	= new DataSet(mH5File[file.first]->createDataSet("label", (groundDtype == "uint8" ? PredType::NATIVE_UCHAR : PredType::NATIVE_FLOAT), *mGroundSpace, ground_prop));
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
	return 0;
}


string Hdf5Writer::WriteData(Metadata* meta) {

	if (!mInitialized) {
		LOG(ERROR) << "Must call Initialize() first";
		return "";
	}

	if (meta->setname == METADATA_TRAIN) {
		if (!mCreateTrainFile) {
			LOG(ERROR) << "Training file not specified but data contains DATA_TRAIN";			
			return "";
		}

	} else if (meta->setname == METADATA_TEST) {
		if (!mCreateTestFile) {
			LOG(ERROR) << "Test file not specified but data contains DATA_TEST";			
			return "";
		}

	} else if (meta->setname == METADATA_VALIDATE) {
		if (!mCreateValFile) {
			LOG(ERROR) << "Validate file not specified but data contains DATA_VALIDATE";			
			return "";
		}

	} else {
		LOG(ERROR) << string("API returned unsupported file type: " + meta->setname).c_str();			
		return "";
	}

	void *source_ptr = NULL, *ground_ptr = NULL;
	string sourceDtype;
	string groundDtype;

	for (MetadataEntry* e : meta->entries) {

		void *ptr = NULL;

		if (e->value_type == METADATA_VALUE_TYPE_RAW) {

			if (e->dtype == "uint8")
				ptr = &e->uint8_raw_data[0];
			else if (e->dtype == "float")
				ptr = &e->float_raw_data[0];
		} else if (e->value_type == METADATA_VALUE_TYPE_NUMERIC) {

			if (e->dtype == "int")
				ptr = &e->int_value;
			else if (e->dtype == "float")
				ptr = &e->float_value;		
		}

		if (e->meta_type == METADATA_TYPE_SOURCE) {
			source_ptr = ptr;
			sourceDtype = e->dtype;
		}
		if (e->meta_type == METADATA_TYPE_GROUND) {
			ground_ptr = ptr;
			groundDtype = e->dtype;
		}
	}

	// Start the actual write
	AppendEntry(mDataSet[meta->setname], source_ptr, mSourceDataDim, ConvertDtype(sourceDtype));
	AppendEntry(mLabelSet[meta->setname], ground_ptr, mGroundDataDim, ConvertDtype(groundDtype));

	return "";
}

int Hdf5Writer::EndWriting() {
	return 0;
}


const DataType& Hdf5Writer::ConvertDtype(string dtype) {

	if (dtype == "uint8")
		return PredType::NATIVE_UCHAR;
	if (dtype == "float")
		return PredType::NATIVE_FLOAT;
	if (dtype == "int")
		return PredType::NATIVE_INT32;

	LOG(ERROR) << string("Unsupported dtype passed for conversion: " + dtype).c_str();
	return PredType::NATIVE_UCHAR;
}

void Hdf5Writer::AppendEntry(DataSet* dataset, void* data_ptr, hsize_t data_size, const DataType& dtype) {

	// Get current extent and increase by 1
	DataSpace tmp_space = dataset->getSpace();

	int num_dims = tmp_space.getSimpleExtentNdims();

	hsize_t sp_size[num_dims];

	tmp_space.getSimpleExtentDims(sp_size, NULL);

	hsize_t new_dim[2] 	= {sp_size[0]+1, data_size};

	// Extend the DataSet with 1 record
	dataset->extend(new_dim);

	// Find the end of the file and append new data. 
	DataSpace filespace = dataset->getSpace();

	// Define the hyperslab position, this is where we write to
	hsize_t offset[2] = {sp_size[0], 0};
	// We want to write a single entry 
	hsize_t startdim[2] = {1, data_size};

	filespace.selectHyperslab(H5S_SELECT_SET, startdim, offset);
	
	DataSpace memspace(2, startdim, NULL);

	dataset->write(data_ptr, dtype, memspace, filespace);
}

string Hdf5Writer::CheckIntegrity(string file_name) {

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
