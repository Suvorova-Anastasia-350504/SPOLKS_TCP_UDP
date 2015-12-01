#include "UDPClient.h"

UDPClient::UDPClient(string address, unsigned int port) : Client(address, port)
{
	this->serverAddressInfo = (sockaddr*)CreateAddressInfoForClient();
	CreateUDPSocket();
	SetReceiveTimeout(this->_udp_socket, GetTimeout(UDP_RECV_TIMEOUT));
	for (auto i = 0; i < PACKAGE_COUNT; i++)
	{
		auto _pair = new pair<fpos_t, char*>();
		_pair->second = new char[UDP_BUFFER_SIZE];
		this->receivedBuffer.push_back(_pair);
	}
}

fpos_t UDPClient::ConnectToServer()
{
	auto connectionInfo = new char[BUFFER_SIZE];
	auto infoSize = CreateConnectionInfo(connectionInfo, BUFFER_SIZE);
	while(true)
	{
		try
		{
			SendRawDataTo(this->_udp_socket, connectionInfo, infoSize, serverAddressInfo);
			auto fileSize = ReceiveFileSize();
			return fileSize;
		} catch(ServerError e) {
			throw;
		}catch(runtime_error e)	{
			cout << e.what() << endl;
		}
	}
}

void UDPClient::DownloadFile(string fileName)
{
	file = new fstream();
	this->fileName = fileName;
	OpenFile(file, GetLocalFileName(fileName));
	fpos_t currentProgress = file->tellp();	
	fileSize = ConnectToServer();
	cout << "Download started. " << fileSize << endl;
	auto timer = new SpeedRater(currentProgress);
	ProgressHolder *progressHolder = new ProgressHolder(0, fileSize, "file");
	auto lastProgress = 0;
	auto done = fileSize <= currentProgress;
	fpos_t currentBatch = 0;

	InitMissingPackages();

	while (true) {
		try {
			ProcessBatches(file, fileSize);
		} catch (ServerError e) {
			cout << e.what() << endl;
			throw;
		} catch (runtime_error e) {
		}
		cout << "PROCESS BATCHES ENDED";
		if (missingPackages.size() == 0) {
			break;
		}
		SendMissingPackages();
	}

	//TODO: Implement write considering lost packages 
	ShowProgress(0, fileSize, fileSize, timer);
	//progressHolder->logFinish();

	file->close();
	cout << "Done." << endl;
}

fpos_t UDPClient::GetNumber(Package* package)
{
	return Base::GetNumber(package->data, package->size - UDP_NUMBER_SIZE);
}

fpos_t UDPClient::ReceiveFileSize()
{
	auto answer = ReceiveMessageFrom(this->_udp_socket, this->serverAddressInfo);
	return StringToFileSize(answer);
}

string UDPClient::CreateFileInfo(string fileName, fpos_t pos, int packageCount, bool request)
{
	return fileName + METADATA_DELIM + to_string(pos) + METADATA_DELIM + to_string(packageCount) + METADATA_DELIM + (request ? "1" : "0");
}

void UDPClient::ProcessBatches(fstream* file, fpos_t fileSize)
{
	fpos_t filePos = file->tellp();
	fpos_t packageNumber;
	fpos_t packageCount;
	
	Package *package;
	//fstream log("log.txt", ios::out);
	while (true) {
		packageCount = 0;
		
		while (true) {
			//try {
			package = ReceiveRawDataFrom(this->_udp_socket, this->serverAddressInfo);
			
			packageNumber = GetNumber(package);
			//log << packageNumber << endl;
			if (packageNumber * UDP_BUFFER_SIZE != filePos) {
				file->seekp(packageNumber * UDP_BUFFER_SIZE);
				filePos = packageNumber * UDP_BUFFER_SIZE;
			}
			
			file->write(package->data, package->size - UDP_NUMBER_SIZE);
			filePos += UDP_BUFFER_SIZE;
			
			RemoveFromMissingPackages(packageNumber);
			//log << missingPackages.size() << endl;
			if (++packageCount >= PACKAGE_COUNT)
			{
				break;
			}
		}
			//SendMessageTo(this->_udp_socket, ACK, this->serverAddressInfo);
		if (missingPackages.size() > 0) {
			SendMissingPackages(packageNumber);
			//SEND ASK
			//cout << "SEND ASK" << endl;
			//SendMissingPackages();
			//log.close();
			//throw 1;
		} else {
			//SendMessageTo(this->_udp_socket, ACK, this->serverAddressInfo);
			break;
		}
	}
}

void UDPClient::RemoveFromMissingPackages(fpos_t index) 
{
	//remove element with [index] value from vector
	
	auto item = find(missingPackages.begin(),
		missingPackages.end(),
		index
		);
	if (item != missingPackages.end()) {	
		missingPackages.erase(item);
	}	
}


void UDPClient::AddBatchToMissingPackages(fpos_t batch) 
{
	for (fpos_t i = batch * PACKAGE_COUNT; i < (batch + 1) * PACKAGE_COUNT; i++) 
	{
		missingPackages.push_back(i);
	}
}

void UDPClient::SendMissingPackages()  
{
	//auto *message = new char[UDP_BUFFER_SIZE];
	auto dataSize = CreateMissingPackagesInfo(buffer, UDP_BUFFER_SIZE);
	SendRawDataTo(this->_udp_socket, buffer, dataSize, serverAddressInfo);
}

fpos_t UDPClient::CreateMissingPackagesInfo(char* buffer, fpos_t bufferSize, bool requestAllPackages)
{
	//TODO: check too much missing packages
	auto metadataSize = fileName.size() + sizeof(char) * 2 + UDP_NUMBER_SIZE;
	auto maxPackageCount = (UDP_BUFFER_SIZE - metadataSize) / UDP_NUMBER_SIZE;
	auto packagesCount = maxPackageCount > missingPackages.size() ? missingPackages.size() : maxPackageCount;
	auto dataSize = metadataSize + packagesCount * UDP_NUMBER_SIZE;
	auto index = 0;
	index += fileName.copy(buffer, fileName.size());
	buffer[index++] = METADATA_DELIM;
	//requestFileSize = false
	buffer[index++] = 0;
	AddNumberToDatagram(buffer, index, requestAllPackages ? REQUEST_ALL_PACKAGES : packagesCount);
	index += UDP_NUMBER_SIZE;
	cout << packagesCount << endl;
	for (auto i = missingPackages.begin(); i != missingPackages.begin() + packagesCount; ++i) {
		AddNumberToDatagram(buffer, index, (*i));
		index += UDP_NUMBER_SIZE;
	}
	return dataSize;
}

fpos_t UDPClient::CreateConnectionInfo(char* buffer, fpos_t bufferSize)  
{
	auto dataSize = fileName.size() + sizeof(char) * 2 + UDP_NUMBER_SIZE;
	if (bufferSize < dataSize) {
		return -1;
	}
	auto index = 0;
	index += fileName.copy(buffer, fileName.size());
	buffer[index++] = METADATA_DELIM;
	//requestFileSize
	buffer[index++] = 1;
	AddNumberToDatagram(buffer, index, REQUEST_ALL_PACKAGES);
	return dataSize;
}

void UDPClient::InitMissingPackages()  
{
	for (fpos_t i = 0; i <= ceil((double)(fileSize/UDP_BUFFER_SIZE)); i++) {
		missingPackages.push_back(i);
	}
}

void UDPClient::SendMissingPackages(fpos_t lastReceivedPackage)
{
	auto *message = new char[UDP_BUFFER_SIZE];
	auto dataSize = CreateMissingPackagesInfo(message, UDP_BUFFER_SIZE, lastReceivedPackage);
	SendRawDataTo(this->_udp_socket, message, dataSize, serverAddressInfo);
}
fpos_t UDPClient::CreateMissingPackagesInfo(char* buffer, fpos_t bufferSize, fpos_t lastReceivedPackage, bool requestAllPackages)
{
	//TODO: check too much missing packages
	auto metadataSize = fileName.size() + sizeof(char)* 2 + UDP_NUMBER_SIZE;
	auto maxPackageCount = (UDP_BUFFER_SIZE - metadataSize) / UDP_NUMBER_SIZE;
	auto packagesCount = maxPackageCount > missingPackages.size() ? missingPackages.size() : maxPackageCount;
	
	int realPackageCount = 0; 
	for (auto i = missingPackages.begin(); i != missingPackages.begin() + packagesCount, *i < lastReceivedPackage; ++i) {
		realPackageCount++;
	}

	auto dataSize = metadataSize + packagesCount * UDP_NUMBER_SIZE;
	auto index = 0;
	index += fileName.copy(buffer, fileName.size());
	buffer[index++] = METADATA_DELIM;
	//requestFileSize = false
	buffer[index++] = 0;
	AddNumberToDatagram(buffer, index, requestAllPackages ? REQUEST_ALL_PACKAGES : realPackageCount);
	index += UDP_NUMBER_SIZE;
	cout << packagesCount << endl;

	for (auto i = missingPackages.begin(); i != missingPackages.begin() + realPackageCount; ++i) {
		AddNumberToDatagram(buffer, index, (*i));
		index += UDP_NUMBER_SIZE;
	}
	return dataSize;

}
