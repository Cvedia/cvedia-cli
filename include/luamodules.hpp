#ifndef _LUAMODULES_HPP
#define _LUAMODULES_HPP

using namespace std;

#include "config.hpp"

#ifdef HAVE_LUA

#include "datawriter.hpp"
#include "api.hpp"
#include "pythonmodules.hpp" /* To use the structs */

class LuaModules {
public:
    static int LoadLuaCore();
    static vector<export_module> ListModules(string path);
};

#endif

#endif
