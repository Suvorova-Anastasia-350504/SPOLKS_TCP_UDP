#include "Client.h"

Client::Client(string address, unsigned int port, bool start) : Base(port), address(address)
{
	if (start)
	{
		Client::Init();
	}	
}

void Client::Init()
{
	CreateTCPSocket();
	SetSocketTimeout();
	Connect();
	cout << "Connected to " << this->address << ":" << this->port << endl;
}

void Client::Connect()
{
	auto serverAddress = CreateAddressInfoForClient();
	auto result = connect(this->_socket, (sockaddr*)serverAddress, sizeof(*serverAddress));
	if (result == SOCKET_ERROR) {
		throw runtime_error(EX_CONNECTION_ERROR);
	}
}

void Client::SetSocketTimeout()
{
	SetSocketTimeout(this->_socket);
}

void Client::SetSocketTimeout(SOCKET socket)
{
	auto timeout = GetTimeout();
	auto result = setsockopt(socket, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof(timeout));
	if (result == SOCKET_ERROR) {
		throw runtime_error(EX_SET_TIMEOUT_ERROR);
	}
	result = setsockopt(socket, SOL_SOCKET, SO_SNDTIMEO, (const char*)&timeout, sizeof(timeout));
	if (result == SOCKET_ERROR) {
		throw runtime_error(EX_SET_TIMEOUT_ERROR);
	}
}

string Client::CreateFileInfo(string fileName, fpos_t pos)
{
	return fileName + "|" + to_string(pos);
}

fpos_t Client::MessageToFileSize(string message)
{
	fpos_t fileSize = 0;
	try {
		fileSize = stoll(message);
	}
	catch (...) {
		cout << message << endl;
		throw ServerError(EX_SERVER_ERROR);
	}
	return fileSize;
}

sockaddr_in* Client::CreateAddressInfoForClient()
{
	return CreateAddressInfo(this->address, this->port);
}

string Client::ReceiveMessage()
{
	return Base::ReceiveMessage(this->_socket);
}

Package Client::ReceiveRawData()
{
	return *Base::ReceiveRawData(this->_socket);
}

//returns file's name without relative path
string Client::GetLocalFileName(string fileName)
{
	//for linux only
	replace(fileName.begin(), fileName.end(), PATH_DELIM_LINUX, PATH_DELIM);
	stringstream fileName_ss;
	fileName_ss << fileName;
	string realFileName;
	while (getline(fileName_ss, realFileName, PATH_DELIM));
	return realFileName;
}

void Client::OpenFile(fstream *file, string fileName)
{
	file->open(fileName, ios::in);
	if (file->is_open())
	{
		file->close();
		file->open(fileName, fstream::out | fstream::in | fstream::binary | fstream::ate);
	}
	else
	{
		file->close();
		file->open(fileName, ios::out | ios::binary);
	}

	if (!file->is_open())
	{
		throw runtime_error(EX_FILE_ERROR);
	}
}

fpos_t Client::ReceiveFileSize()
{
	auto answer = ReceiveMessage();
	return MessageToFileSize(answer);
}

void Client::DownloadFile(string fileName)
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
			SendMessage(this->_socket, CreateFileInfo(fileName, currentProgress));
			fileSize = ReceiveFileSize();
			ReceiveFile(file, currentProgress, fileSize);
			done = true;
		}
		catch (ConnectionInterrupted e) {
			currentProgress = e.GetProgress();
			Reconnect();
		}
		catch (ServerError)	{
			file->close();
			throw;
		}
		catch (runtime_error e)	{
			cout << e.what() << endl;
			Reconnect();
		}
	}
	file->close();
	cout << "Done." << endl;
}

fpos_t Client::ShowProgress(fpos_t lastProgress, fpos_t currentPos, fpos_t fileSize, SpeedRater *timer)
{
	auto progress = 100 * (double(currentPos) / double(fileSize));
	if (progress - lastProgress > DELTA_PERCENTAGE) {
		lastProgress = progress;
		cout << progress << " %. " << timer->GetSpeed(currentPos) << " MB/s" << endl;
	}
	return lastProgress;
}

void Client::ReceiveFile(fstream *file, fpos_t currentPos, fpos_t fileSize)
{
	Package package;
	auto timer = new SpeedRater(currentPos);
	auto lastProgress = ShowProgress(0, currentPos, fileSize, timer);
	
	while (currentPos < fileSize)
	{
		try {
			package = ReceiveRawData();		
			file->write(package.data, package.size);
			currentPos += package.size;
			lastProgress = ShowProgress(lastProgress, currentPos, fileSize, timer);
		} 
		catch (runtime_error e) {
			cout << e.what() << endl;
			file->flush();
			throw ConnectionInterrupted(currentPos);
		}		
	}
}

void Client::Reconnect()
{
	bool reconnected = false;
	while (!reconnected)
	{
		try {
			Close();
			Init();
			reconnected = true;
			cout << "Reconnected." << endl;
		} catch (runtime_error e) {
			cout << e.what() << endl;
		}
	}
}