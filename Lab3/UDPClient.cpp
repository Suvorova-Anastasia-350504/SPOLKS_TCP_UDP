#include "UDPClient.h"

UDPClient::UDPClient(string address, unsigned int port) : Client(address, port)
{
	this->serverAddressInfo = (sockaddr*)CreateAddressInfoForClient();
	CreateUDPSocket();
//	SetSocketTimeout(this->_udp_socket);
}

void UDPClient::DownloadFile(string fileName)
{
	auto file = new fstream();
	OpenFile(file, GetLocalFileName(fileName));
	fpos_t currentProgress = file->tellp();
	fpos_t fileSize = 10000000000;
	fpos_t lastProgress = 0;
	cout << "Download started." << endl;
	auto timer = new SpeedRater(currentProgress);
	while(fileSize > currentProgress)
	{
		try {
			SendMessageTo(this->_udp_socket, CreateFileInfo(fileName, currentProgress), this->serverAddressInfo);
//			fileSize = ReceiveFileSize();
			auto package = ReceiveRawDataFrom(this->_udp_socket, this->serverAddressInfo);
			file->write(package->data, package->size);
			currentProgress += package->size;
			lastProgress = ShowProgress(lastProgress, currentProgress, fileSize, timer);
		} catch (ServerError e) {
			cout << e.what() << endl;
			break;
		} catch(runtime_error e) {
			cout << e.what() << endl;
		}
	}
	cout << "Done." << endl;
}

fpos_t UDPClient::ReceiveFileSize()
{
	auto answer = ReceiveMessageFrom(this->_udp_socket, this->serverAddressInfo);
	return StringToFileSize(answer);
}
