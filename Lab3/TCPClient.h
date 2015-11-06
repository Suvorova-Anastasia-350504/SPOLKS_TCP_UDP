#pragma once
#include "Client.h"

class TCPClient : public Client
{
	void Init();
	void Connect();
	void Reconnect();
	void ReceiveFile(fstream *targetFile, fpos_t currentPos, fpos_t fileSize);
	fpos_t ReceiveFileSize() override;
public:
	TCPClient(string address = DEFAULT_IP, unsigned int port = DEFAULT_PORT);
	void DownloadFile(string fileName) override;
};

