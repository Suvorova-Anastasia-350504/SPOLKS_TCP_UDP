#include "UDPServer.h"
#include "UDPClient.h"

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

void startAsServer() {
	Server *server = NULL;
	try
	{
		server = new Server();
		server->Run();
	}
	catch (exception e)
	{
		cout << e.what();
	}
	delete server;
}

void startAsClient() 
{
	Client *client = NULL;
	string ip;
	cout << "Enter IP:" << endl;
	cin >> ip;
	string fileName;
	cout << "Enter file name" << endl;
	cin >> fileName;
	try
	{
		client = new Client(ip);
		client->DownloadFile(fileName);
	}
	catch (exception e)
	{
		cout << e.what() << endl;
	}
	delete client;
}

void startAsUDPServer()
{
	try {
		auto server = new UDPServer();
		server->Run();
		delete server;
	}
	catch (exception e) {
		cout << e.what() << endl;
	}
}

void startAsUDPClient()
{
	string ip;
	cout << "Enter IP:" << endl;
	cin >> ip;
	string fileName;
	cout << "Enter file name" << endl;
	cin >> fileName;
	try
	{
		auto client = new UDPClient(ip);
		client->DownloadFile(fileName);
		delete client;
	}
	catch (exception e)
	{
		cout << e.what() << endl;
	}
}

int main(void)
{
	InitializeNetwork();

	string mode = "", type;
	cout << "'s' for server mode" << endl;
	cin >> mode;
	
	switch (mode[0]) {
		case 's':
			startAsUDPServer();
			break;
		default:
			cout << "upd or tcp" << endl;
			cin >> type;
			type == "udp" ? startAsUDPClient() : startAsClient();
	}

	ReleaseNetwork();
#ifdef _WIN32
	_getch();
#endif
	return 0;
}
