#pragma once
#include "Client.h"

class UDPClient : public Client
{
	sockaddr *serverAddressInfo;
protected:
	fpos_t virtual ReceiveFileSize() override;
	string CreateFileInfo(string fileName, fpos_t pos, bool knowFileSize);
public:
	UDPClient(string address = DEFAULT_IP, unsigned int port = DEFAULT_PORT);
	void virtual DownloadFile(string fileName) override;
};

