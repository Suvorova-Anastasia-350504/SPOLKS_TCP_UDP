#include "UDPClient.h"

UDPClient::UDPClient(string address, unsigned int port) : Client(address, port)
{
	this->serverAddressInfo = (sockaddr*)CreateAddressInfoForClient();
	CreateUDPSocket();
	SetReceiveTimeout(this->_udp_socket, GetTimeout(UDP_RECV_TIMEOUT));
	for (auto i = 0; i < PACKAGE_COUNT; i++)
	{
		auto _pair = new pair<fpos_t, Package*>();
		_pair->second = new Package();
		_pair->second->data = new char[UDP_BUFFER_SIZE];
		this->receivedBuffer.push_back(_pair);
	}
}

fpos_t UDPClient::ConnectToServer(string metadata)
{
	while(true)
	{
		try
		{
			SendMessageTo(this->_udp_socket, metadata, this->serverAddressInfo);
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
	vector<fpos_t> missingPackageOffsets;
	auto file = new fstream();
	OpenFile(file, GetLocalFileName(fileName));
	fpos_t currentProgress = file->tellp();		
	auto fileSize = ConnectToServer(CreateFileInfo(fileName, currentProgress, PACKAGE_COUNT, true));	
	cout << "Download started." << endl;
	auto timer = new SpeedRater(currentProgress);
	auto lastProgress = 0;
	auto done = fileSize <= currentProgress;
	fpos_t currentBatch = 0;
	while(!done)
	{
		try {
			ReceiveBatch();			
		}
		catch (ServerError e) {
			cout << e.what() << endl;
			break;
		}
		catch (runtime_error e) { 
			//Do nothing.
		}

		//Batch received
		//Check missing packages and add it to vector
		//PACKAGE_COUNT really equal to batch size?
		CollectMissingPackages(currentBatch, missingPackageOffsets);
		
		//TODO: Implement write considering lost packages 
		WriteBatchToFile(file, currentProgress);
		lastProgress = ShowProgress(lastProgress, currentProgress, fileSize, timer);

		done = currentProgress >= fileSize;
		if (!done) 
			SendMessageTo(this->_udp_socket, CreateFileInfo(fileName, currentProgress, PACKAGE_COUNT, false), this->serverAddressInfo);
	}
	file->close();
	cout << "Done." << endl;
}

void UDPClient::WriteBatchToFile(fstream *file, fpos_t& currentProgress)
{
	sort(this->receivedBuffer.begin(), this->receivedBuffer.end(),
		[&](pair<fpos_t, Package*>* v_1, pair<fpos_t, Package*>* v_2)
	{
		return v_1->first < v_2->first;
	});
	for (auto i = this->receivedBuffer.begin(); i != this->receivedBuffer.end(); ++i)
	{
		if ((*i)->first * UDP_BUFFER_SIZE != currentProgress) {
			file->seekg((*i)->first * UDP_BUFFER_SIZE);
			currentProgress = (*i)->first * UDP_BUFFER_SIZE;
		}
		file->write((*i)->second->data, (*i)->second->size);
		currentProgress += UDP_BUFFER_SIZE;
	}
}

void UDPClient::ReceiveBatch()
{
	fpos_t receivedCount = 0;
	while (true)
	{
		try {
			auto package = ReceiveRawDataFrom(this->_udp_socket, this->serverAddressInfo);
			auto number = GetNumber(package);
			auto dataSize = package->size - UDP_NUMBER_SIZE;
			auto index = number % PACKAGE_COUNT;
			this->receivedBuffer[index]->first = number;
			memcpy(this->receivedBuffer[index]->second->data, package->data, dataSize);
			this->receivedBuffer[index]->second->size = dataSize;
			if (++receivedCount == PACKAGE_COUNT) 
			{
				throw runtime_error("Packages batch received.");
			}
		}
		catch (runtime_error e) {
			//cout << e.what() << endl;
			throw;
		}
	}
}

fpos_t UDPClient::GetNumber(Package* package)
{
	return Base::GetNumber(package->data, package->size);
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

void UDPClient::CollectMissingPackages(fpos_t& currentBatch, vector<fpos_t>& missingPackageOffsets) 
{
	bool packageMissed;
	for (fpos_t i = currentBatch * PACKAGE_COUNT; i < (currentBatch + 1)* PACKAGE_COUNT; i++) 
	{
		//TODO: replace find_if by sort + ...
		packageMissed = find_if(
			this->receivedBuffer.begin(),
			this->receivedBuffer.end(),
			[&](pair<fpos_t, Package*>* receivedPackage) { return receivedPackage->first == i; }
		) == this->receivedBuffer.end();
		if (packageMissed) 
		{
			missingPackageOffsets.push_back(i);
		}
	}
}