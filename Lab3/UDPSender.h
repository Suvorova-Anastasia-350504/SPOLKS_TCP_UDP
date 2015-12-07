#pragma once
#include "Server.h"

using namespace std;

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

class UDPSender : public Server
{
	SOCKET socket;
	UDPMetadata *metadata;
	timeval nullDelay;

	void AddUDPClient();
	void SendFile();
	void ACK(timeval delay);
	UDPMetadata* ExtractMetadataUDP(char* rawMetadata);
public:
	UDPSender(unsigned port);
};

