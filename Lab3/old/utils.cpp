#include "utils.h"

void waitForKeyPress() {
#ifdef _WIN32
	system("pause");
#endif
}

void showErrorMessage() {
	cout << "Incorrect args! Signature :" << endl 
		<< "<mode>[s/c] <ip>[if client] <port> <filePath>[if client]"
		<< endl;
}

void initSocketEnvironment() {
#ifdef _WIN32
	int iResult;
	WSADATA wsaData;

	iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != 0) {
		printf("WSAStartup failed with error: %d\n", iResult);
	}
#endif
}

void terminateSocketEnvironment() {
#ifdef _WIN32
	WSACleanup();
#endif
}

TIME_STRUCT initTimeStruct(unsigned time) {
	TIME_STRUCT timeStruct;
#ifdef _WIN32
	timeStruct = time;
#else
	timeStruct.tv_sec = time;
	timeStruct.tv_usec = 0;
#endif
	return timeStruct;
}

sockaddr_in* initAddressInfo(string address, unsigned int port) {
	struct sockaddr_in *addressInfo = new sockaddr_in();
	addressInfo->sin_family = AF_INET;
	addressInfo->sin_port = htons(port);
	inet_pton(AF_INET, address.c_str(), &(addressInfo->sin_addr.s_addr));
	return addressInfo;
}

fpos_t getFileSize(fstream *file) {
	fpos_t currentPosition = file->tellg();
	file->seekg(0, ios::end);
	fpos_t size = file->tellg();
	file->seekg(currentPosition);
	return size;
}

string getFileName(string path) {
#ifdef _WIN32
	char fileName[256];
	char extension[256];
	_splitpath_s(path.c_str(), NULL, 0, NULL, 0, fileName, 256, extension, 256);
	return string(fileName) + string(extension);
#endif
#ifdef linux
	TODO:: use basename()
#endif
}

vector<string> &split(const std::string &s, char delim, std::vector<std::string> &elems) {
	stringstream ss(s);
	string item;
	while (getline(ss, item, delim)) {
		elems.push_back(item);
	}
	return elems;
}


vector<string> split(const string &s, char delim) {
	vector<string> elems;
	split(s, delim, elems);
	return elems;
}

string formFilePath(string dir, string filename) {
	return dir + filename;
}

long long getCurTimeMilisec() {
	return duration_cast< milliseconds>(system_clock::now().time_since_epoch()).count();
}