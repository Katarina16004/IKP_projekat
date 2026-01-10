#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#define NOMINMAX

#include <iostream>
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <string>
#include <limits>

#include "../CommonLib/ConnectionRequest.h"
#include "../CommonLib/ConnectionResponse.h"
#include "../CommonLib/MessageForMove.h"
#include "../CommonLib/Move.h"

// TEMPORARY
#ifndef COMMONLIB_INCLUDED
#define COMMONLIB_INCLUDED
#include "../CommonLib/ConnectionRequest.cpp"
#include "../CommonLib/ConnectionResponse.cpp"
#include "../CommonLib/MessageForMove.cpp"
#include "../CommonLib/Move.cpp"
#endif

#pragma comment(lib, "ws2_32.lib")

#define DEFAULT_PORT 27016
#define DEFAULT_BUFLEN 512
#define SERVER_IP "127.0.0.1"
#define SERVER_ID 100

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

    //cout << "Connecting to server..." << endl;
    iResult = connect(connectSocket, (sockaddr*)&serverAddr, sizeof(serverAddr));
    if (iResult == SOCKET_ERROR) {
        cerr << "Connection failed: " << WSAGetLastError() << endl;
        closesocket(connectSocket);
        WSACleanup();
        return 1;
    }

    //cout << "Connected to server!\n" << endl;

    // Get username
    char username[50];
    cout << "Enter your username: ";
    cin.getline(username, 50);

    // TK: Create and send connection request
    cout << "\n[TK] Sending connection request..." << endl;
    ConnectionRequest request(1, SERVER_ID, username);

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
            cout << "Tentative Player ID: " << response.getIdDest() << endl;
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
    bool running = true;

    // SECOND RESPONSE
    cout << "[TK] Waiting in matchmaking queue..." << endl;
    cout << "[TK] Waiting for TS to find opponent.. .\n" << endl;

    do {
        memset(recvBuffer, 0, DEFAULT_BUFLEN);
        iResult = recv(connectSocket, recvBuffer, DEFAULT_BUFLEN, 0);

        if (iResult > 0) {
            //ConnectionResponse matchResponse;
            MessageForMove matchMessage;
            matchMessage.deserialize(recvBuffer);

            // Validate checksum
            if (!matchMessage.validateChecksum()) {
                cerr << "[TK] ERROR: Invalid checksum in matchmaking response!" << endl;
                closesocket(connectSocket);
                WSACleanup();
                return 1;
            }

            string mess = "Game found!";
            if (mess.compare(0, 11, matchMessage.getMessage(), 0, 11) == 0)
            {
                cout << "\n========================================" << endl;
                cout << "[TK] MATCHMAKING COMPLETE!" << endl;
                cout << "Message: " << matchMessage.getMessage() << endl;
                cout << "Final Game ID: " << matchMessage.getIdGame() << endl;
                cout << "Final Player ID: " << matchMessage.getIdDest() << endl;
                cout << "========================================\n" << endl;

                cout << "[TK] Game is about to start!" << endl;
            }

            if (!matchMessage.getEnd())
            {
                if (matchMessage.getPlaying())
                {
                    cout << "Your move! Pick a free spot on board: " << endl;

                    const int (*b)[3] = matchMessage.getBoard();
                    for (int i = 0; i < 3; i++)
                    {
                        if (i > 0)
                        {
                            cout << "--- --- ---" << endl;
                        }
                        for (int j = 0; j < 3; j++)
                        {
                            if (b[i][j] == 1)
                            {
                                cout << " X ";
                            }
                            else if (b[i][j] == 2)
                            {
                                cout << " O ";
                            }
                            else
                            {
                                cout << "   ";
                            }

                            if (j != 2)
                            {
                                cout << "|";
                            }
                            else
                            {
                                cout << endl;
                            }
                        }
                    }

                    bool badMove = true;
                    int x, y;
                    do {
                        cout << "Row(1-3): ";
                        while (!(cin >> x) || x < 1 || x > 3) {
                            cin.clear();
                            cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
                            cout << "Invalid input. Row (1-3): ";
                        }
                        x--;

                        cout << "Column(1-3): ";
                        while (!(cin >> y) || y < 1 || y > 3) {
                            cin.clear();
                            cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
                            cout << "Invalid input. Column (1-3): ";
                        }
                        y--;

                        if (b[x][y] != 0)
                        {
                            cout << "Invalid move! That spot is already taken, choose again!" << endl;
                        }
                        else
                        {
                            badMove = false;
                        }
                    } while (badMove);
                    cout << "\n[TK] Sending played move ..." << endl;
                    Move playedMove(matchMessage.getIdDest(), matchMessage.getIdSource(), matchMessage.getIdGame(), x, y);

                    char sendBuffer[DEFAULT_BUFLEN];
                    memset(sendBuffer, 0, DEFAULT_BUFLEN);
                    playedMove.serialize(sendBuffer);

                    iResult = send(connectSocket, sendBuffer, DEFAULT_BUFLEN, 0);
                    if (iResult == SOCKET_ERROR) {
                        cerr << "[TK] Send failed: " << WSAGetLastError() << endl;
                        closesocket(connectSocket);
                        WSACleanup();
                        return 1;
                    }
                    cout << "[TK] Played move sent successfully." << endl;
                    cout << "========================================\n" << endl;

                }
                else
                {
                    // drugi igrac je na potezu
                    //cout << matchMessage.getMessage() << endl;
                    cout << "Opponents move! Waiting for his move..." << endl;
                    cout << "========================================\n" << endl;

                }
            }
            else
            {
                // kraj igre 
                cout << matchMessage.getMessage() << endl;
                cout << "========================================\n" << endl;
                running = false;
            }

        }
        else if (iResult == 0) {
            cout << "[TK] Server closed the connection." << endl;
            running = false;
        }
        else {
            cerr << "[TK] Failed to receive response." << endl;
            running = false;
        }
    } while (running);

    cout << "\nPress Enter to exit... ";
    cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    cin.get();

    // Cleanup
    closesocket(connectSocket);
    WSACleanup();
    return 0;
}