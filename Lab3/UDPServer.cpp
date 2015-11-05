#include "UDPServer.h"
#include <bitset>

UDPServer::UDPServer(unsigned int port) : Server(port, true)
{
	this->serverAddressInfo = CreateAddressInfoForServer();
	Init();
}

void UDPServer::Init()
{
	CreateUDPSocket();
	Server::SetSocketTimeout();
	Bind(this->_udp_socket);
	FD_ZERO(&updServerSet);
	FD_SET(this->_udp_socket, &this->updServerSet);
	cout << "UDP started at port: " << this->port << endl;
}

void UDPServer::ProcessTCPClients()
{
	Server::TryToAddClient(this->clients.size() == 0 && this->udpClients.size() == 0);
	Server::SendFileParts();
}

void UDPServer::ProcessUDPClients()
{
	TryAddNewUDPClient();
	//TODO : add handshakes
	UDPServer::SendFileParts();
}

void UDPServer::TryAddNewUDPClient()
{
	try
	{
		/*auto clientsInfo = new sockaddr();
		auto fileMetadata = TryReceiveNewMessage(clientsInfo);
		auto file = new fstream();
		auto iter = find_if(this->udpClients.begin(), this->udpClients.end(), [&](pair<sockaddr*, fstream*>* item)
		{
			return item->first->sa_data == clientsInfo->sa_data;
		});
		if (iter != this->udpClients.end())
		{
			file = (*iter)->file;
		} else
		{
			OpenFile(file, fileMetadata.fileName);
			SendMessageTo(this->_udp_socket, to_string(GetFileSize(file)), clientsInfo);
			auto client = new ClientInfo(clientsInfo, file);
			this->udpClients.push_back(client);
		}
		file->seekg(fileMetadata.progress);
		cout << "New Client added" << endl;*/
	}
	catch (runtime_error e)
	{
	}
}

FileMetadata UDPServer::TryReceiveNewMessage(sockaddr *clientsInfo)
{
	auto serverSet = this->updServerSet;
	auto count = select(FD_SETSIZE, &serverSet, NULL, NULL, new timeval());
	if (count <= 0) throw runtime_error(EX_NO_NEW_CLIENTS);
	return ExtractMetadata(ReceiveMessageFrom(this->_udp_socket, clientsInfo));
}

void UDPServer::SendFileParts()
{
	for (auto clientInfo = this->udpClients.begin(); clientInfo != this->udpClients.end(); ++clientInfo)
	{
		auto file = (*clientInfo)->file;
		auto number = file->tellg();
		file->read(buffer, UDP_BUFFER_SIZE);
		
		auto count = file->gcount();
		AddNumberToDatagram(buffer, count, number);
		SendRawDataTo(this->_udp_socket, buffer, count + UDP_NUMBER_SIZE, (*clientInfo)->addr);
		if (file->eof())
		{
			file->close();
			delete (*clientInfo)->addr;
			clientInfo = this->udpClients.erase(clientInfo);
			if (clientInfo == this->udpClients.end()) break;
		}
	}
}

void UDPServer::AddNumberToDatagram(char* buffer, fpos_t size, fpos_t number)
{
	for (fpos_t i = 0xFF, j = 0; j < UDP_NUMBER_SIZE - 1; i = i << 8, j++)
	{
		auto byte = (number & i) >> j*8;
		buffer[size + j] = byte;
	}
}

void UDPServer::Run()
{
	while (true)
	{
		ProcessTCPClients();
		//ProcessUDPClients();
	}
}