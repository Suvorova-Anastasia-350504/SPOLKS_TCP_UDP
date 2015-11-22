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
	file = new fstream();
	this->fileName = fileName;
	OpenFile(file, GetLocalFileName(fileName));
	fpos_t currentProgress = file->tellp();		
	fileSize = ConnectToServer(CreateFileInfo(fileName, currentProgress, PACKAGE_COUNT, true));	
	cout << "Download started." << endl;
	auto timer = new SpeedRater(currentProgress);
	auto lastProgress = 0;
	auto done = fileSize <= currentProgress;
	fpos_t currentBatch = 0;
	while(!done)
	{
		try {
			ProcessBatches(file, fileSize);
			//ReceiveBatch();			
		}
		catch (ServerError e) {
			cout << e.what() << endl;
			break;
		}
		catch (runtime_error e) { 
			//Do nothing.
		}

		//TODO: Implement write considering lost packages 
		lastProgress = ShowProgress(lastProgress, currentProgress, fileSize, timer);

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

void UDPClient::CollectMissingPackages(fpos_t& currentBatch, vector<fpos_t>& missingPackages) 
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
			missingPackages.push_back(i);
		}
	}
}

void UDPClient::ProcessBatches(fstream* file, fpos_t fileSize)
{
	fpos_t receivedCount = 0, currentBatch = 0;
	fpos_t batchCount = fileSize / PACKAGE_COUNT;
	fpos_t currentProgress = 0;
	fpos_t lastPackageInCurrentBatch, lastReceivedPackage, packageNumber;
	
	Package *package;

	while (true) {
		lastPackageInCurrentBatch = (currentBatch + 1) * PACKAGE_COUNT - 1;
		AddBatchToMissingPackages(currentBatch);
		
		while (true) {
			//try {
			package = ReceiveRawDataFrom(this->_udp_socket, this->serverAddressInfo);
			packageNumber = GetNumber(package);
			//auto dataSize = package->size - UDP_NUMBER_SIZE;
			//auto index = packageNumber % PACKAGE_COUNT;

			if (packageNumber * UDP_BUFFER_SIZE != currentProgress) {
				file->seekg(packageNumber * UDP_BUFFER_SIZE);
				currentProgress = packageNumber * UDP_BUFFER_SIZE;
			}
			file->write(package->data, package->size - UDP_NUMBER_SIZE);
			currentProgress += UDP_BUFFER_SIZE;

			if (packageNumber >= lastPackageInCurrentBatch) {
				break;
			}
			RemoveFromMissingPackages(packageNumber);
		}


		SendMissingPackages();
		if (++currentBatch > batchCount)
		{	//batch receiving finished
			break;
		}
	}
		
}

void UDPClient::RemoveFromMissingPackages(fpos_t index) 
{
	//remove element with [index] value from vector
	missingPackages.erase(std::remove(
		missingPackages.begin(),
		missingPackages.end(),
		index
		), missingPackages.end());
	
}


void UDPClient::AddBatchToMissingPackages(fpos_t batch) 
{
	for (fpos_t i = batch * PACKAGE_COUNT; i < batch * PACKAGE_COUNT; i++) 
	{
		missingPackages.push_back(i);
	}
}

void UDPClient::SendMissingPackages()  
{
	auto *message = new char[BUFFER_SIZE + 1];
	auto dataSize = fileName.size() + sizeof(char)+UDP_NUMBER_SIZE + UDP_NUMBER_SIZE * missingPackages.size();
	auto index = 0;
	index += fileName.copy(message, fileName.size());
	message[index++] = METADATA_DELIM;
	AddNumberToDatagram(message, index, missingPackages.size());
	index += UDP_NUMBER_SIZE;
	for (auto i = missingPackages.begin(); i != missingPackages.end(); ++i) {
		AddNumberToDatagram(message, index, (*i));
		index += UDP_NUMBER_SIZE;
	}
	SendRawDataTo(this->_udp_socket, message, dataSize, serverAddressInfo);
}

