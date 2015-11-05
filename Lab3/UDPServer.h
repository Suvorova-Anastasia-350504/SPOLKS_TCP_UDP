#pragma once
#include "Server.h"

struct ClientInfo
{
	unsigned packagesSent;
	sockaddr *addr;
	fstream *file;
	ClientInfo(sockaddr *addr, fstream*file)
	{
		this->addr = addr;
		this->file = file;
		packagesSent = PACKAGE_COUNT;
	}
};


class UDPServer : protected Server
{
private:
	fd_set updServerSet;
	vector<ClientInfo*> udpClients;
	void virtual Init() override;
	sockaddr_in* serverAddressInfo;
	
	void ProcessTCPClients();
	void ProcessUDPClients();
	void TryAddNewUDPClient();
	FileMetadata TryReceiveNewMessage(sockaddr *clientInfo);
	void SendFileParts() override;
	void AddNumberToDatagram(char *buffer, fpos_t size, fpos_t number);

public:
	UDPServer(unsigned int port = DEFAULT_PORT);
	void virtual Run() override;
};

