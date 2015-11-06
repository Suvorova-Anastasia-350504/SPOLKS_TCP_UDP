#include "Server.h"

Server::Server(unsigned int port)
{
	this->port = port;
	
	CreateUDPSocket();
	CreateTCPSocket();
	Bind(this->_udp_socket);
	Bind(this->_tcp_socket);
	SetSendTimeout(this->_tcp_socket);
	Listen();

	FD_ZERO(&this->clientsSet);
	FD_ZERO(&this->serverSet);
	FD_SET(this->_tcp_socket, &this->serverSet);
	FD_SET(this->_udp_socket, &this->serverSet);

	cout << "Server started at port: " << this->port << endl;
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
	auto result = bind(socket, (sockaddr*)serverAddress, sizeof(*serverAddress));
	if (result == SOCKET_ERROR)
	{
		throw runtime_error(EX_BIND_ERROR);
	}
}

SOCKET Server::Accept()
{
	auto client = accept(this->_tcp_socket, NULL, NULL);
	if (client == INVALID_SOCKET) {
		throw runtime_error(EX_ACCEPT_ERROR);
	}
	return client;
}

fpos_t Server::GetFileSize(fstream *file)
{
	fpos_t currentPosition = file->tellg();
	file->seekg(0, ios::end);
	fpos_t size = file->tellg();
	file->seekg(currentPosition);
	return size;
}

FileMetadata Server::ExtractMetadata(string metadata)
{
	FileMetadata metadata_st;
	string value;
	stringstream ss;
	ss << metadata;
	getline(ss, value, '|');
	metadata_st.fileName = value;
	getline(ss, value, '\\');
	metadata_st.progress = stoll(value);
	return metadata_st;
}

void Server::ProcessUDPClient()
{
	try
	{		
		auto clientsInfo = new sockaddr();
		auto metadata = ExtractMetadata(ReceiveMessageFrom(this->_udp_socket, clientsInfo));
		fstream *file;
		try
		{
			file = GetFile(metadata.fileName);
		} catch (runtime_error e)
		{
			SendMessageTo(this->_udp_socket, e.what(), clientsInfo);
			throw;
		}		
		//SendMessageTo(this->_udp_socket, to_string(GetFileSize(file)), clientsInfo);
		file->seekg(metadata.progress);
		file->read(buffer, UDP_BUFFER_SIZE);
		SendRawDataTo(this->_udp_socket, buffer, file->gcount(), clientsInfo);
	}
	catch (runtime_error e)
	{
	}
}

void Server::Listen()
{
	if (listen(this->_tcp_socket, SOMAXCONN) < 0) {
		throw runtime_error(EX_LISTEN_ERROR);
	}
}

fstream* Server::GetFile(string fileName)
{
	auto file = new fstream();
	auto iter = find_if(this->files.begin(), this->files.end(), [&](pair<string, fstream*>* item)
	{
		return item->first == fileName;
	});
	if (iter != this->files.end())
	{
		file = (*iter)->second;
	}
	else
	{
		OpenFile(file, fileName);
		this->files.push_back(new pair<string, fstream*>(fileName, file));
	}
	return file;
}

void Server::OpenFile(fstream *file, string fileName)
{
	file->open(fileName, ios::binary | ios::in);
	if (!file->is_open())
	{
		throw runtime_error(EX_FILE_NOT_FOUND);
	}
}

void Server::RemoveTCPClient(vector<CLIENT_INFO>::iterator& iter)
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
	auto file = clientInfo->second;
	if (file)
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
	else
	{
		throw runtime_error(EX_SENDING_DONE);
	}
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
			catch (runtime_error e) {
				cout << e.what() << endl;
				RemoveTCPClient(client);
				cout << CLIENTS_ONLINE;
				if (client == this->tcpClients.end()) break;
			}
		}
	}
}

void Server::AddNewTCPClient()
{
	auto client = Accept();
	auto metadata = ExtractMetadata(ReceiveMessage(client));
	auto file = new fstream();
	try {
		OpenFile(file, metadata.fileName);
	}
	catch (runtime_error e) {
		SendMessage(client, e.what());
		throw;
	}
	SendMessage(client, to_string(GetFileSize(file)));
	file->seekg(metadata.progress);

	this->tcpClients.push_back(new pair<SOCKET, fstream*>(client, file));
	FD_SET(client, &this->clientsSet);
}

void Server::Run()
{
	while (true)
	{
		auto clients = this->clientsSet;
		auto servers = this->serverSet;

		auto count = select(FD_SETSIZE, &servers, &clients, NULL, NULL);
		if (count <= 0) continue;
		if (FD_ISSET(this->_tcp_socket, &servers) > 0)
		{
			count--;
			AddNewTCPClient();
			cout << CLIENTS_ONLINE;
		}
		if (FD_ISSET(this->_udp_socket, &servers) > 0)
		{
			count--;
			ProcessUDPClient();
		}
		if (count > 0)
		{
			SendFilePartsTCP(clients);
		}
	}
}