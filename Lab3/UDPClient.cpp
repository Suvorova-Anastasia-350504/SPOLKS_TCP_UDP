#include "UDPClient.h"

UDPClient::UDPClient(string address, unsigned int port) : Client(address, port)
{
	this->serverAddressInfo = (sockaddr*)CreateAddressInfoForClient();
	CreateUDPSocket();
	SetReceiveTimeout(this->_udp_socket, GetTimeout(1));
}

void UDPClient::DownloadFile(string fileName)
{
	auto file = new fstream();
	OpenFile(file, GetLocalFileName(fileName));
	fpos_t currentProgress = file->tellp();
	SendMessageTo(this->_udp_socket, CreateFileInfo(fileName, currentProgress, PACKAGE_COUNT), this->serverAddressInfo);
	auto fileSize = ReceiveFileSize();
	//GenerateExpectedPackages(currentProgress, fileSize);

	cout << "Download started." << endl;
	auto done = false;
	while(!done)
	{
		try {
			ReceiveFile(file, currentProgress, fileSize);
			done = true;
		}
		catch (ServerError e) {
			cout << e.what() << endl;
			break;
		} 
		catch (ConnectionInterrupted e) {
			currentProgress = e.GetProgress();
			cout << this->expectedPackages.size() << endl;
			//SendMessageTo(this->_udp_socket, CreateFileInfo(fileName, this->expectedPackages[0], 0), this->serverAddressInfo);
			//SendMessageTo(this->_udp_socket, CreateFileInfo(fileName, , PACKAGE_COUNT), this->serverAddressInfo);
			fileSize = ReceiveFileSize();
		}
	}
	cout << "Done." << endl;
}

void UDPClient::ReceiveFile(fstream *file, fpos_t currentProgress, fpos_t fileSize)
{
	auto timer = new SpeedRater(currentProgress);
	auto lastProgress = ShowProgress(0, currentProgress, fileSize, timer);
	fpos_t count = 0;
	while (currentProgress < fileSize)
	{
		try {
			auto package = ReceiveRawDataFrom(this->_udp_socket, this->serverAddressInfo);
			auto number = GetNumber(package);
			//RemoveExpectedPackage(number);
			auto dataSize = package->size - UDP_NUMBER_SIZE;
			if (fpos_t(file->tellp()) != number) file->seekp(number);
			file->write(package->data, dataSize);
			currentProgress += dataSize;
			lastProgress = ShowProgress(lastProgress, currentProgress, fileSize, timer);
			count++;
		}
		catch (runtime_error e) {
			cout << e.what() << endl;
			cout << count << endl;
			file->flush();
			delete timer;
			throw ConnectionInterrupted(currentProgress);
		}
	}
	delete timer;
}

fpos_t UDPClient::GetNumber(Package* package)
{
	fpos_t result = 0;
	auto packageSize = package->size - UDP_NUMBER_SIZE;
	for (fpos_t i = 0xFF, j = 0; j < UDP_NUMBER_SIZE - 1; j++, i <<= 8)
	{
		auto byte = unsigned char(package->data[packageSize + j]);
		result += (byte << j * 8);
	}
	return result;
}

void UDPClient::GenerateExpectedPackages(fpos_t start, fpos_t fileSize)
{
	for (fpos_t i = start; i < fileSize; i += UDP_BUFFER_SIZE)
	{
		this->expectedPackages.push_back(i);
	}
}

void UDPClient::RemoveExpectedPackage(fpos_t number)
{
	for (auto i = this->expectedPackages.begin(); i != this->expectedPackages.end(); ++i)
	{
		if ((*i) == number)
		{
			this->expectedPackages.erase(i);
			break;
		}
	}
}

fpos_t UDPClient::ReceiveFileSize()
{
	auto answer = ReceiveMessageFrom(this->_udp_socket, this->serverAddressInfo);
	return StringToFileSize(answer);
}

string UDPClient::CreateFileInfo(string fileName, fpos_t pos, bool requestFileSize)
{
	return fileName + METADATA_DELIM + to_string(pos) + METADATA_DELIM + (requestFileSize ? "1" : "0");
}