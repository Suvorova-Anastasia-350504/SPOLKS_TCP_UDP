#pragma once
#include "Base.h"
#define CLIENTS_ONLINE "Clients online : " << this->tcpClients.size() + this->udpClients.size() << std::endl
#define CLIENT_INFO std::pair<SOCKET, std::fstream*>*

struct TCPMetadata {
	std::string fileName;
	fpos_t progress;
};

struct UDPMetadata {
	std::string fileName;
	fpos_t progress;
	sockaddr *addr;
	fpos_t packagesTillDrop;
	std::fstream *file;
	bool requestFileSize;
	std::vector<fpos_t> missedPackages;
	fpos_t delay;
	fpos_t currentDelay;
	bool returnAllPackages;
	fpos_t lastProgress;
};

class Server : public Base
{
	std::vector<CLIENT_INFO> tcpClients;
	std::vector<std::pair<std::mutex*, UDPMetadata*>*> udpClients;
	std::vector<std::thread*> threads;
	static std::mutex udpMutex;	//mb does not need

	fd_set serverSet;
	fd_set clientsSet;
		
	void virtual OpenFile(std::fstream *file, std::string fileName) override;
	void Bind(SOCKET socket);	
	fpos_t GetFileSize(std::fstream *file);
	sockaddr_in* CreateAddressInfoForServer();

	//UDP
	void AddUDPClient();
	void SendFilePartsUDP();
	static void SendFile(std::pair<std::mutex*, UDPMetadata*>* _pair);
	
	void RemoveUDPClient(std::vector<UDPMetadata*>::iterator& iter);
	bool IsACK(sockaddr *client, UDPMetadata* rawMetadata);
	UDPMetadata* ExtractMetadataUDP(char* rawMetadata);

	//TCP
	TCPMetadata ExtractMetadata(std::string metadata);
	SOCKET Accept();
	void Listen();
	void SendFilePartsTCP(fd_set &clients);
	void AddTCPClient();
	void RemoveTCPClient(std::vector<CLIENT_INFO>::iterator& iter);
	void SendBlock(CLIENT_INFO clientInfo);
public:
	Server(unsigned int port = DEFAULT_PORT);
	void Run();
};

