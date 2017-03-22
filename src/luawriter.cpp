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

#ifdef HAVE_LUA

#include "lua.h"  
#include "lualib.h"  
#include "lauxlib.h" 

#include "easylogging++.h"
#include "cvedia.hpp"
#include "api.hpp"
#include "luawriter.hpp"

LuaWriter::LuaWriter(string export_name, map<string, string> options) {
	luaL_openlibs(L);

	LOG(INFO) << "Initializing LuaWriter";

	mModuleOptions = options;
	// mExportName = export_name;
	mExportName = "lua/recordswriter.lua";
}

LuaWriter::~LuaWriter() {
}

WriterStats LuaWriter::GetStats() {

	return mCsvStats;
}

void LuaWriter::ClearStats() {

	// Clear the stats structure
	mCsvStats = {};
}

string LuaWriter::PrepareData(Metadata* meta) {

	string file_format = "$FILENAME $CATEGORY\n";

	string output_line = file_format;

	return output_line;
}

int LuaWriter::Finalize() {
	// Call the finalize function of our lua script
	return 0;
}

string LuaWriter::WriteData(Metadata* meta) {
	string result = "";
	//push the function to the stack before calling everything
	lua_getglobal(L, "write_data");
	lua_newtable(L);
	int entryDict = lua_gettop(L); //table with setname and entries
	lua_newtable(L);
	int metaEntries = lua_gettop(L); //Contains all entries
	int n = 1;
	for (MetadataEntry* e : meta->entries) {

		//convert the entry, and push it to the 
		lua_newtable(L);
		int metaDict = lua_gettop(L);


		const char* id_key = "id";
		int id_value = e->id;
		lua_pushlstring(L, id_key, strlen(id_key));
		lua_pushinteger(L, id_value);
		lua_settable(L, metaDict);

		const char* field_id_key = "field_id";
		int field_id_value = e->field_id;
		lua_pushlstring(L, field_id_key, strlen(field_id_key));
		lua_pushinteger(L, field_id_value);
		lua_settable(L, metaDict);

		const char* meta_type_key = "meta_type";
		const char* meta_type_value = e->meta_type.c_str();
		lua_pushlstring(L, meta_type_key, strlen(meta_type_key));
		lua_pushlstring(L, meta_type_value, e->meta_type.size());
		lua_settable(L, metaDict);

		const char* value_type_key = "value_type";
		const char* value_type_value = e->value_type.c_str();
		lua_pushlstring(L, value_type_key, strlen(value_type_key));
		lua_pushlstring(L, value_type_value, e->value_type.size());
		lua_settable(L, metaDict);

		const char* file_uri_key = "file_uri";
		const char* file_uri_value = e->file_uri.c_str();
		lua_pushlstring(L, file_uri_key, strlen(file_uri_key));
		lua_pushlstring(L, file_uri_value, e->file_uri.size());
		lua_settable(L, metaDict);

		const char* filename_key = "filename";
		const char* filename_value = e->filename.c_str();
		lua_pushlstring(L, filename_key, strlen(filename_key));
		lua_pushlstring(L, filename_value, e->filename.size());
		lua_settable(L, metaDict);

		const char* url_key = "url";
		const char* url_value = e->url.c_str();
		lua_pushlstring(L, url_key, strlen(url_key));
		lua_pushlstring(L, url_value, e->url.size());
		lua_settable(L, metaDict);

		const char* dtype_key = "dtype";
		const char* dtype_value = e->dtype.c_str();
		lua_pushlstring(L, dtype_key, strlen(dtype_key));
		lua_pushlstring(L, dtype_value, e->dtype.size());
		lua_settable(L, metaDict);

		const char* field_name_key = "field_name";
		const char* field_name_value = e->field_name.c_str();
		lua_pushlstring(L, field_name_key, strlen(field_name_key));
		lua_pushlstring(L, field_name_value, e->field_name.size());
		lua_settable(L, metaDict);

		const char* int_value_key = "int_value";
		int int_value_value = e->int_value;
		lua_pushlstring(L, int_value_key, strlen(int_value_key));
		lua_pushinteger(L, int_value_value);
		lua_settable(L, metaDict);

		const char* float_value_key = "float_value";
		lua_Number float_value_value  = e->float_value;
		lua_pushlstring(L, float_value_key, strlen(float_value_key));
		lua_pushnumber(L, float_value_value);
		lua_settable(L, metaDict);

		const char* data_channels_key = "data_channels";
		int data_channels_value = e->data_channels;
		lua_pushlstring(L, data_channels_key, strlen(data_channels_key));
		lua_pushinteger(L, data_channels_value);
		lua_settable(L, metaDict);

		const char* data_width_key = "data_width";
		int data_width_value = e->data_width;
		lua_pushlstring(L, data_width_key, strlen(data_width_key));
		lua_pushinteger(L, data_width_value);
		lua_settable(L, metaDict);

		const char* data_height_key = "data_height";
		int data_height_value = e->data_height;
		lua_pushlstring(L, data_height_key, strlen(data_height_key));
		lua_pushinteger(L, data_height_value);
		lua_settable(L, metaDict);


		const char* entriesKey = "metadata";
		lua_pushlstring(L, entriesKey, strlen(entriesKey));
		lua_newtable(L);
		int metadataTable = lua_gettop(L);
		int metadataIndex = 1;
		if (e->value_type == METADATA_VALUE_TYPE_RAW) {

			if (e->dtype == "uint8") {
				for (auto &data : e->uint8_raw_data) {
					lua_pushnumber(L, data);
					lua_rawseti( L, metadataTable, metadataIndex );
					metadataIndex++;
				}	

			} else if (e->dtype == "float") {
				for (auto &data : e->float_raw_data) {
					lua_Number luaFloat  = data;
					lua_pushnumber(L, luaFloat);
					lua_rawseti( L, metadataTable, metadataIndex );
					metadataIndex++;
				}	

			} else {
				LOG(ERROR) << "Unsupported dtype '" << e->dtype << "' passed";
				return "";
			}



		// Data is a regular image. Pass as an array nontheless
		} else if (e->value_type == METADATA_VALUE_TYPE_IMAGE) {

			// LOG(INFO) << "METADATA_VALUE_TYPE_IMAGE";
			// LOG(INFO) << e->image_data.size();
			// LOG(INFO) << reinterpret_cast<char*>(e->image_data.data());
			// LOG(INFO) << strlen(reinterpret_cast<char*>(e->image_data.data()));
			string metadata(e->image_data.begin(), e->image_data.end());
			// lua_pushstring(L, metadata.c_str());
			// lua_pushlstring(L, metadata.c_str(), strlen(metadata.c_str()));
			// LOG(INFO) << metadata.length();
			// LOG(INFO) << metadata;
			// LOG(INFO) << metadata.c_str();
			// LOG(INFO) << "C_STR strlen" << strlen(metadata.c_str());
			// lua_settable(L, metaDict);
			// for (auto &data : e->image_data) 
			// {
			// 	LOG(INFO) << "Foreach METADATA_VALUE_TYPE_IMAGE: " << data;
			// 	// const char* tmp = reinterpret_cast<const char*>(data);
			// 	// const char tmp =  (const char)(data);
			// 	char tmp[2];
			// 	// tmp[0] = (const char)(data);
			// 	tmp[0] = &data;
			// 	tmp[1] = '\0';
			// 	LOG(INFO) << "Foreach METADATA_VALUE_TYPE_IMAGE 2: " << tmp;
			// 	// lua_pushlstring(L, tmp, strlen(tmp));
			// 	lua_pushstring(L, tmp);
			// 	// lua_rawseti( L, metadataTable, metadataIndex );
			// 	// metadataIndex++;
			// }

			for( string::iterator it = metadata.begin(); it != metadata.end(); ++it )
			{
				char tempCString[2];
				tempCString[1] = '\0';
				tempCString[0] = *it;
				// LOG(INFO) << "Foreach METADATA_VALUE_TYPE_IMAGE asd: " << tempCString;
				lua_pushlstring(L, tempCString, strlen(tempCString));
				lua_rawseti( L, metadataTable, metadataIndex );
				metadataIndex++;
			}

			// for (auto &data : metadata) 
			// {
			// 	LOG(INFO) << "Foreach METADATA_VALUE_TYPE_IMAGE" << data;
			// 	const char* tmp = "" + data;
			// 	LOG(INFO) << "Foreach METADATA_VALUE_TYPE_IMAGE 2" << tmp;
			// 	lua_pushlstring(L, tmp, strlen(tmp));
			// 	// lua_rawseti( L, metadataTable, metadataIndex );
			// 	// metadataIndex++;
			// }

			// const char* tmp = metadata.c_str();
			// for ( unsigned int i = 0; i < strlen(metadata.c_str()) ; i++ ) {

			// 	LOG(INFO) << "Foreach METADATA_VALUE_TYPE_IMAGE" << tmp[i];
			// 	// lua_pushlstring(L, tmp, strlen(tmp));
			// 	// lua_rawseti( L, metadataTable, metadataIndex );
			// 	// metadataIndex++;
			// }

			// for ( unsigned int i = 0; i < metadata.length() ; i++ ) {

				// LOG(INFO) << "Foreach METADATA_VALUE_TYPE_IMAGE" << &metadata[i];
				// lua_pushlstring(L, tmp, strlen(tmp));
				// lua_rawseti( L, metadataTable, metadataIndex );
				// metadataIndex++;
			// }
			
		// Numerical value passed, check in which format
		} else if (e->value_type == METADATA_VALUE_TYPE_NUMERIC) {

			if (e->dtype == "int") {
				lua_pushnumber(L, e->int_value);
				lua_rawseti( L, metadataTable, metadataIndex );
			} else if (e->dtype == "float") {
				lua_Number luaFloat  = e->float_value;
				lua_pushnumber(L, luaFloat);
				lua_rawseti( L, metadataTable, metadataIndex );
			} else {
				LOG(ERROR) << "Unsupported dtype '" << e->dtype << "' passed";
				return "";				
			}
		} else if (e->value_type == METADATA_VALUE_TYPE_STRING) {
			lua_pushstring(L, e->string_value[0].c_str());
			lua_rawseti( L, metadataTable, metadataIndex );
		}

		//add the metadataTable to metaDict
		lua_settable(L, metaDict);

		//add the table on the top of the stack to the table
		lua_rawseti(L, metaEntries, n);
		n++;
	}

	//add meta entries to entryDict
	const char* entriesKey = "entries";
	lua_pushlstring(L, entriesKey, strlen(entriesKey));
	lua_pushvalue(L, metaEntries);
	lua_settable(L, entryDict);
	lua_remove(L, metaEntries); //remove the index now


	const char* setKey = "set";
	const char* setValue = meta->setname.c_str();
	lua_pushlstring(L, setKey, strlen(setKey));
	lua_pushlstring(L, setValue, meta->setname.size());
	lua_settable(L, entryDict);

	// AddToDict(entry_dict, PyUnicode_FromString("set"), PyUnicode_FromString(meta->setname.c_str()));
	// AddToDict(entry_dict, PyUnicode_FromString("entries"), pyMetaentry);

	//do the call (1 argument, 1 result) 
	if (lua_pcall(L, 1, 1, 0) != 0) {
		LOG(ERROR) << "Error running function 'WriteData' " << lua_tostring(L, -1) ;
		return "";
	}

		//get function result
	if (!lua_isstring(L, -1)) {
		LOG(ERROR) << "Error running function 'WriteData' must return a string";
		return "";
	} else {
		result = string(lua_tostring(L, -1));
	}

	return result;
}

int LuaWriter::Initialize(DatasetMetadata* dataset_meta, int mode) {
	ifstream script_stream(mExportName);
	if (!script_stream.is_open()) {
		LOG(ERROR) << "Error reading script file " << mExportName;
		return -1;
	}

	string script_data((istreambuf_iterator<char>(script_stream)), istreambuf_iterator<char>());

	if (luaL_loadfile(L, mExportName.c_str()) || lua_pcall(L, 0, 0, 0)) {
		LOG(ERROR) << "Error running luaL_loadfile " << lua_tostring(L, -1);
        return -1;
	}

	// Find the initialize function
	lua_getglobal(L, "initialize");
	if(!lua_isfunction(L,-1))
	{
		lua_pop(L,1);
		LOG(ERROR) << "Could not find 'load_module' def in Lua script";
        return -1;
	}

	// Find the can_handle function
	lua_getglobal(L, "can_handle");
	if(!lua_isfunction(L,-1))
	{
		lua_pop(L,1);
		LOG(ERROR) << "Could not find 'can_handle' def in Lua script";
		return -1;
	}

	// Find the begin_writing function
	lua_getglobal(L, "begin_writing");
	if(!lua_isfunction(L,-1))
	{
		lua_pop(L,1);
		LOG(ERROR) << "Could not find 'begin_writing' def in Lua script";
		return -1;
	}

	// Find the end_writing function
	lua_getglobal(L, "end_writing");
	if(!lua_isfunction(L,-1))
	{
		lua_pop(L,1);
		LOG(ERROR) << "Could not find 'end_writing' def in Lua script";
		return -1;
	}

	// Find the write_data function
	lua_getglobal(L, "write_data");
	if(!lua_isfunction(L,-1))
	{
		lua_pop(L,1);
		LOG(ERROR) << "Could not find 'write_data' def in Lua script";
		return -1;
	}

	// Find the check_integrity function
	lua_getglobal(L, "check_integrity");
	if(!lua_isfunction(L,-1))
	{
		lua_pop(L,1);
		if (CanHandle("integrity")) {
			LOG(ERROR) << "Could not find 'check_integrity' def in Lua script";
			return -1;
		}
		return -1;
	} 

	// Find the finalize function
	lua_getglobal(L, "finalize");
	if(!lua_isfunction(L,-1))
	{
		lua_pop(L,1);
		LOG(ERROR) << "Could not find 'finalize' def in Lua script";
		return -1;
	}

	mBasePath = mModuleOptions["working_dir"];

	//push the function to the stack before calling everything
	lua_getglobal(L, "initialize");


	// pass module options to lua
	lua_newtable(L);
	int options_table = lua_gettop(L);
	// int new_table = lua_gettop(L);
	// int index = 1;
	for (auto const& entry : mModuleOptions) {

		const char* key = entry.first.c_str();
		const char* value = entry.second.c_str();
		lua_pushlstring(L, key, entry.first.size());
		lua_pushlstring(L, value, entry.second.size());
		lua_settable(L, options_table);
		//lua_rawseti(L, new_table, ++index);
	}


	lua_newtable(L);
	int field_list_table = lua_gettop(L);
	// Build a list of all DatasetMetadata
	int n = 1;
	for (auto const& entry : dataset_meta->mapping_by_id) {
		const char* id_key = "id";
		int id_value = entry->id;

		const char* name_key = "name";
		const char* name_value = entry->name.c_str();
		lua_newtable(L);
		int lua_dict = lua_gettop(L);

		lua_pushlstring(L, id_key, strlen(id_key));
		lua_pushinteger(L, id_value);
		lua_settable(L, lua_dict);
		lua_pushlstring(L, name_key, strlen(name_key));
		lua_pushlstring(L, name_value, entry->name.size());
		lua_settable(L, lua_dict);
		//add the table on the top of the stack to the table
		lua_rawseti(L, field_list_table, n);
		n++;
	}

	lua_newtable(L);
	int set_list_table = lua_gettop(L);
	// Build a list of all DatasetMetadata
	n = 1;
    for (auto const& entry : dataset_meta->sets) {
    	const char* set_name = entry.set_name.c_str();

		lua_pushlstring(L, set_name, entry.set_name.size());
		lua_rawseti(L, set_list_table, n);
		n++;
    }

	//check if it truly was in the table
	// lua_pushstring(L, "export_name");
	// lua_gettable(L, options_table); 
	// const char* lua_module_name = lua_tostring(L, -1);
	// lua_pop(L, 1); 
	// string module_name(lua_module_name);
	// LOG(INFO) << "is the value in lua table?? " << module_name;
	
	// Add Sets to a 'sets' dictionary
	// LOG(INFO) << "About to call generate dict_meta";
	// const char* sets_key = "sets";
	// const char* fields_key = "fields";

	// lua_newtable(L);
	// int dict_meta = lua_gettop(L);
	// lua_pushlstring(L, sets_key, strlen(sets_key));
	// lua_gettable(L, set_list_table); 
	// lua_settable(L, dict_meta);
	// dict_meta = dict_meta -2; //it moved because the pushed stuff
	// lua_pushlstring(L, fields_key, strlen(fields_key));
	// lua_gettable(L, field_list_table); 

	// LOG(INFO) << "What is dict_meta: " << lua_typename(L, lua_type(L, dict_meta));


	// lua_settable(L, dict_meta);
	// LOG(INFO) << "Second settable done";
	// LOG(INFO) << "What is dict_meta: " << lua_typename(L, lua_type(L, dict_meta));
	// DumpTable(L, dict_meta);
	// LOG(INFO) << "dump done";
	// lua_rawset(L, sets_key, set_list_table);
	// lua_rawset(L, fields_key, field_list_table);


	// Put dictionary in a tuple
	// PyObject* pTuple = PyTuple_New(3);
	// PyTuple_SetItem( pTuple, 0, dict_module_options);
	// PyTuple_SetItem( pTuple, 1, dict_meta);
	// PyTuple_SetItem( pTuple, 2, PyLong_FromLong(mode));


	//Order arguments to be passed
	lua_pushinteger(L, mode);
	// lua_gettable(L, set_list_table); 
	// lua_gettable(L, field_list_table); 
	// lua_gettable(L, options_table);



	//do the call (4 arguments, 1 result) 
	if (lua_pcall(L, 4, 1, 0) != 0) {
		LOG(ERROR) << "Error running function 'initialize' " << lua_tostring(L, -1) ;
		return -1;
	}

	mInitialized = 1;

	return 0;
}

bool LuaWriter::CanHandle(string support) {

    bool can_handle = false;

	lua_getglobal(L, "can_handle");
	lua_pushlstring(L, support.c_str(), support.size());
    if (lua_pcall(L, 1, 1, 0) != 0) {
    	LOG(ERROR) << "Error running function 'can_handle' " << lua_tostring(L, -1) ;
    	return false;
    }

    //get function result
    if (!lua_isboolean(L, -1)) {
    	LOG(ERROR) << "Error running function 'can_handle' must return a boolean";
    	return false;
    } else {
    	can_handle = lua_toboolean(L, -1);
    }

    return can_handle;
}

int LuaWriter::BeginWriting() {
	// Call the begin_writing function of our lua script

	lua_getglobal(L, "begin_writing");
    if (lua_pcall(L, 0, 1, 0) != 0) {
    	LOG(ERROR) << "Error running function 'begin_writing' " << lua_tostring(L, -1) ;
    	return false;
    }

    //get function result
    if (!lua_isboolean(L, -1)) {
    	LOG(ERROR) << "Error running function 'begin_writing' must return a boolean";
    	return false;
    } 

	return 0;
}

int LuaWriter::EndWriting() {
	// Call the end_writing function of our python script
	lua_getglobal(L, "end_writing");
    if (lua_pcall(L, 0, 1, 0) != 0) {
    	LOG(ERROR) << "Error running function 'end_writing' " << lua_tostring(L, -1) ;
    	return false;
    }

    //get function result
    if (!lua_isboolean(L, -1)) {
    	LOG(ERROR) << "Error running function 'end_writing' must return a boolean";
    	return false;
    } 

	return 0;
}

string LuaWriter::CheckIntegrity(string file_name) {

	string result = "";

	lua_getglobal(L, "check_integrity");
	lua_pushlstring(L, file_name.c_str(), file_name.size());
	if (lua_pcall(L, 1, 1, 0) != 0) {
		LOG(ERROR) << "Error running function 'check_integrity' " << lua_tostring(L, -1) ;
		return "";
	}

    //get function result
	if (!lua_isstring(L, -1)) {
		LOG(ERROR) << "Error running function 'check_integrity' must return a string";
		return "";
	} else {
		result = string(lua_tostring(L, -1));
	}

	return result;
}

/**
 * Debug method, dumps the table to the log
 * @param L           Lua state
 * @param table_index Index of the table
 * @param recursive   Is a inner recursive call?
 */
void LuaWriter::DumpTable( lua_State *L, int table_index, bool recursive ) {
	if(recursive == false){
		lua_gettable(L, table_index); 
	}
	lua_pushnil(L);  /* first key */
	while (lua_next(L, table_index) != 0) {
		/* uses 'key' (at index -2) and 'value' (at index -1) */
		const char* valueType = lua_typename(L, lua_type(L, -1));
		LOG(INFO) << "Checking table value type: " << valueType;
		const char* keyType = lua_typename(L, lua_type(L, -2));
		LOG(INFO) << "Checking table key type: " << keyType;
		if(lua_istable(L, -1)){
			LOG(INFO) << "Checking inner table";
			DumpTable(L, -1 -1, true);
		} else {
			string keyTypeString(keyType);
			if(keyTypeString == "string"){
				const char* key = lua_tostring(L, -2);
				LOG(INFO) << "Key is: " << key;
			} else if(keyTypeString == "number"){
				int key = lua_tonumber(L, -2);
				LOG(INFO) << "Key is: " << key;
			}
			string valueTypeString(valueType);
			if(valueTypeString == "string"){
				const char* value = lua_tostring(L, -1);
				LOG(INFO) << "Value is: " << value;
			} else if(valueTypeString == "number"){
				int value = lua_tonumber(L, -1);
				LOG(INFO) << "Value is: " << value;
			}


		}

		lua_pop(L, 1);
	}
}

#endif
