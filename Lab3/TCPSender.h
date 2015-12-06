#pragma once
#include "Base.h"
#include "SharedMemoryManager.h"
#include "Server.h"


class TCPSender: public Server
{
	SOCKET socket;
	std::fstream *file;
	void AcceptConnection();
	void SendFile();
	SOCKET Accept();
	TCPMetadata ExtractMetadata(std::string metadata);
public:
	TCPSender();

};

