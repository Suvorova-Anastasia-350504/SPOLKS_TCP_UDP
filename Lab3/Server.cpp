#include "Server.h"

Server::Server(unsigned int port, bool start) : Base(port)
{
	FD_ZERO(&this->clientsSet);
	if (start) {
		Server::Init();
	}
}

void Server::Init()
{
	CreateTCPSocket();
	SetSocketTimeout();
	Bind();	
	if (listen(this->_socket, SOMAXCONN) < 0) {
		throw runtime_error(EX_LISTEN_ERROR);
	}
	FD_ZERO(&this->serverSet);
	FD_SET(this->_socket, &this->serverSet);

	cout << "Started at port: " << this->port << endl;
}

void Server::Bind()
{
	Bind(this->_socket);
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

void Server::SetSocketTimeout()
{
	Base::SetSocketTimeout(this->_socket);
}

sockaddr_in* Server::CreateAddressInfoForServer()
{
	auto addressInfo = CreateAddressInfo(DEFAULT_IP, this->port);
	addressInfo->sin_addr.s_addr = ADDR_ANY;
	return addressInfo;
}

void Server::Run()
{
	while (true)
	{
		TryToAddClient(this->clients.size() <= 0);
		SendFileParts();
	}
}

void Server::TryToAddClient(bool wait)
{
	try {
		auto client = CheckForNewConnection(wait);

		cout << "New client has arrived." << endl;

		auto metadata = ExtractMetadata(ReceiveMessage(client));
		cout << metadata.fileName << " " << metadata.progress << endl;
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

		this->clients.push_back(new pair<SOCKET, fstream*>(client, file));
		FD_SET(client, &this->clientsSet);
		cout << CLIENTS_ONLINE;
	}
	catch (NoNewClients e) {	
	}
	catch (runtime_error e) {
		cout << e.what() << endl;
	}
}

void Server::RemoveClient(vector<CLIENT_INFO>::iterator& iter)
{
	auto clientInfo = *iter;
	FD_CLR(clientInfo->first, &this->clientsSet);
	clientInfo->second->close();
	shutdown(clientInfo->first, SD_BOTH);
	closesocket(clientInfo->first);
	iter = this->clients.erase(iter);
}

void Server::SendFileParts()
{
	auto clients = this->clientsSet;
	auto count = select(FD_SETSIZE, NULL, &clients, NULL, new timeval());
	if (count > 0)
	{
		for (auto client = this->clients.begin(); client != this->clients.end(); ++client)
		{
			if (FD_ISSET((*client)->first, &clients) > 0)
			{
				try {
					SendBlock(*client);
				}
				catch (runtime_error e) {
					cout << e.what() << endl;
					RemoveClient(client);
					cout << CLIENTS_ONLINE;
					if (client == this->clients.end()) break;
				}
			}
		}
	}
	FD_ZERO(&clients);
}

SOCKET Server::CheckForNewConnection(bool wait)
{
	auto temp = this->serverSet;
	auto delay = new timeval();
	delay->tv_usec = 10000;
	if (select(FD_SETSIZE, &temp, NULL, NULL, wait ? delay : new timeval()) <= 0) {
		throw NoNewClients(EX_NO_NEW_CLIENTS);
	}

	auto client = accept(this->_socket, NULL, NULL);
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

void Server::OpenFile(fstream *file, string fileName)
{
	file->open(fileName, ios::binary | ios::in);
	if (!file->is_open())
	{
		throw runtime_error(EX_FILE_NOT_FOUND);
	}
}

void Server::SendBlock(CLIENT_INFO clientInfo)
{
	auto file = clientInfo->second;
	if (file) {
		file->read(buffer, BUFFER_SIZE);
		size_t read = file->gcount();
		size_t realySent = SendRawData(clientInfo->first, buffer, read);
		if (realySent != read) {
			fpos_t pos = file->tellg();
			file->seekg(pos - (read - realySent));
		}
	}
	else {
		throw runtime_error(EX_SENDING_DONE);
	}
}



