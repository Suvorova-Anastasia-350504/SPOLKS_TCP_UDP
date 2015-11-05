#include "UDPClient.h"

UDPClient::UDPClient(string address, unsigned int port) : Client(address, port, false)
{
	this->serverAddressInfo = (sockaddr*)CreateAddressInfoForClient();
	UDPClient::Init();
}

void UDPClient::DownloadFile(string fileName)
{
	auto file = new fstream();
	OpenFile(file, GetLocalFileName(fileName));
	fpos_t currentProgress = file->tellp();
	fpos_t fileSize = 1000000000000;
	auto lastProgress = 0;
	
	SendMessageTo(this->_udp_socket, CreateFileInfo(fileName, currentProgress), this->serverAddressInfo);
	fileSize = ReceiveFileSize();
	GenerateExpectedPackages(currentProgress, fileSize);

	cout << "Download started." << endl;

	while(fileSize > currentProgress)
	{
		try {
			cout << this->expectedPackages->size() << endl;
			auto package = ReceiveRawDataFrom(this->_udp_socket, this->serverAddressInfo);
			auto number = GetNumber(package);
			RemoveExpectedPackage(number);
			file->seekp(number);
			file->write(package->data, package->size - UDP_NUMBER_SIZE);
			currentProgress += package->size - UDP_NUMBER_SIZE;
			lastProgress = ShowProgress(lastProgress, currentProgress, fileSize);
		} catch (ServerError e) {
			cout << e.what() << endl;
			break;
		} catch(runtime_error e) {
			cout << e.what() << endl;
		}
	}
	//TODO : signal about the end
	cout << "Done." << endl;
}

fpos_t UDPClient::GetNumber(Package *package)
{
	fpos_t result = 0;
	auto packageSize = package->size - UDP_NUMBER_SIZE;
	for (fpos_t i = 0xFF, j = 0; j < UDP_NUMBER_SIZE - 1; j++, i <<= 8)
	{
		auto byte = unsigned char(package->data[packageSize + j]);
		result += (byte << j*8);
	}
	return result;
}

void UDPClient::GenerateExpectedPackages(fpos_t start, fpos_t fileSize)
{
	this->expectedPackages = new vector<fpos_t>();
	for (fpos_t i = start; i < fileSize; i+=UDP_BUFFER_SIZE)
	{
		this->expectedPackages->push_back(i);
	}
}

void UDPClient::RemoveExpectedPackage(fpos_t number)
{
	for (auto i = this->expectedPackages->begin(); i != this->expectedPackages->end(); ++i)
	{
		if ((*i) == number)
		{
			this->expectedPackages->erase(i);
			break;
		}
	}
}

void UDPClient::Init()
{
	CreateUDPSocket();
	SetSocketTimeout(this->_udp_socket);
}

fpos_t UDPClient::ReceiveFileSize()
{
	auto answer = ReceiveMessageFrom(this->_udp_socket, this->serverAddressInfo);
	return MessageToFileSize(answer);
}