#include "Server.h"

std::mutex Server::udpMutex;
fd_set Server::clientsSet;

Server::Server(unsigned int port)
{
	this->port = port;
	
	_udp_socket = CreateUDPSocket();
	CreateTCPSocket();
	Bind(_udp_socket);
	Bind(this->_tcp_socket);
	SetSendTimeout(this->_tcp_socket);
	SetReceiveTimeout(_udp_socket, GetTimeout(100));
	Listen();

	FD_ZERO(&this->clientsSet);
	FD_ZERO(&this->serverSet);
	FD_SET(this->_tcp_socket, &this->serverSet);
	FD_SET(_udp_socket, &this->serverSet);

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

SOCKET Server::Accept()
{
	auto client = accept(this->_tcp_socket, NULL, NULL);
	if (client == INVALID_SOCKET) {
		throw std::runtime_error(EX_ACCEPT_ERROR);
	}
	return client;
}

fpos_t Server::GetFileSize(std::fstream *file)
{
	fpos_t currentPosition = file->tellg();
	file->seekg(0, std::ios::end);
	fpos_t size = file->tellg();
	file->seekg(currentPosition);
	return size;
}

TCPMetadata Server::ExtractMetadata(std::string metadata)
{
	TCPMetadata metadata_st;
	std::string value;
	std::stringstream ss(metadata);
	std::getline(ss, value, METADATA_DELIM);
	metadata_st.fileName = value;
	std::getline(ss, value, PATH_DELIM);
	metadata_st.progress = stoll(value);
	return metadata_st;
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
			std::unique_lock<std::mutex> lock(udpMutex);
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
		auto new_thread = new std::thread(SendFile, pair);
		this->threads.push_back(new_thread);
		std::cout << "new thread created" << std::endl;
	}
	catch (std::runtime_error e)
	{
		//file not found
	}
}

void Server::SendFile(std::pair<std::mutex*, UDPMetadata*>* _pair) {
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
			std::unique_lock<std::mutex> lock(udpMutex);
			SendRawDataTo(_udp_socket, local_buffer, dataSize + UDP_NUMBER_SIZE, metadata->addr);
		}
	}
	std::cout << "UDP sending finished." << std::endl;
}

void Server::SendFilePartsUDP()
{
	//for (auto client = this->udpClients.begin(); client != this->udpClients.end(); ++client)
	//{
	//	auto metadata = *client;
	//	if (--metadata->currentDelay > 0) continue;
	//	
	//	metadata->currentDelay = metadata->delay;
	//	auto file = metadata->file;
	//	if (metadata->missedPackages.size() > 0) {
	//		file->seekg(metadata->missedPackages[0] * UDP_BUFFER_SIZE);
	//		metadata->missedPackages.erase(metadata->missedPackages.begin());
	//	}
	//	else {
	//		file->seekg(metadata->progress);
	//		metadata->progress += UDP_BUFFER_SIZE;
	//	}
	//	
	//	auto packageNumber = file->tellg() / UDP_BUFFER_SIZE;
	//	file->read(buffer, UDP_BUFFER_SIZE);
	//	auto dataSize = file->gcount();
	//	//std::cout << packageNumber << std::endl;

	//	AddNumberToDatagram(buffer, dataSize, packageNumber);
	//	//SendRawDataTo(_udp_socket, buffer, dataSize + UDP_NUMBER_SIZE, metadata->addr);
	//	
	//	if (--metadata->packagesTillDrop <= 0) {
	//		RemoveUDPClient(client);
	//		std::cout << "UDP client disconnected." << std::endl;
	//		if (client == this->udpClients.end()) break;
	//	}
	//	if (file->eof() ||
	//		(!metadata->returnAllPackages && metadata->missedPackages.size() == 0)) {
	//		RemoveUDPClient(client);
	//		std::cout << "UDP sending finished." << std::endl;
	//		if (client == this->udpClients.end()) break;
	//	}
	//}
}

void Server::RemoveUDPClient(std::vector<UDPMetadata*>::iterator& iter)
{
	/*auto clientInfo = *iter;
	clientInfo->file->close();
	delete clientInfo->file;
	delete clientInfo->addr;
	iter = this->udpClients.erase(iter);*/
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

void Server::RemoveTCPClient(std::vector<CLIENT_INFO>::iterator& iter)
{
	auto clientInfo = *iter;
	FD_CLR(clientInfo->first, &this->clientsSet);
	clientInfo->second->close();
	shutdown(clientInfo->first, SD_BOTH);
	closesocket(clientInfo->first);
	iter = this->tcpClients.erase(iter);
}

void Server::SendBlock(CLIENT_INFO clientInfo)
{
	char buffer[BUFFER_SIZE];
	auto file = clientInfo->second;
	try {
		while (file)
		{
			file->read(buffer, BUFFER_SIZE);
			size_t read = file->gcount();
			size_t realySent = SendRawData(clientInfo->first, buffer, read);
			if (realySent != read)
			{
				fpos_t pos = file->tellg();
				file->seekg(pos - (read - realySent));
			}
		}
	} catch(std::runtime_error e) {
		
	}
	FD_CLR(clientInfo->first, &clientsSet);
	file->close();
	shutdown(clientInfo->first, SD_BOTH);
	closesocket(clientInfo->first);
}

void Server::SendFilePartsTCP(fd_set& clients)
{
	for (auto client = this->tcpClients.begin(); client != this->tcpClients.end(); ++client)
	{
		if (FD_ISSET((*client)->first, &clients) > 0)
		{
			try {
				SendBlock(*client);
			}
			catch (std::runtime_error e) {
				std::cout << e.what() << std::endl;
				RemoveTCPClient(client);
				std::cout << CLIENTS_ONLINE;
				if (client == this->tcpClients.end()) break;
			}
		}
	}
}

void Server::AddTCPClient()
{
	auto client = Accept();
	auto metadata = ExtractMetadata(ReceiveMessage(client));
	auto file = new std::fstream();
	try {
		OpenFile(file, metadata.fileName);
	}
	catch (std::runtime_error e) {
		SendMessage(client, e.what());
		throw;
	}
	SendMessage(client, std::to_string(GetFileSize(file)));
	file->seekg(metadata.progress);
	FD_SET(client, &this->clientsSet);
	this->tcpClients.push_back(new std::pair<SOCKET, std::fstream*>(client, file));
	std::thread thr(SendBlock, new std::pair<SOCKET, std::fstream*>(client, file));
	thr.detach();
}

void Server::Run()
{
	auto nullDelay = new timeval();
	//nullDelay->tv_usec = 1;
	while (true)
	{
		//auto clients = this->clientsSet;
		auto servers = this->serverSet;
		auto count = select(FD_SETSIZE, &servers, NULL, NULL, /*this->udpClients.size() != 0 ? nullDelay :*/ NULL);
		//if (this->udpClients.size() != 0) SendFilePartsUDP();
		if (count <= 0) continue;
		if (FD_ISSET(this->_tcp_socket, &servers) > 0)
		{
			count--;
			AddTCPClient();
			//std::cout << CLIENTS_ONLINE;
		}
		if (FD_ISSET(_udp_socket, &servers) > 0)
		{
			count--;
			AddUDPClient();
			//std::cout << CLIENTS_ONLINE;
		}
		/*if (count > 0)
		{
			SendFilePartsTCP(clients);
		}*/
	}
}