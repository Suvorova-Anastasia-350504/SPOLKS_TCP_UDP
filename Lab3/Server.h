#pragma once
#include "Base.h"
#define CLIENTS_ONLINE "Clients online : " << this->tcpClients.size() << endl
#define CLIENT_INFO pair<SOCKET, fstream*>*

struct FileMetadata {
	string fileName;
	fpos_t progress;
};

class Server : public Base
{
	vector<CLIENT_INFO> tcpClients;
	vector<pair<string, fstream *>*> files;
	fd_set serverSet;
	fd_set clientsSet;
		
	void virtual OpenFile(fstream *file, string fileName) override;
	fstream* GetFile(string fileName);
	void Bind(SOCKET socket);	
	fpos_t GetFileSize(fstream *file);
	FileMetadata ExtractMetadata(string metadata);
	sockaddr_in* CreateAddressInfoForServer();

	void ProcessUDPClient();
	//TCP
	SOCKET Accept();
	void Listen();
	void SendFilePartsTCP(fd_set &clients);
	void AddNewTCPClient();
	void RemoveTCPClient(vector<CLIENT_INFO>::iterator& iter);
	void SendBlock(CLIENT_INFO clientInfo);
public:
	Server(unsigned int port = DEFAULT_PORT);
	void Run();
};

