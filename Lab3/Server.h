#pragma once
#include "Base.h"
#define CLIENTS_ONLINE "Clients online : " << this->tcpClients.size() << endl
#define CLIENT_INFO pair<SOCKET, fstream*>*

struct TCPMetadata {
	string fileName;
	fpos_t progress;
};

struct UDPMetadata {
	string fileName;
	fpos_t progress;
	sockaddr *addr;
	fpos_t packagesToSend;
	fstream *file;
};

class Server : public Base
{
	vector<CLIENT_INFO> tcpClients;
	vector<UDPMetadata*> udpClients;
	fd_set serverSet;
	fd_set clientsSet;
		
	void virtual OpenFile(fstream *file, string fileName) override;
	void Bind(SOCKET socket);	
	fpos_t GetFileSize(fstream *file);
	TCPMetadata ExtractMetadata(string metadata);
	UDPMetadata* ExtractMetadataUDP(string metadata);
	sockaddr_in* CreateAddressInfoForServer();

	//UDP
	void AddUDPClient();
	void SendFilePartsUDP();
	void AddNumberToDatagram(char *buffer, fpos_t size, fpos_t number);
	void RemoveUDPClient(vector<UDPMetadata*>::iterator& iter);
	//TCP
	SOCKET Accept();
	void Listen();
	void SendFilePartsTCP(fd_set &clients);
	void AddTCPClient();
	void RemoveTCPClient(vector<CLIENT_INFO>::iterator& iter);
	void SendBlock(CLIENT_INFO clientInfo);
public:
	Server(unsigned int port = DEFAULT_PORT);
	void Run();
};

