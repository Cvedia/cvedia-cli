#ifndef _LUAWRITER_HPP
#define _LUAWRITER_HPP

using namespace std;

#include "config.hpp"

#ifdef HAVE_LUA

#include "datawriter.hpp"
#include "api.hpp"
#include "pythonmodules.hpp" /* To use the structs */
#include "lua.h"  
#include "lualib.h"  
#include "lauxlib.h" 

class LuaWriter: public IDataWriter {

public:
	LuaWriter(string export_name, map<string, string> options);
	LuaWriter() {};
	~LuaWriter();

	WriterStats GetStats();
	void ClearStats();
	
	virtual bool CanHandle(string support);

	virtual int Initialize(DatasetMetadata* dataset_meta, int mode);

	virtual int BeginWriting();
	string WriteData(Metadata* meta);
	virtual int EndWriting();

	virtual string CheckIntegrity(string file_name);

	virtual int Finalize();

	string mBasePath;
	string mBaseDir;
	string mExportName;

	int mImagesWritten = 0;

	map<string, string> mModuleOptions;

private:

	virtual string PrepareData(Metadata* meta);
	string WriteImageData(string filename, vector<uint8_t> image_data);
	void DumpTable(lua_State *L, int table_index, bool recursive = false);

	WriterStats mCsvStats;

	bool mInitialized;
};

#endif

#endif
