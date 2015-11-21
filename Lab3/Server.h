#pragma once
#include "Base.h"
#define CLIENTS_ONLINE "Clients online : " << this->tcpClients.size() + this->udpClients.size() << endl
#define CLIENT_INFO pair<SOCKET, fstream*>*

struct TCPMetadata {
	string fileName;
	fpos_t progress;
};

struct UDPMetadata {
	string fileName;
	fpos_t progress;
	sockaddr *addr;
	fpos_t packagesTillDrop;
	fstream *file;
	bool requestFileSize;
	vector<fpos_t> missedPackages;
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
	sockaddr_in* CreateAddressInfoForServer();

	//UDP
	void AddUDPClient();
	void SendFilePartsUDP();
	void AddNumberToDatagram(char *buffer, fpos_t size, fpos_t number);
	void RemoveUDPClient(vector<UDPMetadata*>::iterator& iter);
	bool IsACK(sockaddr *client);
	UDPMetadata* ExtractMetadataUDP(char* rawMetadata);
	//TCP
	TCPMetadata ExtractMetadata(string metadata);
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

