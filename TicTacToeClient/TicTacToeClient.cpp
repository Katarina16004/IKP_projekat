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

    // TK: Create and send connection request
    cout << "\n[TK] Sending connection request..." << endl;
    ConnectionRequest request(1, 0, username);

    char sendBuffer[DEFAULT_BUFLEN];
    memset(sendBuffer, 0, DEFAULT_BUFLEN);
    request.serialize(sendBuffer);

    iResult = send(connectSocket, sendBuffer, DEFAULT_BUFLEN, 0);
    if (iResult == SOCKET_ERROR) {
        cerr << "[TK] Send failed: " << WSAGetLastError() << endl;
        closesocket(connectSocket);
        WSACleanup();
        return 1;
    }
    cout << "[TK] Connection request sent successfully." << endl;

    // FIRST RESPONSE
    cout << "\n[TK] Waiting for TOZ response..." << endl;
    char recvBuffer[DEFAULT_BUFLEN];
    memset(recvBuffer, 0, DEFAULT_BUFLEN);

    iResult = recv(connectSocket, recvBuffer, DEFAULT_BUFLEN, 0);
    if (iResult > 0) {
        ConnectionResponse response;
        response.deserialize(recvBuffer);

        // Validate checksum
        if (!response.validateChecksum()) {
            cerr << "[TK] ERROR: Invalid checksum in response!" << endl;
            closesocket(connectSocket);
            WSACleanup();
            return 1;
        }

        // Check if accepted or rejected
        cout << "\n========================================" << endl;
        if (response.getAccepted()) {
            cout << "[TK] [OK] CONNECTION ACCEPTED by TOZ" << endl;
            cout << "Message: " << response.getMessage() << endl;
            cout << "Tentative Game ID: " << response.getIdGame() << endl;
        }
        else {
            cout << "[TK] [X] CONNECTION REJECTED by TOZ" << endl;
            cout << "Reason: " << response.getMessage() << endl;
            cout << "========================================\n" << endl;

            // Connection rejected, close and exit
            closesocket(connectSocket);
            WSACleanup();
            cout << "\nPress Enter to exit... ";
            cin.get();
            return 1;
        }
        cout << "========================================\n" << endl;

    }
    else {
        cerr << "[TK] Failed to receive response from server." << endl;
        closesocket(connectSocket);
        WSACleanup();
        return 1;
    }

    // Cleanup
    closesocket(connectSocket);
    WSACleanup();

    cout << "\nPress Enter to exit... ";
    cin.get();

    return 0;
}