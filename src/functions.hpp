#ifndef _FUNCTIONS_HPP
#define _FUNCTIONS_HPP

string ReplaceString(string subject, const string& search, const string& replace) {
	
	size_t pos = 0;

	while ((pos = subject.find(search, pos)) != string::npos) {
	     subject.replace(pos, search.length(), replace);
	     pos += replace.length();
	}

	return subject;
}

void Split(const string &s, char delim, vector<string> &elems) {
    stringstream ss;
    ss.str(s);
    string item;
    while (getline(ss, item, delim)) {
        elems.push_back(item);
    }
}

vector<string> Split(const string &s, char delim) {
    vector<string> elems;
    Split(s, delim, elems);
    return elems;
}

#endif