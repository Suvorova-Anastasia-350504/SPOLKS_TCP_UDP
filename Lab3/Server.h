#pragma once
#include "Base.h"
#include "SharedMemoryManager.h"
#define CLIENTS_ONLINE "Clients online : " << this->tcpClients.size() + this->udpClients.size() << std::endl
#define CLIENT_INFO std::pair<SOCKET, std::fstream*>*

#ifdef __unix__
extern char **environ;
#endif

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
	std::string executablePath;
	SharedMemoryDescriptor tcpSharedMemory;
	SharedMemoryDescriptor udpSharedMemory;

	std::vector<std::pair<std::mutex*, UDPMetadata*>*> udpClients;

	fd_set serverSet;

protected:		
	void virtual OpenFile(std::fstream *file, std::string fileName) override;
	void Bind(SOCKET socket);	
	fpos_t GetFileSize(std::fstream *file);
	sockaddr_in* CreateAddressInfoForServer();
	void Listen();

	void StartNewProcess(std::string processType);
	
	//UDP
	void AddUDPClient();
	static void SendFileViaUDP(std::pair<std::mutex*, UDPMetadata*>* _pair);
	bool IsACK(sockaddr *client, UDPMetadata* rawMetadata);
	UDPMetadata* ExtractMetadataUDP(char* rawMetadata);

public:
	Server();
	Server(std::string executablePath, unsigned int port = DEFAULT_PORT);
	void Run();
	~Server();
};

