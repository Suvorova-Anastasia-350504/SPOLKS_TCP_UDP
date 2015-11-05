#pragma once
#include "Base.h"

#define CLIENTS_ONLINE "Clients online : " << this->clients.size() << endl
#define CLIENT_INFO pair<SOCKET, fstream*>*

struct FileMetadata {
	string fileName;
	fpos_t progress;
};

class Server : public Base
{
protected:
	vector<CLIENT_INFO> clients;
	fd_set serverSet;
	fd_set clientsSet;

	void TryToAddClient(bool wait = false);
	void RemoveClient(vector<CLIENT_INFO>::iterator& iter);
	virtual void SendFileParts();
	SOCKET CheckForNewConnection(bool wait = false);
	void SendBlock(CLIENT_INFO clientInfo);

	void Bind();
	void Bind(SOCKET socket);
	void virtual Init() override;
	void virtual OpenFile(fstream *file, string fileName) override;
	void virtual SetSocketTimeout() override;

	fpos_t GetFileSize(fstream *file);
	FileMetadata ExtractMetadata(string metadata);	
	sockaddr_in* CreateAddressInfoForServer();
public:
	explicit Server(unsigned int port = DEFAULT_PORT, bool start = true);

	void virtual Run();	
};

class NoNewClients : public runtime_error
{
public:
	explicit NoNewClients(const char* details) : runtime_error(details)
	{
	}
};

