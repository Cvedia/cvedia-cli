#ifndef _CVEDIA_HPP
#define _CVEDIA_HPP

extern int gDebug;

#define WriteDebugLog(s) if (gDebug == 1) {timeval curTime; struct tm tr; gettimeofday(&curTime, NULL); localtime_r(&curTime.tv_sec,&tr); int milli = curTime.tv_usec / 1000;printf("%02d:%02d:%02d.%d %d %s:%d] %s\n",tr.tm_hour, tr.tm_min, tr.tm_sec, milli, getpid(), __FILE__,__LINE__,s);}

#endif