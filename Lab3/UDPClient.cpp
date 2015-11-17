#include "UDPClient.h"

UDPClient::UDPClient(string address, unsigned int port) : Client(address, port)
{
	this->serverAddressInfo = (sockaddr*)CreateAddressInfoForClient();
	CreateUDPSocket();
	SetReceiveTimeout(this->_udp_socket, GetTimeout(1000));
}

void UDPClient::DownloadFile(string fileName)
{
	auto file = new fstream();
	OpenFile(file, GetLocalFileName(fileName));
	fpos_t currentProgress = file->tellp();
	SendMessageTo(this->_udp_socket, CreateFileInfo(fileName, currentProgress, PACKAGE_COUNT, true), this->serverAddressInfo);
	auto fileSize = ReceiveFileSize();

	//GenerateExpectedPackages(currentProgress / UDP_BUFFER_SIZE, fileSize / UDP_BUFFER_SIZE);

	cout << "Download started." << endl;
	auto done = fileSize <= currentProgress;
	auto timer = new SpeedRater(currentProgress);
	auto lastProgress = ShowProgress(0, currentProgress, fileSize, timer);
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
			sort(this->receivedPackages.begin(), this->receivedPackages.end());
			for (auto i = this->receivedPackages.begin(); i != this->receivedPackages.end(); ++i)
			{
				if ((*i) * UDP_BUFFER_SIZE != currentProgress) break;
				currentProgress += UDP_BUFFER_SIZE;
				//this->expectedPackages.erase(this->expectedPackages.begin());
			}
			receivedPackages.clear();
			lastProgress = ShowProgress(lastProgress, currentProgress, fileSize, timer);
			SendMessageTo(this->_udp_socket, CreateFileInfo(fileName, currentProgress, PACKAGE_COUNT, false), this->serverAddressInfo);
		}
	}
	file->close();
	cout << "Done." << endl;
}

void UDPClient::ReceiveFile(fstream *file, fpos_t currentProgress, fpos_t fileSize)
{
	fpos_t count = 0;
	while (currentProgress < fileSize)
	{
		try {
			auto package = ReceiveRawDataFrom(this->_udp_socket, this->serverAddressInfo);
			auto number = GetNumber(package);
			receivedPackages.push_back(number);
			auto dataSize = package->size - UDP_NUMBER_SIZE;
			if (fpos_t(file->tellp()) != number * UDP_BUFFER_SIZE) file->seekp(number * UDP_BUFFER_SIZE);
			file->write(package->data, dataSize);
			currentProgress += dataSize;
			if (++count == PACKAGE_COUNT) throw runtime_error("Packages batch received.");
		}
		catch (runtime_error e) {
			file->flush();
			throw ConnectionInterrupted(currentProgress);
		}
	}
}

fpos_t UDPClient::GetNumber(Package* package)
{
	fpos_t result = 0;
	auto packageSize = package->size - UDP_NUMBER_SIZE;
	for (fpos_t i = 0xFF, j = 0; j < UDP_NUMBER_SIZE - 1; j++, i <<= 8)
	{
		auto byte = unsigned char(package->data[packageSize + j]);
		result += byte << (j << 3);
	}
	return result;
}

void UDPClient::GenerateExpectedPackages(fpos_t start, fpos_t fileSize)
{
	for (auto i = start; i < fileSize; i++)
	{
		this->expectedPackages.push_back(i);
	}
}

void UDPClient::RemoveExpectedPackage(fpos_t number)
{
	for (auto i = this->expectedPackages.begin(); i != this->expectedPackages.end(); ++i)
	{
		if ((*i) != number) continue;
		this->expectedPackages.erase(i);
		return;
	}
	cout << "YYEAH" << number << endl;
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