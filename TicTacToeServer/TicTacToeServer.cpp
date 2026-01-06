#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS

#include <iostream>
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <thread>
#include <mutex>
#include "../CommonLib/ConnectionRequest.h"
#include "../CommonLib/ConnectionResponse.h"
#include "../CommonLib/Player.h"
#include "../CommonLib/List.h"

// TEMPORARY
#ifndef COMMONLIB_INCLUDED
#define COMMONLIB_INCLUDED
#include "../CommonLib/ConnectionRequest.cpp"
#include "../CommonLib/ConnectionResponse.cpp"
#include "../CommonLib/Player.cpp"
#endif

#pragma comment(lib, "ws2_32.lib")

#define DEFAULT_PORT 27016
#define DEFAULT_BUFLEN 512

using namespace std;

List<Player> waitingPlayers;
mutex waitingMutex;

int nextGameId = 1;
int nextClientId = 1;
mutex idMutex;

bool serverRunning = true;

void ClientConnectionHandler(SOCKET clientSocket);  // TOZ thread

int main()
{
    WSADATA wsaData;
    int iResult;

    cout << "========================================" << endl;
    cout << "    TIC TAC TOE SERVER   " << endl;
    cout << "========================================\n" << endl;

    // Initialize Winsock
    iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (iResult != 0) {
        cerr << "WSAStartup failed: " << iResult << endl;
        return 1;
    }

    // Create socket
    SOCKET listenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listenSocket == INVALID_SOCKET) {
        cerr << "Socket creation failed:  " << WSAGetLastError() << endl;
        WSACleanup();
        return 1;
    }

    // Bind socket
    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(DEFAULT_PORT);

    iResult = bind(listenSocket, (sockaddr*)&serverAddr, sizeof(serverAddr));
    if (iResult == SOCKET_ERROR) {
        cerr << "Bind failed: " << WSAGetLastError() << endl;
        closesocket(listenSocket);
        WSACleanup();
        return 1;
    }

    // Listen
    iResult = listen(listenSocket, SOMAXCONN);
    if (iResult == SOCKET_ERROR) {
        cerr << "Listen failed: " << WSAGetLastError() << endl;
        closesocket(listenSocket);
        WSACleanup();
        return 1;
    }

    cout << "[SERVER] Listening on port " << DEFAULT_PORT << endl;
    cout << "[SERVER] Ready to accept multiple clients..." << endl;

    // Accept connections (TOZ threads will be created here)
    while (serverRunning) {
        SOCKET clientSocket = accept(listenSocket, NULL, NULL);
        if (clientSocket == INVALID_SOCKET) {
            cerr << "[SERVER] Accept failed: " << WSAGetLastError() << endl;
            continue;
        }

        cout << "[SERVER] New client connection received!" << endl;

        // TOZ:  Create thread for each client connection
        thread clientThread(ClientConnectionHandler, clientSocket);
        clientThread.detach();
    }

    closesocket(listenSocket);
    WSACleanup();
    return 0;
}

// TOZ 
void ClientConnectionHandler(SOCKET clientSocket) {
    char recvBuffer[DEFAULT_BUFLEN];
    char sendBuffer[DEFAULT_BUFLEN];
    int iResult;

    cout << "[TOZ] New client handler thread started." << endl;

    // Receive connection request
    memset(recvBuffer, 0, DEFAULT_BUFLEN);
    iResult = recv(clientSocket, recvBuffer, DEFAULT_BUFLEN, 0);

    if (iResult <= 0) {
        cout << "[TOZ] Client disconnected before sending request." << endl;
        closesocket(clientSocket);
        return;
    }

    ConnectionRequest request;
    request.deserialize(recvBuffer);

    cout << "[TOZ] Connection request from:  " << request.getUsername() << endl;

    // Validate checksum
    if (!request.validateChecksum()) {
        cout << "[TOZ] [X] Invalid checksum! Rejecting connection." << endl;

        ConnectionResponse response(1, request.getIdSource(), false,
            "Invalid checksum - connection rejected", 0);

        memset(sendBuffer, 0, DEFAULT_BUFLEN);
        response.serialize(sendBuffer);
        send(clientSocket, sendBuffer, DEFAULT_BUFLEN, 0);

        closesocket(clientSocket);
        cout << "[TOZ] Connection rejected. Thread ending.\n" << endl;
        return;
    }

    cout << "[TOZ] [OK] Checksum valid." << endl;

    // Accept connection and assign tentative game ID
    int gameId;
    {
        lock_guard<mutex> lock(idMutex);
        gameId = nextGameId;
    }

    cout << "[TOZ] [OK] CONNECTION ACCEPTED" << endl;
    cout << "[TOZ]   Username: " << request.getUsername() << endl;
    cout << "[TOZ]   Tentative Game ID: " << gameId << endl;

    ConnectionResponse response(1, request.getIdSource(), true,
        "Connection accepted!   Waiting for opponent...", gameId);

    memset(sendBuffer, 0, DEFAULT_BUFLEN);
    response.serialize(sendBuffer);
    send(clientSocket, sendBuffer, DEFAULT_BUFLEN, 0);

    // Create Player object and add to waiting list
    Player player(request.getUsername(), clientSocket, gameId);

    {
        lock_guard<mutex> lock(waitingMutex);
        waitingPlayers.add(waitingPlayers.size() + 1, player);
        cout << "[TOZ] Player '" << player.getUsername() << "' added to waiting list." << endl;
        cout << "[TOZ] Waiting players: " << waitingPlayers.size() << endl;
    }

    cout << "[TOZ] Client connection handler finished.   Player is waiting.\n" << endl;
}