#pragma once
#include "Client.h"

#ifdef _WIN32
#define UDP_RECV_TIMEOUT 1000
#else
#define UDP_RECV_TIMEOUT 1
#endif

class UDPClient : public Client
{
	sockaddr *serverAddressInfo;
	vector<pair<fpos_t, Package*>*> receivedBuffer;
	vector<fpos_t> receivedPackages;

	fpos_t GetNumber(Package *package);
	fpos_t ConnectToServer(string metadata);
	void UDPClient::ReceiveBatch();
	void WriteBatchToFile(fstream* file, fpos_t& currentProgress);
protected:
	fpos_t virtual ReceiveFileSize() override;
	string CreateFileInfo(string fileName, fpos_t pos, int packageCount, bool request);
public:
	UDPClient(string address = DEFAULT_IP, unsigned int port = DEFAULT_PORT);
	void virtual DownloadFile(string fileName) override;
};

