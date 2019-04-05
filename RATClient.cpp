#undef UNICODE

#define WIN32_LEAN_AND_MEAN

#include "windows.h"
#include "winsock2.h"
#include "ws2tcpip.h"
#include "stdlib.h"
#include "stdio.h"
#include <iostream>
#include <string>
#include <vector>

using namespace std;

// Need to link with Ws2_32.lib
#pragma comment (lib, "Ws2_32.lib")
#pragma comment (lib, "Mswsock.lib")

#define DEFAULT_BUFLEN 512
#define DEFAULT_PORT "27015"

void main(void)
{
	WSADATA wsaData;
	int iResult;

	SOCKET ListenSocket = INVALID_SOCKET;
	SOCKET ClientSocket = INVALID_SOCKET;

	struct addrinfo *result = NULL;
	struct addrinfo hints;

	int iSendResult;
	char recvbuf[DEFAULT_BUFLEN];
	int recvbuflen = DEFAULT_BUFLEN;

	// Initialize Winsock
	iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != 0) {
		cout << "WSAStartup failed with error: " << iResult << endl;
		return;
	}

	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	hints.ai_flags = AI_PASSIVE;

	// Resolve the server address and port
	iResult = getaddrinfo(NULL, DEFAULT_PORT, &hints, &result);
	if (iResult != 0) {
		cout << "getaddrinfo failed with error: " << iResult << endl;
		WSACleanup();
		return;
	}

	// Create a SOCKET for connecting to server
	ListenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
	if (ListenSocket == INVALID_SOCKET) {
		cout << "socket failed with error: " << WSAGetLastError() << endl;
		freeaddrinfo(result);
		WSACleanup();
		return;
	}

	// Setup the TCP listening socket
	iResult = bind(ListenSocket, result->ai_addr, (int)result->ai_addrlen);
	if (iResult == SOCKET_ERROR) {
		cout << "bind failed with error: " << WSAGetLastError() << endl;
		freeaddrinfo(result);
		closesocket(ListenSocket);
		WSACleanup();
		return;
	}

	freeaddrinfo(result);

	iResult = listen(ListenSocket, SOMAXCONN);
	if (iResult == SOCKET_ERROR) {
		cout << "listen failed with error: " << WSAGetLastError() << endl;
		closesocket(ListenSocket);
		WSACleanup();
		return;
	}

	// Accept a client socket
	ClientSocket = accept(ListenSocket, NULL, NULL);
	if (ClientSocket == INVALID_SOCKET) {
		cout << "accept failed with error: " << WSAGetLastError() << endl;
		closesocket(ListenSocket);
		WSACleanup();
		return;
	}

	// No longer need server socket
	closesocket(ListenSocket);

	// User send commands
	string commande;
	cout << "Command prompt,\n    type \"help\" to display help\n    type \"quit\" to quit" << endl;
	do
	{
		cout << ">";
		getline(cin, commande);
		int length = commande.length();
		const char* c = commande.c_str();
		// Send length of command
		iSendResult = send(ClientSocket, (char *)(&length), sizeof(int), 0);
		// Send command
		iSendResult = send(ClientSocket, c, length, 0);

		// Receive result :
		int taille;
		int cumul = 0;

		vector<string> argvect;
		int j = 0;
		string tmp;
		// split string into argument vector
		while (commande[j] != '\0')
		{
			if (commande[j] != ' ')
				tmp += commande[j];
			else
			{
				argvect.push_back(tmp);
				tmp.clear();
			}
			j++;
		}
		argvect.push_back(tmp);

		// Get file from server
		if (argvect[0] == "get")
		{
			int cumul;
			int taille;

			cumul = 0;
			cout << "TEST1" << endl;

			// reçoit la taille du fichier
			while (cumul < sizeof(int) - 1) {   
				iResult = recv(ClientSocket,
					((char *)(&taille)) + cumul,
					sizeof(taille),
					0);
				cumul += iResult;
			}
			cumul = 0;
			FILE * recv_file;

			// ouverture du fichier avant de recevoir les informations
			if (fopen_s(&recv_file, argvect[2].c_str(), "ab") != 0)
			{
				cout << "could not open" << argvect[2] << endl;
				return;
			}

			// recoit tout le fichier
			while (cumul < taille - 1) {
				iResult = recv(ClientSocket,
					&recvbuf[0],
					recvbuflen,
					0);
				fwrite(recvbuf, sizeof(char), iResult, recv_file);
				cumul += iResult;
			}
			fclose(recv_file);
		}

		// Receive result of command
		while (cumul < sizeof(int) - 1)
		{
			iResult = recv(ClientSocket,
				((char *)(&taille)) + cumul,
				sizeof(taille),
				0);
			cumul += iResult;
		}

		char* result = new char[taille];
		cumul = 0;
		while (cumul < taille - 1)
		{
			iResult = recv(ClientSocket,
				result,
				taille,
				0);
			cumul += iResult;
		}
		cout << result << endl;
	} while (commande != "quit");

	// cleanup
	closesocket(ClientSocket);
	WSACleanup();

	return;
}
