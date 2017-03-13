#include <deque>
#include <map>
#include <vector>
#include <string>
#include <regex>
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

#define RANK         2
#define RANKC        1
#define NX           10
#define NY           5 


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
	string writeCountStr =  std::to_string(writeCount);
	for (DatasetSet s : mDatasetMetadata->sets) {
		// LOG(INFO) << mBasePath;
		// LOG(INFO) << s.set_name;
		mSetFile[s.set_name].open(mBasePath + s.set_name + ".txt", ios::out | ios::app);
		// LOG(INFO) << "msetFile .open";
		if (!mSetFile[s.set_name].is_open()) {
			LOG(ERROR) << "Failed to open: " << mBasePath << s.set_name << ".txt";
			return -1;			
		}		

		try {
			// LOG(INFO) << "Before New h5file";
			if(mH5File[s.set_name] != NULL ){
				// LOG(INFO) << "Reopen";
				// mH5File[s.set_name]->reOpen();
				mH5File[s.set_name]->openFile(mBasePath + s.set_name + writeCountStr + ".h5", H5F_ACC_RDWR);
			} else {
				// LOG(INFO) << "new h5file";
				mH5File[s.set_name] = new H5File(mBasePath + s.set_name + writeCountStr + ".h5", H5F_ACC_TRUNC);
			}
			// LOG(INFO) << "New h5file";
			mSetFile[s.set_name] << mBasePath << s.set_name << writeCountStr << ".h5" << endl;
		} catch (H5::Exception e) {
			LOG(ERROR) << "Failed to create: " + mBasePath + s.set_name << writeCountStr << ".h5";
			LOG(ERROR) << e.getCDetailMsg();
			return -1;						
		}

	}
	writeCount++;

	return 0;
}


string Hdf5Writer::WriteData(Metadata* meta) {
	if(!mSetFile.count(meta->setname)){
		LOG(ERROR) << "Set file does not exist " << meta->setname;
	}

	void *ptr = NULL;
	string dtype;
	bool initializedDatasets = !mDataSet.empty();
	string output_line;
	string hashes = "";
	string hash = "";

	for (MetadataEntry* e : meta->entries) {
		long unsigned int dataDim = 0;

		void *tmp_ptr = NULL;
		if (e->value_type == METADATA_VALUE_TYPE_IMAGE || e->value_type == METADATA_VALUE_TYPE_RAW) {
			if (e->dtype == "uint8"){
				tmp_ptr = &e->uint8_raw_data[0];
				dataDim = e->uint8_raw_data.size();
				string s(e->uint8_raw_data.begin(), e->uint8_raw_data.end());
			}
			else if (e->dtype == "float"){
				tmp_ptr = &e->float_raw_data[0];
				dataDim = e->float_raw_data.size();
				string s(e->float_raw_data.begin(), e->float_raw_data.end());
			} else { //dtype is ""
				tmp_ptr = &e->image_data[0];
				string s(e->image_data.begin(), e->image_data.end());
				dataDim = e->image_data.size();
				// &e->image_data[0], e->image_data.size()
				// dataDim = 1;
			}
		} else if (e->value_type == METADATA_VALUE_TYPE_NUMERIC) {
			if (e->dtype == "int"){
				tmp_ptr = &e->int_value;
				string s = std::to_string(e->int_value);
				dataDim = 1;
			}
			else if (e->dtype == "float"){
				tmp_ptr = &e->float_value;		
				string s = std::to_string(e->float_value);
				dataDim = 1;
			}
		}
		// LOG(INFO) << "ValueType: " << e->value_type;
		// LOG(INFO) << "DType: " << e->dtype;
		// LOG(INFO) << "Datadim: " << dataDim;

		dtype = e->dtype;
		ptr = tmp_ptr;

		// LOG(INFO) << "Hash of pointer2 " << md5(ptr);

		if(initializedDatasets == false ){
			// LOG(INFO) << "Initializing datasets";
			//Due to api limitations ATM, we need to initialize datasets here, because its the first place where we know the datatype
			// @TODO: Move this to beginWriting
			// We start with 0 entries and can scale endlessly
			// LOG(INFO) << "Datadim: " << dataDim;
			DSetCreatPropList prop;
			hsize_t dims[2] 		= {0, dataDim};	
			hsize_t maxdims[2] 	= {H5S_UNLIMITED, H5S_UNLIMITED};
			hsize_t chunk_dims[2] = {2, dataDim};
			mSpace = new DataSpace(2, dims, maxdims);
			prop.setChunk(2, chunk_dims);

			// Create the chunked dataset.  Note the use of pointer.
			for(auto const& file : mH5File) {
				mDataSet[file.first] 	= new DataSet(mH5File[file.first]->createDataSet(file.first, (e->dtype == "uint8" ? PredType::NATIVE_UCHAR : PredType::NATIVE_FLOAT), *mSpace, prop));
			}

			// LOG(INFO) << "Initialiazed datasets";
			initializedDatasets = true;
		}

		// Start the actual write
		// LOG(INFO) << "Datadim before AppendEntry: " << dataDim;

		hash = AppendEntry(mDataSet[meta->setname], ptr, dataDim, ConvertDtype(dtype));
		hashes = hashes + hash;
	}

	// for (DatasetMapping* mapping : mDataset->mapping_by_id)	{
	// 	output_line += (output_line != "" ? " " : "") + output_values[mapping->id];
	// }
	
	//must return md5 here
	//return md5 of the incoming image (we have the pointer to the image)
	LOG(INFO) << "Hashes string:" << hashes;
	LOG(INFO) << "file=" + meta->setname + ".h5;hash=" + hashes;
	return "file=" + meta->setname + ".h5;hash=" + hashes;
}

int Hdf5Writer::EndWriting() {
	// Close all open files
	for (DatasetSet s : mDatasetMetadata->sets) {
		// LOG(INFO) << "Closing " << s.set_name;
		mSetFile[s.set_name].close();
		mH5File[s.set_name]->close();

		mH5File.erase(s.set_name);
		mDataSet.erase(s.set_name);
	}


	return 0;
}


const DataType& Hdf5Writer::ConvertDtype(string dtype) {
	// LOG(INFO) << "ConvertDtype: " << dtype;

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

string Hdf5Writer::AppendEntry(DataSet* dataset, void* data_ptr, hsize_t data_size, const DataType& dtype) {
	// LOG(INFO) << "AppendEntry data_size: " << data_size;
	std::string field_value;

	try{
		// Get current extent and increase by 1
		DataSpace tmp_space = dataset->getSpace();

		int num_dims = tmp_space.getSimpleExtentNdims();

		// LOG(INFO) << "num_dims: " << num_dims;
		// LOG(INFO) << "ptr: " << data_ptr;


		hsize_t sp_size[num_dims];

		tmp_space.getSimpleExtentDims(sp_size, NULL);

		hsize_t new_dim[2] 	= {sp_size[0]+1, data_size};

		// Extend the DataSet with 1 record
		// LOG(INFO) << "new_dim: " << new_dim[0];
		// LOG(INFO) << "new_dim: " << new_dim[1];
		dataset->extend(new_dim);

		// Find the end of the file and append new data. 
		DataSpace filespace = dataset->getSpace();

		// int num_dims_filespace = filespace.getSimpleExtentNdims();
		// LOG(INFO) << "filespace datasize: " << num_dims_filespace;


		// Define the hyperslab position, this is where we write to
		hsize_t offset[2] = {sp_size[0], 0};
		// We want to write a single entry 
		hsize_t startdim[2] = {1, data_size};
		// LOG(INFO) << "Data size: " << data_size;

		filespace.selectHyperslab(H5S_SELECT_SET, startdim, offset);
		
		DataSpace memspace(2, startdim, NULL);
		// LOG(INFO) << "startdim: " << startdim;

		dataset->write(data_ptr, dtype, memspace, filespace);
		dataset->read( field_value, dtype, memspace, filespace );
		LOG(INFO) << "Read after write md5: " << md5(field_value);
	} catch(...) {
		LOG(ERROR) << "Retrying...";
		AppendEntry(dataset, data_ptr, data_size, dtype);
	}

	return md5(field_value);

}

string Hdf5Writer::CheckIntegrity(string file_name) {

	LOG(INFO) << "Checking integrity of " << file_name;
	string datasetName = regex_replace(file_name, regex("[\\d]*\\.h5"), "");

	if( !checkFileCount.count(datasetName)){
		checkFileCount[datasetName] = 1;
	}
	int datasetCheckFileCount = checkFileCount[datasetName];

	if(!FileExists(mBasePath + file_name)){
		file_name = regex_replace(file_name, regex("[\\d]*\\.h5"), std::to_string(datasetCheckFileCount) + ".h5");
	}

	string fullFile = mBasePath + file_name;
	if(!FileExists(fullFile)){
		return "result=eof";
	}

	LOG(INFO) << "fullFile:  " << fullFile;

	// if(mH5File[s.set_name] != NULL ){
	// 	mH5File[s.set_name]->openFile(mBasePath + s.set_name + writeCountStr + ".h5", H5F_ACC_RDWR);
	// } else {
	// 	mH5File[s.set_name] = new H5File(mBasePath + s.set_name + writeCountStr + ".h5", H5F_ACC_RDONLY);
	// }
	// DSetCreatPropList prop;
	// hsize_t dims[2] 		= {0, dataDim};	
	// hsize_t maxdims[2] 	= {H5S_UNLIMITED, H5S_UNLIMITED};
	// hsize_t chunk_dims[2] = {2, dataDim};
	// mSpace = new DataSpace(2, dims, maxdims);
	// prop.setChunk(2, chunk_dims);

	// 		// Create the chunked dataset.  Note the use of pointer.
	// for(auto const& file : mH5File) {
	// 	mDataSet[file.first] 	= new DataSet(mH5File[file.first]->createDataSet(meta->setname, (e->dtype == "uint8" ? PredType::NATIVE_UCHAR : PredType::NATIVE_FLOAT), *mSpace, prop));
	// }


	// hid_t       file;                     
	// hid_t       dataset;  
	// hid_t       filespace;                   
	// hid_t       memspace;                  
	// hid_t       cparms;                   
	// hsize_t     dims[2];                  
	// hsize_t     chunk_dims[2];
	// hsize_t     col_dims[1];
	// hsize_t     count[2];
	// hsize_t     offset[2];

	// herr_t      status, status_n;                             

	// int         data_out[NX][NY];  
	// int         chunk_out[2][5];   
	// int         column[10];        
	// int         rank, rank_chunk;
	// hsize_t	i, j;

	H5File file( fullFile, H5F_ACC_RDONLY );
	DataSet dataset = file.openDataSet(datasetName); 
	DataType dataType = dataset.getDataType();

	DataSpace fileDataSpace = dataset.getSpace();
	int nDims = fileDataSpace.getSimpleExtentNdims();

	LOG(INFO) << "nDims: " << nDims;

	hsize_t sp_size[nDims];
	int rank = fileDataSpace.getSimpleExtentDims( sp_size );
	LOG(INFO) << "rank: " << rank;
	LOG(INFO) << "sp_size0: " << sp_size[0];
	LOG(INFO) << "sp_size1: " << sp_size[1];

	// int newbuffer[40000];
	std::string field_value;
	// int *p2DArray = new int[NX*NY]; 
	// vector<unsigned char> newBuffer;
	// string  newBuffer[NX][NY];
	// for (j = 0; j < NX; j++)
	// {
	// 	for (i = 0; i < NY; i++)
	// 	{
	// 		for (k = 0; k < NZ ; k++)
	// 			newBuffer[j][i][k] = 0;
	// 	}
	// }
	// hsize_t count[2] = {sp_size[0], 0};
	// hsize_t count[2] = {sp_size[0], sp_size[1]};
	// // hsize_t start[2] = {1, sp_size[1]};
	// hsize_t start[2] = {0, 0};

	// Define the hyperslab position, this is where we write to
	// hsize_t offset[2] = {sp_size[0], 0};
	LOG(INFO) << "checkIntegrityCount: " << checkIntegrityCount;

	hsize_t offset[2] = {checkIntegrityCount, 0};
	// We want to fetch a single entry 
	hsize_t startdim[2] = {1, sp_size[1]};
	// LOG(INFO) << "Data size: " << data_size;
	DataSpace mspace1(2, startdim, NULL);

	fileDataSpace.selectHyperslab(H5S_SELECT_SET, startdim, offset);



	// fileDataSpace.selectHyperslab(H5S_SELECT_SET, count, start);
	dataset.read( field_value, PredType::NATIVE_UCHAR, mspace1, fileDataSpace );

	// string s(newBuffer.begin(), newBuffer.end());
	// LOG(INFO) << "Datasize " << newBuffer.size();
	LOG(INFO) << "String " << md5(field_value);
	LOG(INFO) << "String size " << field_value.size();

	// hsize_t count[2] = {sp_size[0], 0};
	// count[2] = {sp_size[0], sp_size[1]};
	// hsize_t start[2] = {1, sp_size[1]};
			// offset[0] = 2;

			// fileDataSpace.selectHyperslab(H5S_SELECT_SET, startdim, offset);

			// dataset.read( field_value, PredType::NATIVE_UCHAR, mspace1, fileDataSpace );
			// LOG(INFO) << "String2 " << md5(field_value);
			// LOG(INFO) << "String2 size " << field_value.size();
	// string datasetData = "";
	// dataset.read( datasetData, dataType,  , dataSpace );
	// LOG(INFO) << "Read data: " datasetData;

	// filespace = H5Dget_space(dataset);  
	// rank      = H5Sget_simple_extent_ndims(filespace);
	// status_n  = H5Sget_simple_extent_dims(filespace, dims, NULL);
	// printf("dataset rank %d, dimensions %lu x %lu\n", rank, (unsigned long)(dims[0]), (unsigned long)(dims[1]));

	// cparms = H5Dget_create_plist(dataset);

	// if (H5D_CHUNKED == H5Pget_layout(cparms))  {
	// 	rank_chunk = H5Pget_chunk(cparms, 2, chunk_dims);
	// 	printf("chunk rank %d, dimensions %lu x %lu\n", rank_chunk, (unsigned long)(chunk_dims[0]), (unsigned long)(chunk_dims[1]));
	// }

	//dataset rank 2, dimensions 154 x 32640
	//chunk rank 2, dimensions 2 x 32640

	// memspace = H5Screate_simple(RANK,dims,NULL);
	// LOG(INFO) << "After memspace";

	// status = H5Dread(dataset, H5T_NATIVE_INT, memspace, filespace, H5P_DEFAULT, data_out);
	// LOG(INFO) << "After H5Dread";
	// printf("\n");
	// printf("Dataset: \n");
	// for (j = 0; j < dims[0]; j++) {
	// 	for (i = 0; i < dims[1]; i++) printf("%d ", data_out[j][i]);
	// 		printf("\n");
	// }     
	file.close();

	string dataline;

	/* Increment file count for and reset index count next loop */
	if(checkIntegrityCount >= sp_size[0] -1 ){
		checkFileCount[datasetName]++;
		checkIntegrityCount = 0;
	} else {
		checkIntegrityCount++;
	}

	return "hash=" + md5(field_value);

}

bool Hdf5Writer::FileExists(string fileName) {
    std::ifstream infile(fileName);
    return infile.good();
}

#endif
