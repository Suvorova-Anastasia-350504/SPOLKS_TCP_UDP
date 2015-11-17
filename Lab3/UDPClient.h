#pragma once
#include "Client.h"

class UDPClient : public Client
{
	sockaddr *serverAddressInfo;
	vector<fpos_t> expectedPackages;
	vector<fpos_t> receivedPackages;

	fpos_t GetNumber(Package *package);
	void GenerateExpectedPackages(fpos_t start, fpos_t fileSize);
	void RemoveExpectedPackage(fpos_t number);
	void UDPClient::ReceiveFile(fstream *file, fpos_t currentPos, fpos_t fileSize);
protected:
	fpos_t virtual ReceiveFileSize() override;
	string CreateFileInfo(string fileName, fpos_t pos, int packageCount, bool request);
public:
	UDPClient(string address = DEFAULT_IP, unsigned int port = DEFAULT_PORT);
	void virtual DownloadFile(string fileName) override;
};

