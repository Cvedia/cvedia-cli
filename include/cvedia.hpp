#ifndef _CVEDIA_HPP
#define _CVEDIA_HPP

#include "curlreader.hpp"
#include "api.hpp"

using namespace std;

extern int gDebug;

int VerifyApi(map<string,string> options);
void ConfigureLogger();
string JoinStringVector(const vector<string>& vec, const char* delim);
int StartExport(map<string,string> options);
string ReplaceString(string subject, const string& search, const string& replace);
void DisplayProgressBar(float progress, int cur_value, int max_value);
int ReadRequestToMetadataEntry(ReadRequest* req, MetadataEntry* entry);

void split(const std::string &s, char delim, std::vector<std::string> &elems);
std::vector<std::string> split(const std::string &s, char delim);

//#define WriteDebugLog(s) if (gDebug == 1) {timeval curTime; struct tm tr; gettimeofday(&curTime, NULL); localtime_r(&curTime.tv_sec,&tr); int milli = curTime.tv_usec / 1000;printf("%02d:%02d:%02d.%d %d %s:%d] %s\n",tr.tm_hour, tr.tm_min, tr.tm_sec, milli, getpid(), __FILE__,__LINE__,s);}
//#define WriteErrorLog(s) {timeval curTime; struct tm tr; gettimeofday(&curTime, NULL); localtime_r(&curTime.tv_sec,&tr); int milli = curTime.tv_usec / 1000;printf("%02d:%02d:%02d.%d %d %s:%d] %s\n",tr.tm_hour, tr.tm_min, tr.tm_sec, milli, getpid(), __FILE__,__LINE__,s);}
//#define WriteLog(s) {timeval curTime; struct tm tr; gettimeofday(&curTime, NULL); localtime_r(&curTime.tv_sec,&tr); int milli = curTime.tv_usec / 1000;printf("%02d:%02d:%02d.%d %d %s:%d] %s\n",tr.tm_hour, tr.tm_min, tr.tm_sec, milli, getpid(), __FILE__,__LINE__,s);}

#endif
