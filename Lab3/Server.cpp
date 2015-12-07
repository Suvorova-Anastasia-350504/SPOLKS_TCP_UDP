#include "Server.h"

Server::Server() {
}

Server::Server(std::string executablePath, unsigned int port)
{
	this->port = port;
	this->nextFreePort = port + 1;
	this->executablePath = executablePath;

	_udp_socket = CreateUDPSocket();
	CreateTCPSocket();
	Bind(_udp_socket);
	Bind(this->_tcp_socket);
	SetSendTimeout(this->_tcp_socket);
	SetReceiveTimeout(_udp_socket, GetTimeout(100));
	Listen();

	FD_ZERO(&this->serverSet);
	FD_SET(this->_tcp_socket, &this->serverSet);
	FD_SET(_udp_socket, &this->serverSet);

	tcpSharedMemory = SharedMemoryManager::CreateSharedMemory(sizeof(SOCKET), SHARED_MEMORY_NAME_TCP);
	memcpy(tcpSharedMemory.memory, &_tcp_socket, sizeof(SOCKET));
	udpSharedMemory = SharedMemoryManager::CreateSharedMemory(sizeof(SOCKET), SHARED_MEMORY_NAME_UDP);
	memcpy(udpSharedMemory.memory, &_udp_socket, sizeof(SOCKET));

	std::cout << "Server started at port: " << this->port << std::endl;
}

sockaddr_in* Server::CreateAddressInfoForServer()
{
	auto addressInfo = CreateAddressInfo(DEFAULT_IP, this->port);
	addressInfo->sin_addr.s_addr = ADDR_ANY;
	return addressInfo;
}

void Server::Bind(SOCKET socket)
{
	auto serverAddress = CreateAddressInfoForServer();
	int result = bind(socket, (sockaddr*)serverAddress, sizeof(*serverAddress));
	if (result == SOCKET_ERROR)
	{
		throw std::runtime_error(EX_BIND_ERROR);
	}
}

fpos_t Server::GetFileSize(std::fstream *file)
{
	fpos_t currentPosition = file->tellg();
	file->seekg(0, std::ios::end);
	fpos_t size = file->tellg();
	file->seekg(currentPosition);
	return size;
}

void Server::Listen()
{
	if (listen(this->_tcp_socket, SOMAXCONN) < 0) {
		throw std::runtime_error(EX_LISTEN_ERROR);
	}
}

void Server::OpenFile(std::fstream *file, std::string fileName)
{
	file->open(fileName, std::ios::binary | std::ios::in);
	if (!file->is_open())
	{
		throw std::runtime_error(EX_FILE_NOT_FOUND);
	}
}

void Server::StartNewProcess(std::string processType)
{
	auto commandLine = std::string(" ") + CHILD + std::string(" ") + processType;
	PROCESS_INFORMATION process;
#ifdef _WIN32
	STARTUPINFOA si;
	memset(&si, 0, sizeof(si));
	si.cb = sizeof(si);
	CreateProcess(executablePath.c_str(), LPSTR(commandLine.c_str()), nullptr,
		nullptr, TRUE, 0, nullptr, nullptr, &si, &process);
#else
	posix_spawn(&process, executablePath.c_str(), nullptr, nullptr, commandLine.c_str(), environ);
#endif
}

void Server::Run()
{
	while (true)
	{
		auto servers = this->serverSet;
		auto count = select(FD_SETSIZE, &servers, NULL, NULL, NULL);
		if (count <= 0) continue;
		if (FD_ISSET(this->_tcp_socket, &servers) > 0)
		{
			StartNewProcess(TCP);
			Sleep(1000);	//delay to ensure the child accepts connection // while(!sharedFlag)
		}
		if (FD_ISSET(_udp_socket, &servers) > 0)
		{
			StartNewProcess(UDP + std::string(" ") + std::to_string(nextFreePort++));
			Sleep(1000);	//delay to ensure the child accepts connection // while(!sharedFlag)
		}
	}
}

Server::~Server() {
	SharedMemoryManager::RemoveSharedMemory(tcpSharedMemory);
	SharedMemoryManager::RemoveSharedMemory(udpSharedMemory);
}