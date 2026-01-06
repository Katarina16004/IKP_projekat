#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS

#include <iostream>
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <string>

#include "../CommonLib/ConnectionRequest.h"
#include "../CommonLib/ConnectionResponse.h"

// TEMPORARY
#ifndef COMMONLIB_INCLUDED
#define COMMONLIB_INCLUDED
#include "../CommonLib/ConnectionRequest.cpp"
#include "../CommonLib/ConnectionResponse.cpp"
#endif

#pragma comment(lib, "ws2_32.lib")

#define DEFAULT_PORT 27016
#define DEFAULT_BUFLEN 512
#define SERVER_IP "127.0.0.1"

using namespace std;

int main()
{
    WSADATA wsaData;
    int iResult;

    cout << "========================================" << endl;
    cout << "    TIC TAC TOE CLIENT    " << endl;
    cout << "========================================\n" << endl;

    // Initialize Winsock
    iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (iResult != 0) {
        cerr << "WSAStartup failed: " << iResult << endl;
        return 1;
    }

    // Create socket
    SOCKET connectSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (connectSocket == INVALID_SOCKET) {
        cerr << "Socket creation failed: " << WSAGetLastError() << endl;
        WSACleanup();
        return 1;
    }

    // Connect to server
    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = inet_addr(SERVER_IP);
    serverAddr.sin_port = htons(DEFAULT_PORT);

    iResult = connect(connectSocket, (sockaddr*)&serverAddr, sizeof(serverAddr));
    if (iResult == SOCKET_ERROR) {
        cerr << "Connection failed: " << WSAGetLastError() << endl;
        closesocket(connectSocket);
        WSACleanup();
        return 1;
    }

    cout << "Connected to server!" << endl;

    // Get username
    char username[50];
    cout << "Enter your username: ";
    cin.getline(username, 50);

    // Cleanup
    closesocket(connectSocket);
    WSACleanup();

    cout << "\nPress Enter to exit...  ";
    cin.get();

    return 0;
}