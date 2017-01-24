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
#include "LuaWriter.hpp"

LuaWriter::LuaWriter(string export_name, map<string, string> options) {

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
	return "";
}

int LuaWriter::Initialize(DatasetMetadata* dataset_meta, int mode) {
	lua_State *L = luaL_newstate();
	LOG(INFO) << "Lua initialize called with module " << mExportName;

	luaL_openlibs(L);

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
		LOG(ERROR) << "Could not find 'check_integrity' def in Lua script";
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

	lua_getglobal(L, "initialize");

	//Order arguments to be passed
	lua_pushinteger(L, mode);
	lua_gettable(L, set_list_table); 
	lua_gettable(L, field_list_table); 
	lua_gettable(L, options_table);



	//do the call (4 arguments, 1 result) 
	if (lua_pcall(L, 1, 1, 0) != 0) {
		LOG(ERROR) << "Error running function 'initialize' " << lua_tostring(L, -1) ;
		return -1;
	}

	mInitialized = 1;

	return 0;
}

bool LuaWriter::CanHandle(string support) {

    bool can_handle = false;

    return can_handle;
}

int LuaWriter::BeginWriting() {
	return 1;
}

int LuaWriter::EndWriting() {
	return 0;
}

string LuaWriter::CheckIntegrity(string file_name) {
	return "";
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
