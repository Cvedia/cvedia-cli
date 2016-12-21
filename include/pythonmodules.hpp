#ifndef _PYTHONMODULES_HPP
#define _PYTHONMODULES_HPP

using namespace std;

#include "config.hpp"

#ifdef HAVE_PYTHON

#include "Python.h"
#include "datawriter.hpp"
#include "api.hpp"

struct export_module_param {
	bool required;
	string option;
	string example;
	string description;
	string help;
};

struct export_module {

	string module_name;

	vector<export_module_param> module_params;
};

class PythonModules {

public:

	static int LoadPythonCore();
	static vector<export_module> ListModules(string path);
};

#endif

#endif
