#include "Server.h"
#include "UDPClient.h"
#include "TCPClient.h"
#include <conio.h>

//just for Windows
void InitializeNetwork()
{
#ifdef _WIN32
	WSADATA wsaData;
	int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (result != 0)
	{
		throw "Can't initialize sockets";
	}
#endif
}

void ReleaseNetwork()
{
#ifdef _WIN32
	WSACleanup();
#endif
}

void startAsServer(std::string path)
{
	try {
		auto server = new Server(path);
		server->Run();
		delete server;
	}
	catch (exception e) {
		cout << e.what() << endl;
	}
}

void startAsClient() 
{
	Client *client = NULL;
	string type, ip, fileName;
	
	cout << "'udp' for UDP type" << endl;
	cin >> type;
	cout << "Enter IP:" << endl;
	cin >> ip;
	cout << "Enter file name" << endl;
	cin >> fileName;

	try
	{
		if (type == "udp") client = new UDPClient(ip);
		else client = new TCPClient(ip);
		client->DownloadFile(fileName);
	}
	catch (exception e)
	{
		cout << e.what() << endl;
	}
	delete client;
}

void startAsMain(char** argv) {
	string mode = "", type;
	cout << "'s' for server mode" << endl;
	cin >> mode;

	switch (mode[0]) {
	case 's':
		startAsServer(argv[0]);
		break;
	default:
		startAsClient();
	}
}

void startAsChild(char **argv) {
	if (std::string(argv[1]) == UDP) {
		//TODO : receive UDP file
	} else {
		//TODO recieve TCP file
	}
}

int main(int argc, char **argv)
{
	InitializeNetwork();	
	
	if (std::string(argv[0]) == CHILD) {
		startAsChild(argv);
	} else {
		startAsMain(argv);
	}

	ReleaseNetwork();
#ifdef _WIN32
	_getch();
#endif
	return 0;
}
