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
#include <dirent.h>
#include <unistd.h>
#include <dlfcn.h>

using namespace std;

#include "config.hpp"

#ifdef HAVE_LUA

#include "lua.hpp"

#include "easylogging++.h"
#include "functions.hpp"
#include "cvedia.hpp"
#include "api.hpp"
#include "luamodules.hpp"

int LuaModules::LoadLuaCore() {
	lua_State *L = luaL_newstate();
	luaL_openlibs(L);
	return 1;
}

vector<export_module> LuaModules::ListModules(string path) {

	vector<export_module> modules;

	DIR* dir;
	struct dirent* p_ent;

	vector<string> lua_scripts;

	lua_State *L = luaL_newstate();

	luaL_openlibs(L);

	if ((dir = opendir(path.c_str())) != NULL) {
		while ((p_ent = readdir(dir)) != NULL) {

			string fname(p_ent->d_name);

			if (hasEnding(fname,".lua")) {
				lua_scripts.push_back(path + fname);
			}
		}

		closedir(dir);
	} else {
		LOG(ERROR) << "Could not open lua module folder at " << path;
		return modules;
	}

	if (lua_scripts.size() == 0) {
		LOG(ERROR) << "No lua modules found";
		return modules;		
	}

	for (string script_file : lua_scripts) {
		LOG(DEBUG) << "Reading module options from lua script " << script_file;

		export_module module;

		ifstream script_stream(script_file);
		if (!script_stream.is_open()) {
			LOG(ERROR) << "Error reading script file " << script_file;
			continue;
		}

		string script_data((istreambuf_iterator<char>(script_stream)),
					istreambuf_iterator<char>());

		if (luaL_loadfile(L, script_file.c_str()) || lua_pcall(L, 0, 0, 0)) {
	        LOG(ERROR) << "Error running luaL_loadfile " << lua_tostring(L, -1);
			continue;
		}


	    // Find the initialize function
		lua_getglobal(L, "load_module");
		if(!lua_isfunction(L,-1))
		{
			lua_pop(L,1);
	        LOG(ERROR) << "Could not find 'load_module' def in Lua script";
			continue;
		}


		//do the call (0 arguments, 1 result) 
		if (lua_pcall(L, 0, 1, 0) != 0) {
			LOG(ERROR) << "Error running function 'load_module' " << lua_tostring(L, -1) ;
			continue;
		}

		//get function result
		if (!lua_istable(L, -1)) {
			LOG(ERROR) << "Error running function 'load_module' must return a table";
			continue;
		}


		//get module name
		lua_pushstring(L, "module_name");
		lua_gettable(L, 1); 
		const char* lua_module_name = lua_tostring(L, -1);
		lua_pop(L, 1); 
		string module_name(lua_module_name);
		module.module_name = module_name;
		LOG(INFO) << "Lua Module name:" << module_name;


	    modules.push_back(module);

	}

	return modules;
}

#endif
