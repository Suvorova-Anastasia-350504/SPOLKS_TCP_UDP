#include "Server.h"

Server::Server() {
}

Server::Server(std::string executablePath, unsigned int port)
{
	this->port = port;
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

UDPMetadata* Server::ExtractMetadataUDP(char* rawMetadata)
{
	auto metadata = new UDPMetadata();
	auto index = 0;
	while (rawMetadata[index] != METADATA_DELIM) metadata->fileName += rawMetadata[index++];
	metadata->requestFileSize = rawMetadata[++index] == 1;
	auto missedPackagesCount = GetNumber(rawMetadata, ++index);
	metadata->returnAllPackages = missedPackagesCount == REQUEST_ALL_PACKAGES;
	if (metadata->returnAllPackages) return metadata;
	for (auto count = 0; count < missedPackagesCount; count++) {
		index += UDP_NUMBER_SIZE;
		metadata->missedPackages.push_back(GetNumber(rawMetadata, index));
	}
	std::cout << metadata->missedPackages.size() << "  " << metadata->missedPackages[0] << std::endl;

	return metadata;
}

void Server::AddUDPClient()
{
	try
	{		
		//udpMutex.lock();
		auto clientsInfo = new sockaddr();
		char *rawMetadata;
		{
			//std::unique_lock<std::mutex> lock(udpMutex);
			rawMetadata = ReceiveRawDataFrom(this->_udp_socket, clientsInfo)->data;
		}
		auto metadata = ExtractMetadataUDP(rawMetadata);
		
		if (IsACK(clientsInfo, metadata)/* || memcmp(rawMetadata, ACK, 3) == 0*/) return;
		std::cout << "not ACK" << std::endl;
		metadata->file = new std::fstream();
		try	{
			OpenFile(metadata->file, metadata->fileName);
		} catch (std::runtime_error e) {
			SendMessageTo(this->_udp_socket, e.what(), clientsInfo);
			throw;
		}

		if (metadata->requestFileSize)
		{
			auto fileSize = GetFileSize(metadata->file);
			SendMessageTo(this->_udp_socket, std::to_string(fileSize), clientsInfo);
		}
		//udpMutex.unlock();
		metadata->file->seekg(metadata->progress);
		metadata->packagesTillDrop = PACKAGES_TILL_DROP;
		metadata->addr = clientsInfo;
		metadata->delay = 100;
		metadata->currentDelay = 1000;

		auto pair = new std::pair<std::mutex*, UDPMetadata*>(new std::mutex(), metadata);
		this->udpClients.push_back(pair);
		auto new_thread = new std::thread(SendFileViaUDP, pair);
		//this->threads.push_back(new_thread);
		std::cout << "new thread created" << std::endl;
	}
	catch (std::runtime_error e)
	{
		//file not found
	}
}

void Server::SendFileViaUDP(std::pair<std::mutex*, UDPMetadata*>* _pair) {
	//auto mutex = &_pair->first;
	auto local_buffer = new char[UDP_BUFFER_SIZE];
	auto metadata = _pair->second;
	auto file = metadata->file;
	while (!metadata->file->eof() && metadata->returnAllPackages || metadata->missedPackages.size() > 0) 
	{
		{
			std::unique_lock<std::mutex> lock(*_pair->first);

			if (--metadata->currentDelay > 0) continue;
			metadata->currentDelay = metadata->delay;

			if (--metadata->packagesTillDrop <= 0) {
				//RemoveUDPClient(client);
				std::cout << "UDP client disconnected." << std::endl;
				break;
				//if (client == udpClients.end()) break;
			}
			
			/*if (metadata->missedPackages.size() > 0 )
				std::cout << metadata->missedPackages.size() << std::endl;*/
			if (metadata->missedPackages.size() > 0) {
				file->seekg(metadata->missedPackages[0] * UDP_BUFFER_SIZE);
				metadata->missedPackages.erase(metadata->missedPackages.begin());
			}
			else {
				//file->seekg(metadata->progress);
				file->seekg(metadata->progress);
				metadata->progress += UDP_BUFFER_SIZE;
			}
		}

		auto packageNumber = file->tellg() / UDP_BUFFER_SIZE;
		std::cout << packageNumber << std::endl;
		file->read(local_buffer, UDP_BUFFER_SIZE);
		auto dataSize = file->gcount();

		AddNumberToDatagram(local_buffer, dataSize, packageNumber);

		{
			//std::unique_lock<std::mutex> lock(udpMutex);
			SendRawDataTo(_udp_socket, local_buffer, dataSize + UDP_NUMBER_SIZE, metadata->addr);
		}
	}
	std::cout << "UDP sending finished." << std::endl;
}

bool Server::IsACK(sockaddr* client, UDPMetadata* metadata) {

	std::cout << "ACK" << std::endl;
	auto _client = find_if(this->udpClients.begin(), this->udpClients.end(),
		[&](std::pair<std::mutex*, UDPMetadata*> *pair)
	{
		return memcmp(pair->second->addr->sa_data, client->sa_data, 14) == 0;
	});
	if (_client != this->udpClients.end()) {	//ACK

		//std::cout << "1" << std::endl;
		//(*_client)->first->lock();
		std::unique_lock<std::mutex> lock(*(*_client)->first);
		auto clientMeta = (*_client)->second;
		if (clientMeta->file->eof() && clientMeta->returnAllPackages || 
			!clientMeta->returnAllPackages && clientMeta->missedPackages.size() == 0 ||
			clientMeta->packagesTillDrop <= 0) 
		{
			this->udpClients.erase(_client);
			return false;
		}
		//std::cout << "2" << std::endl;
		clientMeta->packagesTillDrop = PACKAGES_TILL_DROP;
		clientMeta->missedPackages = metadata->missedPackages;
		clientMeta->delay = 1;
		//clientMeta->delay = (clientMeta->progress - clientMeta->lastProgress) / UDP_BUFFER_SIZE / (double)PACKAGE_COUNT;

		clientMeta->lastProgress = clientMeta->progress;
		clientMeta->currentDelay = clientMeta->delay;

		return true;
	}
	return false;
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
			Sleep(1000);	//delay to ensure the child accepts connection
		}
		if (FD_ISSET(_udp_socket, &servers) > 0)
		{
			StartNewProcess(UDP);
			Sleep(100000);	// anti-BSOD
		}
	}
}

Server::~Server() {
	SharedMemoryManager::RemoveSharedMemory(tcpSharedMemory);
	SharedMemoryManager::RemoveSharedMemory(udpSharedMemory);
}