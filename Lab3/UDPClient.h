#pragma once
#include "Client.h"

class UDPClient : protected Client
{
	sockaddr *serverAddressInfo;
	vector<fpos_t>* expectedPackages;
	fpos_t GetNumber(Package *package);
	void GenerateExpectedPackages(fpos_t start, fpos_t fileSize);
	void RemoveExpectedPackage(fpos_t number);
protected:
	void virtual Init() override;
	fpos_t virtual ReceiveFileSize() override;
public:
	UDPClient(string address, unsigned int port = DEFAULT_PORT);
	void virtual DownloadFile(string fileName) override;
};

