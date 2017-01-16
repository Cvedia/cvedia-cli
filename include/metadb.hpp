#ifndef _METADB_HPP
#define _METADB_HPP

using namespace std;

#include <sqlite3.h>

#include "api.hpp"

class MetaDb {

public:
	MetaDb();
	~MetaDb();

	int NewDb(const string db_file);
	int LoadDb(const string db_file);
	void InsertHash(string api_hash, string record_hash);
	void InsertMeta(string key, string value);
	string GetMeta(string key);
	bool HasApiHash(string hash);

private:
	void PrepareStatements();
	void SetPragmaOptions();

	sqlite3_stmt* stmt_api_hash_select;
	sqlite3_stmt* stmt_hash_insert;

	sqlite3* db;
};

#endif
