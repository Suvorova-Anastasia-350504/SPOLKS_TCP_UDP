#include "TCPClient.h"

TCPClient::TCPClient(string address, unsigned port) : Client(address, port)
{
	Init();
}

void TCPClient::Init()
{
	CreateTCPSocket();
	SetReceiveTimeout(this->_tcp_socket);
	SetSendTimeout(this->_tcp_socket);
	Connect();
	cout << "Connected to " << this->address << ":" << this->port << endl;
}

void TCPClient::Connect()
{
	auto serverAddress = CreateAddressInfoForClient();
	auto result = connect(this->_tcp_socket, (sockaddr*)serverAddress, sizeof(*serverAddress));
	if (result == SOCKET_ERROR) {
		throw runtime_error(EX_CONNECTION_ERROR);
	}
}

void TCPClient::ReceiveFile(fstream *file, fpos_t currentPos, fpos_t fileSize)
{
	Package *package;
	auto timer = new SpeedRater(currentPos);
	auto lastProgress = ShowProgress(0, currentPos, fileSize, timer);
	while (currentPos < fileSize)
	{
		try {
			package = ReceiveRawData(this->_tcp_socket);
			file->write(package->data, package->size);
			currentPos += package->size;
			lastProgress = ShowProgress(lastProgress, currentPos, fileSize, timer);
			delete package;
		}
		catch (runtime_error e) {
			cout << e.what() << endl;
			file->flush();
			delete timer;
			throw ConnectionInterrupted(currentPos);
		}
	}
	delete timer;
}

fpos_t TCPClient::ReceiveFileSize()
{
	auto answer = ReceiveMessage(this->_tcp_socket);
	return StringToFileSize(answer);
}

void TCPClient::Reconnect()
{
	auto reconnected = false;
	auto delay = 5;
	while (!reconnected)
	{
		Sleep(GetTimeout(TIMEOUT / delay));
		try {
			Close();
			Init();
			reconnected = true;
			cout << "Reconnected." << endl;
		}
		catch (runtime_error e) {
			if (delay != 1) delay--;
			cout << e.what() << endl;
		}
	}
}

void TCPClient::DownloadFile(string fileName)
{
	auto file = new fstream();
	OpenFile(file, GetLocalFileName(fileName));
	fpos_t currentProgress = file->tellp();
	fpos_t fileSize = 0;

	auto done = false;
	cout << "Download started." << endl;
	while (!done)
	{
		try {
			SendMessage(this->_tcp_socket, CreateFileInfo(fileName, currentProgress));
			fileSize = ReceiveFileSize();
			ReceiveFile(file, currentProgress, fileSize);
			done = true;
		}
		catch (ConnectionInterrupted e) {
			currentProgress = e.GetProgress();
			Reconnect();
		}
		catch (ServerError) {
			file->close();
			throw;
		}
		catch (runtime_error e) {
			cout << e.what() << endl;
			Reconnect();
		}
	}
	file->close();
	cout << "Done." << endl;
}