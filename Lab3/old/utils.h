#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <vector>
#include <chrono>
#include "DataTypeDefinitions.h"


#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#pragma comment (lib, "Ws2_32.lib")
#endif

#ifdef linux

#include <sys/types.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/socket.h>
#endif

#define _CRT_SECURE_NO_WARNINGS

using namespace std;
using namespace std::chrono;

void showErrorMessage();
void waitForKeyPress();
void initSocketEnvironment();
void terminateSocketEnvironment();
TIME_STRUCT initTimeStruct(unsigned time);
sockaddr_in* initAddressInfo(string address, unsigned int port);
fpos_t getFileSize(fstream *file);
string getFileName(string path);
vector<string> split(const string &s, char delim);
string formFilePath(string dir, string filename);
long long getCurTimeMilisec();