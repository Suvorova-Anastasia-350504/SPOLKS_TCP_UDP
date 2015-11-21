#include "Client.h"

Client::Client(string address, unsigned int port) : Base(port), address(address)
{
}

string Client::CreateFileInfo(string fileName, fpos_t pos)
{
	return fileName + METADATA_DELIM + to_string(pos);
}

fpos_t Client::StringToFileSize(string message)
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

fpos_t Client::ShowProgress(fpos_t lastProgress, fpos_t currentPos, fpos_t fileSize, SpeedRater *timer)
{
	auto progress = 100 * (double(currentPos) / double(fileSize));
	if (progress - lastProgress > DELTA_PERCENTAGE) {
		lastProgress = progress;
		cout << progress << " %. " << timer->GetSpeed(currentPos) << " MB/s " << currentPos << endl;
	}
	return lastProgress;
}

