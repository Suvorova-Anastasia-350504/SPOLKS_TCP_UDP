#pragma once
#include "Base.h"
#include "SharedMemoryManager.h"
#define CLIENTS_ONLINE "Clients online : " << this->tcpClients.size() + this->udpClients.size() << std::endl
#define CLIENT_INFO std::pair<SOCKET, std::fstream*>*

#ifdef __unix__
extern char **environ;
#endif

struct UDPMetadata;

class Server : public Base
{
	std::string executablePath;
	SharedMemoryDescriptor tcpSharedMemory;
	SharedMemoryDescriptor udpSharedMemory;
	int nextFreePort;
protected:		
	fd_set serverSet;
	void virtual OpenFile(std::fstream *file, std::string fileName) override;
	void Bind(SOCKET socket);	
	fpos_t GetFileSize(std::fstream *file);
	sockaddr_in* CreateAddressInfoForServer();
	void Listen();
	void StartNewProcess(std::string processType);

public:
	Server();
	Server(std::string executablePath, unsigned int port = DEFAULT_PORT);
	void Run();
	~Server();
};

