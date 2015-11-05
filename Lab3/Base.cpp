#include "Base.h"

Base::Base(unsigned int port) : port(port)
{
	this->buffer = new char[BUFFER_SIZE + 1];
}

void Base::Close()
{	
	if (this->_socket != INVALID_SOCKET)
	{
		shutdown(this->_socket, SD_BOTH);
		closesocket(this->_socket);
	}
}

Base::~Base()
{
	this->Close();
}

void Base::CreateTCPSocket()
{
	this->_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (this->_socket == INVALID_SOCKET)
	{
		throw runtime_error(EX_SOCKET_ERROR);
	}
}

void Base::CreateUDPSocket()
{
	this->_udp_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (this->_udp_socket == INVALID_SOCKET)
	{
		throw runtime_error(EX_SOCKET_ERROR);
	}
}

void Base::SetSocketTimeout(SOCKET socket)
{
	auto timeout = GetTimeout(SERVER_TIMEOUT);
	auto result = setsockopt(socket, SOL_SOCKET, SO_SNDTIMEO, (const char*)&timeout, sizeof(timeout));
	if (result == SOCKET_ERROR) {
		throw runtime_error(EX_SET_TIMEOUT_ERROR);
	}
}

TIME_STRUCT Base::GetTimeout(unsigned time)
{
	TIME_STRUCT timeout;
#ifdef _WIN32
	timeout = time;
#else
	timeout.tv_sec = time;
	timeout.tv_usec = 0;
#endif
	return timeout;
}

sockaddr_in* Base::CreateAddressInfo(string address, unsigned int port)
{
	auto addressInfo = new sockaddr_in();
	addressInfo->sin_family = AF_INET;
	addressInfo->sin_port = htons(port);
    auto saddrinfo = new struct addrinfo();
    saddrinfo->ai_family = AF_INET;
    if (getaddrinfo(address.c_str(), NULL, saddrinfo, &saddrinfo) != 0)
	{
		throw runtime_error(EX_WRONG_IP);
	}
    addressInfo->sin_addr = ((sockaddr_in*)saddrinfo->ai_addr)->sin_addr;
	//inet_pton(AF_INET, address.c_str(), &(addressInfo->sin_addr.s_addr));
	return addressInfo;
}

void Base::SendMessageTo(SOCKET socket, string message, sockaddr* to)
{
	message += "\n";
	SendRawDataTo(socket, message.c_str(), message.size(), to);
}

size_t Base::SendRawData(SOCKET socket, const char *data, size_t size)
{
	auto result = send(socket, data, size, NULL);
	if (result == SOCKET_ERROR || result == 0)
	{
		throw runtime_error(EX_SENDING_FAILED);
	}
	return result;
}

size_t Base::SendRawDataTo(SOCKET socket, const char* data, size_t size, sockaddr* to)
{
	auto result = sendto(socket, data, size, NULL, to, sizeof(*to));
	if (result == SOCKET_ERROR || result == 0)
	{
		throw runtime_error(EX_SENDING_FAILED);
	}
	return result;
}

void Base::SendMessage(SOCKET socket, string message)
{
	message += "\n";
	this->SendRawData(socket, message.c_str(), message.size());
}

string Base::ReceiveMessage(SOCKET socket)
{
	Package *package = NULL;
	char *newString = NULL;
	do {
		package = this->PeekRawData(socket);
		newString = strchr(package->data, '\n');
	} while (newString == NULL);
	package = this->ReceiveRawData(socket, newString - package->data + 1);
	return string(package->data);
}

string Base::ReceiveMessageFrom(SOCKET socket, sockaddr* from)
{
	Package *package = NULL;
	package = ReceiveRawDataFrom(socket, from, UDP_BUFFER_SIZE, NULL);
	return string(package->data);
}

void Base::CheckRecvResult(int result)
{
	if (result > 0)	{
		return;
	}
	if (result == 0) {
		throw runtime_error(EX_CONNECTION_CLOSED);
	}
	throw runtime_error(EX_CONNECTION_INTERRUPTED);
}

Package* Base::PeekRawData(SOCKET socket)
{
	return ReceiveRawData(socket, BUFFER_SIZE, MSG_PEEK);
}

Package* Base::ReceiveRawData(SOCKET socket, int size, int flags)
{
	auto result = recv(socket, buffer, size, flags);
	CheckRecvResult(result);
	buffer[result] = '\0';
	auto package = new Package(buffer, result);
	return package;
}

Package* Base::ReceiveRawDataFrom(SOCKET socket, sockaddr* from, int size, int flags)
{
#ifdef _WIN32
    auto len = int(sizeof(*from));
#else
    auto len = sizeof(*from);
#endif
	auto result = recvfrom(socket, buffer, size, flags, from, &len);
	CheckRecvResult(result);
	buffer[result] = '\0';
	auto package = new Package(buffer, result);
	return package;
}

