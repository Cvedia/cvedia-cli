#ifndef _FUNCTIONS_HPP
#define _FUNCTIONS_HPP

#include <iostream>

using namespace std;

string ReplaceString(string subject, const string& search, const string& replace);
void Split(const string& s, char delim, vector<string>& elems);
vector<string> Split(const string& s, char delim);
bool hasEnding(string const& fullString, string const& ending);

#endif