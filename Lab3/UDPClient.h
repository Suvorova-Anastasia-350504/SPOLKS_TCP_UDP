#pragma once
#include "Client.h"

#ifdef _WIN32
#define UDP_RECV_TIMEOUT 100
#else
#define UDP_RECV_TIMEOUT 100000
#endif

class UDPClient : public Client
{
	sockaddr *serverAddressInfo;
	vector<pair<fpos_t, char*>*> receivedBuffer;
	vector<fpos_t> receivedPackages;
	vector<fpos_t> missingPackages;

	fstream* file;
	string fileName;
	fpos_t fileSize;

	fpos_t GetNumber(Package *package);
	fpos_t ConnectToServer();
	void ProcessBatches(fstream* file, fpos_t fileSize);
	void ReceiveBatch();
	void WriteBatchToFile(fstream* file, fpos_t& currentProgress);
	void CollectMissingPackages(fpos_t& currentBatch, vector<fpos_t>& missingPackages);
	void RemoveFromMissingPackages(fpos_t index);
	void AddBatchToMissingPackages(fpos_t batch);
	void SendMissingPackages();
	fpos_t CreateMissingPackagesInfo(char* buffer, fpos_t bufferSize, bool requestAllPackages = false);
	fpos_t CreateConnectionInfo(char* buffer, fpos_t bufferSize);
	void InitMissingPackages();
protected:
	fpos_t virtual ReceiveFileSize() override;
	string CreateFileInfo(string fileName, fpos_t pos, int packageCount, bool request);
public:
	UDPClient(string address = DEFAULT_IP, unsigned int port = DEFAULT_PORT);
	void virtual DownloadFile(string fileName) override;
};

