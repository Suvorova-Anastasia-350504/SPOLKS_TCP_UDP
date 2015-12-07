#include "UDPSender.h"

UDPSender::UDPSender(unsigned port) {
	auto sharedMemory = SharedMemoryManager::OpenSharedMemory(sizeof(SOCKET), SHARED_MEMORY_NAME_UDP);
	this->socket = *(SOCKET*)sharedMemory.memory;
	this->port = port;
	AddUDPClient();
	this->socket = CreateUDPSocket();
	Bind(socket);
	FD_ZERO(&this->serverSet);
	SendFile();
	nullDelay.tv_usec = 10000;
}

UDPMetadata* UDPSender::ExtractMetadataUDP(char* rawMetadata)
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

void UDPSender::AddUDPClient()
{
	try
	{
		auto clientsInfo = new sockaddr();		
		auto rawMetadata = ReceiveRawDataFrom(this->socket, clientsInfo)->data;		
		auto metadata = ExtractMetadataUDP(rawMetadata);

		metadata->file = new std::fstream();
		try {
			OpenFile(metadata->file, metadata->fileName);
		}
		catch (std::runtime_error e) {
			SendMessageTo(this->socket, e.what(), clientsInfo);
			throw;
		}

		if (metadata->requestFileSize)
		{
			auto fileSize = GetFileSize(metadata->file);
			SendMessageTo(this->socket, std::to_string(fileSize) + METADATA_DELIM + to_string(port), clientsInfo);
		}
		
		metadata->file->seekg(metadata->progress);
		metadata->packagesTillDrop = PACKAGES_TILL_DROP;
		metadata->addr = clientsInfo;
		metadata->delay = 100;
		metadata->currentDelay = 1000;
		this->metadata = metadata;
	}
	catch (std::runtime_error e)
	{
		//file not found
	}
}

void UDPSender::SendFile() {
	auto file = metadata->file;
	auto notNullDelay = new timeval();
	notNullDelay->tv_sec = 500;
	while (1) {
		std::cout << "Sending" << endl;
		do {			
			//if (--metadata->currentDelay > 0) continue;
			metadata->currentDelay = metadata->delay;

			if (--metadata->packagesTillDrop <= 0) {
				std::cout << "UDP client disconnected." << std::endl;
				break;
			}
			if (metadata->missedPackages.size() > 0) {
				std::cout << metadata->missedPackages[0] * UDP_BUFFER_SIZE << endl;
				file->seekg(metadata->missedPackages[0] * UDP_BUFFER_SIZE);
				metadata->missedPackages.erase(metadata->missedPackages.begin());
			
			}
			else {
				file->seekg(metadata->progress);
				metadata->progress += UDP_BUFFER_SIZE;
			}
			cout << file->tellg() << endl;
			auto packageNumber = file->tellg() / UDP_BUFFER_SIZE;
			file->read(buffer, UDP_BUFFER_SIZE);
			auto dataSize = file->gcount();
			//cout << packageNumber << endl;
			AddNumberToDatagram(buffer, dataSize, packageNumber);
			SendRawDataTo(socket, buffer, dataSize + UDP_NUMBER_SIZE, metadata->addr);
			ACK(nullDelay);
		} while (!metadata->file->eof() && metadata->returnAllPackages || metadata->missedPackages.size() > 0);
		std::cout << "Waiting ACK" << endl;
		ACK(*notNullDelay);
		if (metadata->missedPackages.size() == 0) break;
		file->close();
		file->open(metadata->fileName);
	}
	std::cout << "finished." << endl;
}

void UDPSender::ACK(timeval delay) {

	FD_SET(socket, &serverSet);
	auto result = select(FD_SETSIZE, &serverSet, NULL, NULL, &delay);
	if (result <= 0) return;
	//cout << "ACK" << endl;
	auto newMetadata = ExtractMetadataUDP(ReceiveRawDataFrom(socket, metadata->addr)->data);
	//cout << newMetadata->missedPackages.size() << " " << newMetadata->missedPackages[0] << endl;
	metadata->packagesTillDrop = PACKAGES_TILL_DROP;
	//cout << newMetadata->missedPackages.size() << endl;
	metadata->missedPackages = newMetadata->missedPackages;
	metadata->delay = 10;
	//clientMeta->delay = (clientMeta->progress - clientMeta->lastProgress) / UDP_BUFFER_SIZE / (double)PACKAGE_COUNT;
	//metadata->lastProgress = metadata->progress;
	//metadata->currentDelay = metadata->delay;	
}