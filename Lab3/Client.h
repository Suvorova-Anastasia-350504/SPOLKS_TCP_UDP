#pragma once
#include "Base.h"
#include "SpeedRater.h"

#define DELTA_PERCENTAGE 5

class Client : public Base
{
protected:
	string address;	
	
	fpos_t virtual ReceiveFileSize() = 0;

	void OpenFile(fstream *file, string fileName) override;
	string CreateFileInfo(string fileName, fpos_t pos);
	fpos_t StringToFileSize(string message);
	string GetLocalFileName(string fileName);
	fpos_t ShowProgress(fpos_t lastProgress, fpos_t currentPos, fpos_t fileSize, SpeedRater *timer);
	sockaddr_in* CreateAddressInfoForClient();
public:
	Client(string address, unsigned int port);
	void virtual DownloadFile(string fileName) = 0;
};

class ServerError : public runtime_error
{
public :
	ServerError(const char *details) : runtime_error(details)
	{
	}
};

