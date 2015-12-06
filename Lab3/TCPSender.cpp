#include "TCPSender.h"

TCPSender::TCPSender()
{
	auto sharedMemory = SharedMemoryManager::OpenSharedMemory(sizeof(SOCKET), SHARED_MEMORY_NAME_TCP);
	this->socket = *(SOCKET*)sharedMemory.memory;
	this->file = new std::fstream();
	AcceptConnection();
	SendFile();
}

SOCKET TCPSender::Accept()
{
	socket = accept(socket, NULL, NULL);
	if (socket == INVALID_SOCKET) {
		throw std::runtime_error(EX_ACCEPT_ERROR);
	}
	return socket;
}

void TCPSender::AcceptConnection() {
	socket = Accept();
	auto metadata = ExtractMetadata(ReceiveMessage(socket));
	try {
		OpenFile(file, metadata.fileName);
	}
	catch (std::runtime_error e) {
		SendMessage(socket, e.what());
		throw;
	}
	SendMessage(socket, std::to_string(GetFileSize(file)));
	file->seekg(metadata.progress);
}

void TCPSender::SendFile()
{
	try {
		while (file)
		{
			file->read(buffer, BUFFER_SIZE);
			size_t read = file->gcount();
			size_t realySent = SendRawData(socket, buffer, read);
			if (realySent != read)
			{
				fpos_t pos = file->tellg();
				file->seekg(pos - (read - realySent));
			}
		}
	}
	catch (std::runtime_error e) {

	}
	file->close();
	shutdown(socket, SD_BOTH);
	closesocket(socket);
}

TCPMetadata TCPSender::ExtractMetadata(std::string metadata)
{
	TCPMetadata metadata_st;
	std::string value;
	std::stringstream ss(metadata);
	std::getline(ss, value, METADATA_DELIM);
	metadata_st.fileName = value;
	std::getline(ss, value, PATH_DELIM);
	metadata_st.progress = stoll(value);
	return metadata_st;
}
