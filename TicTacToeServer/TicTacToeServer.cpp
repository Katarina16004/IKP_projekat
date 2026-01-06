#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS

#include <iostream>
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <thread>
#include <mutex>
#include <chrono>
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
void MatchmakingThread();  // TS thread

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

    // Start TS thread
    thread matchmakingThread(MatchmakingThread);
    matchmakingThread.detach();
    cout << "[TS] Matchmaking thread started.\n" << endl;

    // Accept connections (TOZ threads will be created here)
    while (serverRunning) {
        SOCKET clientSocket = accept(listenSocket, NULL, NULL);
        if (clientSocket == INVALID_SOCKET) {
            cerr << "[SERVER] Accept failed: " << WSAGetLastError() << endl;
            continue;
        }

        cout << "[SERVER] New client connection received!" << endl;

        // TOZ create thread for each client connection
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

    cout << "[TOZ] Client connection handler finished.  Player is waiting.\n" << endl;
}

// TS
void MatchmakingThread() {
    cout << "[TS] Matchmaking service started." << endl;

    while (serverRunning) {
        // Check every second for players to match
        this_thread::sleep_for(chrono::milliseconds(1000));

        lock_guard<mutex> lock(waitingMutex);

        // Check if we have at least 2 players waiting
        if (waitingPlayers.size() < 2) {
            continue;
        }

        // Get first two players
        Player player1, player2;
        waitingPlayers.read(1, player1);
        waitingPlayers.read(2, player2);

        // Remove them from waiting list
        waitingPlayers.remove(1);
        waitingPlayers.remove(1);

        int gameId;
        {
            lock_guard<mutex> lock(idMutex);
            gameId = nextGameId++;
        }

        // Update both players with the correct game ID
        player1.setIdGame(gameId);
        player2.setIdGame(gameId);

        cout << "\n[TS-MATCHMAKING] ================================" << endl;
        cout << "[TS-MATCHMAKING] Game " << gameId << " created!" << endl;
        cout << "[TS-MATCHMAKING]   Player 1 (X): " << player1.getUsername()
            << " (Game ID: " << player1.getIdGame() << ")" << endl;
        cout << "[TS-MATCHMAKING]   Player 2 (O): " << player2.getUsername()
            << " (Game ID: " << player2.getIdGame() << ")" << endl;
        cout << "[TS-MATCHMAKING] ================================\n" << endl;

        // Send game start notifications
        char buffer[DEFAULT_BUFLEN];

        ConnectionResponse msg1(1, player1.getIdStruct(), true,
            "Game found!  You are Player X.  Game starting...", gameId);
        memset(buffer, 0, DEFAULT_BUFLEN);
        msg1.serialize(buffer);
        send(player1.getAcceptedSocket(), buffer, DEFAULT_BUFLEN, 0);

        ConnectionResponse msg2(1, player2.getIdStruct(), true,
            "Game found!  You are Player O. Game starting..  .", gameId);
        memset(buffer, 0, DEFAULT_BUFLEN);
        msg2.serialize(buffer);
        send(player2.getAcceptedSocket(), buffer, DEFAULT_BUFLEN, 0);

        cout << "[TS] Game " << gameId << " notifications sent to both players." << endl;
        cout << "[TS] Waiting players remaining: " << waitingPlayers.size() << endl;
        cout << "[TS] Ready to match more players.. .\n" << endl;

        // TODO: Start game logic 
        // For now, just close connections
        Sleep(3000);
        closesocket(player1.getAcceptedSocket());
        closesocket(player2.getAcceptedSocket());
    }

    cout << "[TS] Matchmaking thread ending." << endl;
}