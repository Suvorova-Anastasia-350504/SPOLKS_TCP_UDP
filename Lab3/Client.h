#pragma once
#include "Base.h"
#include "SpeedRater.h"

#define DELTA_PERCENTAGE 5

using namespace std;

class Client : public Base
{
protected:
	std::string address;	
	
	fpos_t virtual ReceiveFileSize() = 0;

	void OpenFile(std::fstream *file, std::string fileName) override;
	std::string CreateFileInfo(std::string fileName, fpos_t pos);
	fpos_t StringToFileSize(std::string message);
	std::string GetLocalFileName(std::string fileName);
	fpos_t ShowProgress(fpos_t lastProgress, fpos_t currentPos, fpos_t fileSize, SpeedRater *timer);
	sockaddr_in* CreateAddressInfoForClient();
public:
	Client(std::string address, unsigned int port);
	void virtual DownloadFile(std::string fileName) = 0;
};

class ServerError : public std::runtime_error
{
public :
	ServerError(const char *details) : std::runtime_error(details)
	{
	}
};

