#pragma once
#include "Base.h"
#include "SpeedRater.h"

#define DELTA_PERCENTAGE 5

class Client : public Base
{
private:
	void Connect();
	void Reconnect();

	void ReceiveFile(fstream *targetFile, fpos_t currentPos, fpos_t fileSize);
protected:
	string address;

	void virtual Init() override;
	void virtual OpenFile(fstream *file, string fileName) override;
	void virtual SetSocketTimeout() override;
	void virtual SetSocketTimeout(SOCKET socket) override;
	fpos_t virtual ReceiveFileSize();
	string virtual CreateFileInfo(string fileName, fpos_t pos);
	fpos_t MessageToFileSize(string message);
	string GetLocalFileName(string fileName);

	sockaddr_in* CreateAddressInfoForClient();
	Package* ReceiveRawData();
	string ReceiveMessage();
public:
	Client(string address = DEFAULT_IP, unsigned int port = DEFAULT_PORT, bool start = true);
	void virtual DownloadFile(string fileName);
	fpos_t ShowProgress(fpos_t lastProgress, fpos_t currentPos, fpos_t fileSize, SpeedRater *timer);
};

class ServerError : public runtime_error
{
public :
	ServerError(const char *details) : runtime_error(details)
	{
	}
};

