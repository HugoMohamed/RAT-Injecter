#undef UNICODE

#define WIN32_LEAN_AND_MEAN

#include "windows.h"
#include "winsock2.h"
#include "ws2tcpip.h"
#include "stdlib.h"
#include "stdio.h"
#include "iostream"

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

	bool done;
	int i, cumul;

	done = false;
	cumul = 0;
	while (!done) {
		iResult = recv(ClientSocket,
			&recvbuf[cumul],
			recvbuflen,
			0);
		if (iResult != 0) { // if connection has not been shutdown (some bytes received)
			for (i = cumul; i < cumul + iResult; i++) {
				if (recvbuf[i] == '\0')
				{
					cout << endl;
					done = true;
				}
				else
					printf("%c", recvbuf[i]);
			}
			cumul += iResult; // increment the byte counter
		}
		else {  // if connection has been shutdown by the other party (the client), quit the loop 
			done = true;
		}
	}

	cout << "Bytes received: " << cumul << endl;

	int length;  //***

	FILE * file;
	char * byte_array;

	fopen_s(&file, recvbuf, "rb");
	fseek(file, 0, SEEK_END);
	length = ftell(file);
	fseek(file, 0, SEEK_SET);

	cout << "length = " << length << endl;
	byte_array = (char*)malloc(sizeof(char)*length);

	iSendResult = send(ClientSocket, (char *)(&length), sizeof(int), 0);  //***  envoi de la taille

	size_t j = 0;

	cout << "size of byte_array " << sizeof(byte_array) << endl;

	if (!byte_array)
	{
		printf("error memory\n");
		return;
	}

	cout << "number of element successfully read " << fread(byte_array, sizeof(char), length, file) << endl;

	iSendResult = send(ClientSocket, byte_array, length, 0);  //*** envoi de tout le fichier

	cout << "Bytes sent: " << iSendResult << endl;


	// cleanup
	closesocket(ClientSocket);
	WSACleanup();

	return;
}
