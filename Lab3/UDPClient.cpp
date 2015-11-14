#include "UDPClient.h"

UDPClient::UDPClient(string address, unsigned int port) : Client(address, port)
{
	this->serverAddressInfo = (sockaddr*)CreateAddressInfoForClient();
	CreateUDPSocket();
	SetReceiveTimeout(this->_udp_socket);
}

void UDPClient::DownloadFile(string fileName)
{
	auto file = new fstream();
	OpenFile(file, GetLocalFileName(fileName));
	fpos_t currentProgress = file->tellp();
	fpos_t fileSize = currentProgress + 1;
	fpos_t lastProgress = 0;
	cout << "Download started." << endl;
	auto timer = new SpeedRater(currentProgress);
	auto dontKnowFileSize = true;
	while(fileSize > currentProgress)
	{
		try {
			SendMessageTo(this->_udp_socket, CreateFileInfo(fileName, currentProgress, dontKnowFileSize), this->serverAddressInfo);
			if (dontKnowFileSize) {
				dontKnowFileSize = false;
				fileSize = ReceiveFileSize();
			}
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

string UDPClient::CreateFileInfo(string fileName, fpos_t pos, bool requestFileSize)
{
	return fileName + METADATA_DELIM + to_string(pos) + METADATA_DELIM + (requestFileSize ? "1" : "0");
}