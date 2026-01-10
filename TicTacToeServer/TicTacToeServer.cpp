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
#include "../CommonLib/MessageForMove.h"
#include "../CommonLib/Move.h"

// TEMPORARY
#ifndef COMMONLIB_INCLUDED
#define COMMONLIB_INCLUDED
#include "../CommonLib/ConnectionRequest.cpp"
#include "../CommonLib/ConnectionResponse.cpp"
#include "../CommonLib/Player.cpp"
#include "../CommonLib/MessageForMove.cpp"
#include "../CommonLib/Move.cpp"
#endif

#pragma comment(lib, "ws2_32.lib")

#define DEFAULT_PORT 27016
#define DEFAULT_BUFLEN 512
#define SERVER_ID 100

using namespace std;

List<Player> waitingPlayers;
mutex waitingMutex;

int nextGameId = 1;
int nextClientId = 1;
mutex idMutex;

bool serverRunning = true;

void ClientConnectionHandler(SOCKET clientSocket);  // TOZ thread
void MatchmakingThread();  // TS thread
void PlayingThread(Player player1, Player player2); // TG thread

bool recvWithTimeout(SOCKET sock, char* buffer, int bufLen, int timeoutSeconds) {
    fd_set readfds;
    FD_ZERO(&readfds);
    FD_SET(sock, &readfds);

    // timeout
    timeval tv;
    tv.tv_sec = timeoutSeconds;
    tv.tv_usec = 0;

    int result = select(0, &readfds, NULL, NULL, &tv);

    if (result > 0) {
        // podaci spremni za recv
        int iResult = recv(sock, buffer, bufLen, 0);
        return iResult > 0;  // true ako je primljeno
    }
    else if (result == 0) {
        // timeout
        return false;
    }
    else {
        // ako se desi greska
        std::cerr << "select() failed: " << WSAGetLastError() << std::endl;
        return false;
    }
}

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
        cerr << "WSAStartup failed:  " << iResult << endl;
        return 1;
    }

    // Create socket
    SOCKET listenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listenSocket == INVALID_SOCKET) {
        cerr << "Socket creation failed:   " << WSAGetLastError() << endl;
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
        cerr << "Bind failed:  " << WSAGetLastError() << endl;
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

    cout << "[TOZ] Connection request from:   " << request.getUsername() << endl;

    // Validate checksum
    if (!request.validateChecksum()) {
        cout << "[TOZ] [X] Invalid checksum!  Rejecting connection." << endl;

        ConnectionResponse response(SERVER_ID, request.getIdSource(), false,
            "Invalid checksum - connection rejected", 0);

        memset(sendBuffer, 0, DEFAULT_BUFLEN);
        response.serialize(sendBuffer);
        send(clientSocket, sendBuffer, DEFAULT_BUFLEN, 0);

        closesocket(clientSocket);
        cout << "[TOZ] Connection rejected.  Thread ending.\n" << endl;
        return;
    }

    cout << "[TOZ] [OK] Checksum valid." << endl;

    //username validation
    bool usernameValid = false;
    string username;

    while (!usernameValid) {
        username = request.getUsername();

        bool usernameTaken = false;
        {
            lock_guard<mutex> lock(waitingMutex);

            for (int i = 1; i <= waitingPlayers.size(); i++) {
                Player existingPlayer;
                waitingPlayers.read(i, existingPlayer);

                if (strcmp(existingPlayer.getUsername(), username.c_str()) == 0) {
                    usernameTaken = true;
                    break;
                }
            }
        }

        if (usernameTaken) {
            cout << "[TOZ] [X] Username '" << username << "' already exists!  Asking for new username." << endl;

            ConnectionResponse response(SERVER_ID, request.getIdSource(), false,
                "Username already taken - choose another name", 0);

            memset(sendBuffer, 0, DEFAULT_BUFLEN);
            response.serialize(sendBuffer);
            send(clientSocket, sendBuffer, DEFAULT_BUFLEN, 0);

            //Wait for new username request
            memset(recvBuffer, 0, DEFAULT_BUFLEN);
            iResult = recv(clientSocket, recvBuffer, DEFAULT_BUFLEN, 0);

            if (iResult <= 0) {
                cout << "[TOZ] Client disconnected." << endl;
                closesocket(clientSocket);
                return;
            }

            request.deserialize(recvBuffer);
            cout << "[TOZ] New username attempt:  " << request.getUsername() << endl;
        }
        else {
            usernameValid = true;
            cout << "[TOZ] [OK] Username '" << username << "' is unique." << endl;
        }
    }

    // Accept connection and assign tentative game ID
    int gameId, playerId;
    {
        lock_guard<mutex> lock(idMutex);
        gameId = nextGameId;
        playerId = nextClientId;
        nextClientId++;
    }

    cout << "[TOZ] [OK] CONNECTION ACCEPTED" << endl;
    cout << "[TOZ]   Username: " << username << endl;
    cout << "[TOZ]   Tentative Game ID: " << gameId << endl;
    cout << "[TOZ]   Tentative Player ID:  " << playerId << endl;

    ConnectionResponse response(SERVER_ID, request.getIdSource(), true,
        "Connection accepted!   Waiting for opponent.. .", gameId);

    memset(sendBuffer, 0, DEFAULT_BUFLEN);
    response.serialize(sendBuffer);
    send(clientSocket, sendBuffer, DEFAULT_BUFLEN, 0);

    // Create Player object and add to waiting list
    int move = 0;
    if (waitingPlayers.size() % 2 == 0)
    {
        move = 1;
    }
    else
    {
        move = 2;
    }

    Player player(username.c_str(), clientSocket, gameId, playerId, move); 

    {
        lock_guard<mutex> lock(waitingMutex);
        waitingPlayers.add(waitingPlayers.size() + 1, player);
        cout << "[TOZ] Player '" << player.getUsername() << "' added to waiting list." << endl;
        cout << "[TOZ] Waiting players:  " << waitingPlayers.size() << endl;
    }

    cout << "[TOZ] Client connection handler finished.   Player is waiting.\n" << endl;
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

        thread gameThread(PlayingThread, player1, player2);
        gameThread.detach();
    }

    cout << "[TS] Matchmaking thread ending." << endl;
}

//TG
void PlayingThread(Player player1, Player player2) {
    const int TIMEOUT_SECONDS = 20; // timeout po potezu - za testiranje moze i manje da se stavi

    bool bothAlive = true;

    char buffer[DEFAULT_BUFLEN];
    int board[3][3] = { 0 };

    MessageForMove msg1(SERVER_ID, player1.getIdPlayer(), player1.getIdGame(), false, true,
        "Game found!   You are Player X.  You are playing first.. .", board, 1);
    memset(buffer, 0, DEFAULT_BUFLEN);
    msg1.serialize(buffer);
    if (send(player1.getAcceptedSocket(), buffer, DEFAULT_BUFLEN, 0) == SOCKET_ERROR)
    {
        cout << "=================================================" << endl;
        cout << "[TG] Game  " << player1.getIdGame() << " Player 1 disconnected before matchmaking. Removed from players waiting list." << endl;
        cout << "=================================================\n" << endl;

        bothAlive = false;
        nextGameId--;

        // Player1 diskonektovan, Player2 se vraća u listu čekanja
        lock_guard<mutex> lock(waitingMutex);
        waitingPlayers.add(waitingPlayers.size() + 1, player2);
        closesocket(player1.getAcceptedSocket());
        return;
    }
    

    MessageForMove msg2(SERVER_ID, player2.getIdPlayer(), player2.getIdGame(), false, false,
        "Game found!  You are Player O. You are playing second...", board, 2);
    memset(buffer, 0, DEFAULT_BUFLEN);
    msg2.serialize(buffer);
    if (send(player2.getAcceptedSocket(), buffer, DEFAULT_BUFLEN, 0) == SOCKET_ERROR) 
    {
        cout << "=================================================" << endl;
        cout << "[TG] Game  " << player2.getIdGame() << " Player 2 disconnected before matchmaking. Removed from players waiting list." << endl;
        cout << "=================================================\n" << endl;

        bothAlive = false;
        nextGameId--;

        // Player2 diskonektovan, Player1 se vraća u listu čekanja
        lock_guard<mutex> lock(waitingMutex);
        waitingPlayers.add(waitingPlayers.size() + 1, player1);
        closesocket(player2.getAcceptedSocket());
        return;
    }

    if (bothAlive)
    {
        cout << "=================================================" << endl;
        cout << "[TG] Game " << player1.getIdGame() << " notifications sent to both players." << endl;
        cout << "=================================================\n" << endl;

        int iResult;

        bool playing = true;
        while (playing) {
            // player 1 move
            memset(buffer, 0, DEFAULT_BUFLEN);
            if (!recvWithTimeout(player1.getAcceptedSocket(), buffer, DEFAULT_BUFLEN, TIMEOUT_SECONDS)) {
                cout << "[TG] Game  " << player1.getIdGame() << " Player 1 timed out or disconnected.  Ending game." << endl;

                MessageForMove timeoutMsg1(SERVER_ID, player1.getIdPlayer(), player1.getIdGame(),
                    true, false, "Timeout!  You lost the game.", board, 1);
                MessageForMove timeoutMsg2(SERVER_ID, player2.getIdPlayer(), player2.getIdGame(),
                    true, true, "Opponent timed out!  You win!", board, 2);

                memset(buffer, 0, DEFAULT_BUFLEN);
                timeoutMsg1.serialize(buffer);
                send(player1.getAcceptedSocket(), buffer, DEFAULT_BUFLEN, 0);
                memset(buffer, 0, DEFAULT_BUFLEN);
                timeoutMsg2.serialize(buffer);
                send(player2.getAcceptedSocket(), buffer, DEFAULT_BUFLEN, 0);

                closesocket(player1.getAcceptedSocket());
                closesocket(player2.getAcceptedSocket());
                return;
            }

            Move playedMove1;
            playedMove1.deserialize(buffer);

            board[playedMove1.getX()][playedMove1.getY()] = 1;
            const char* message = "Player 1 played his move. ";
            bool win1 = false;

            for (int i = 0; i < 3; ++i) {
                if ((board[i][0] == 1 && board[i][1] == 1 && board[i][2] == 1) ||
                    (board[0][i] == 1 && board[1][i] == 1 && board[2][i] == 1)) win1 = true;
            }
            if ((board[0][0] == 1 && board[1][1] == 1 && board[2][2] == 1) ||
                (board[0][2] == 1 && board[1][1] == 1 && board[2][0] == 1)) win1 = true;

            if (win1)
            {
                cout << "=================================================\n" << endl;
                cout << "[TG] Game " << player1.getIdGame() << "  =>  " << "PLAYER 1 WINS! !!  Game ending!" << endl;
                cout << "=================================================\n" << endl;

                MessageForMove finalMsg1(SERVER_ID, player1.getIdPlayer(), player1.getIdGame(), win1, true,
                    "YOU WIN! !! Game ending!", board, 1);
                memset(buffer, 0, DEFAULT_BUFLEN);
                finalMsg1.serialize(buffer);
                send(player1.getAcceptedSocket(), buffer, DEFAULT_BUFLEN, 0);

                MessageForMove finalMsg2(SERVER_ID, player2.getIdPlayer(), player2.getIdGame(), win1, true,
                    "You lost.  Player 1 wins.  Game ending!", board, 2);
                memset(buffer, 0, DEFAULT_BUFLEN);
                finalMsg2.serialize(buffer);
                send(player2.getAcceptedSocket(), buffer, DEFAULT_BUFLEN, 0);

                closesocket(player1.getAcceptedSocket());
                closesocket(player2.getAcceptedSocket());
                return;
            }

            MessageForMove msg1(SERVER_ID, player1.getIdPlayer(), player1.getIdGame(), win1, false,
                message, board, 1);
            memset(buffer, 0, DEFAULT_BUFLEN);
            msg1.serialize(buffer);
            send(player1.getAcceptedSocket(), buffer, DEFAULT_BUFLEN, 0);

            MessageForMove msg2(SERVER_ID, player2.getIdPlayer(), player2.getIdGame(), win1, true,
                message, board, 2);
            memset(buffer, 0, DEFAULT_BUFLEN);
            msg2.serialize(buffer);
            send(player2.getAcceptedSocket(), buffer, DEFAULT_BUFLEN, 0);

            // player 2 move
            memset(buffer, 0, DEFAULT_BUFLEN);
            if (!recvWithTimeout(player2.getAcceptedSocket(), buffer, DEFAULT_BUFLEN, TIMEOUT_SECONDS)) {
                cout << "[TG] Game  " << player2.getIdGame() << " Player 2 timed out or disconnected. Ending game." << endl;

                MessageForMove timeoutMsg2(SERVER_ID, player2.getIdPlayer(), player2.getIdGame(),
                    true, false, "Timeout! You lost the game.", board, 2);
                MessageForMove timeoutMsg1(SERVER_ID, player1.getIdPlayer(), player1.getIdGame(),
                    true, true, "Opponent timed out! You win!", board, 1);

                memset(buffer, 0, DEFAULT_BUFLEN);
                timeoutMsg2.serialize(buffer); send(player2.getAcceptedSocket(), buffer, DEFAULT_BUFLEN, 0);
                memset(buffer, 0, DEFAULT_BUFLEN);
                timeoutMsg1.serialize(buffer); send(player1.getAcceptedSocket(), buffer, DEFAULT_BUFLEN, 0);

                closesocket(player1.getAcceptedSocket());
                closesocket(player2.getAcceptedSocket());
                return;
            }

            Move playedMove2;
            playedMove2.deserialize(buffer);

            board[playedMove2.getX()][playedMove2.getY()] = 2;
            const char* message2 = "Player 2 played his move.";
            bool win2 = false;

            for (int i = 0; i < 3; ++i) {
                if ((board[i][0] == 2 && board[i][1] == 2 && board[i][2] == 2) ||
                    (board[0][i] == 2 && board[1][i] == 2 && board[2][i] == 2)) win2 = true;
            }
            if ((board[0][0] == 2 && board[1][1] == 2 && board[2][2] == 2) ||
                (board[0][2] == 2 && board[1][1] == 2 && board[2][0] == 2)) win2 = true;

            if (win2)
            {
                cout << "=================================================\n" << endl;
                cout << "[TG] Game " << player1.getIdGame() << "  =>  " << "PLAYER 2 WINS!!! Game ending!" << endl;
                cout << "=================================================\n" << endl;

                MessageForMove finalMsg1(SERVER_ID, player1.getIdPlayer(), player1.getIdGame(), win2, true,
                    "You lost. Player 2 wins.  Game ending!", board, 1);
                memset(buffer, 0, DEFAULT_BUFLEN);
                finalMsg1.serialize(buffer);
                send(player1.getAcceptedSocket(), buffer, DEFAULT_BUFLEN, 0);

                MessageForMove finalMsg2(SERVER_ID, player2.getIdPlayer(), player2.getIdGame(), win2, true,
                    "YOU WIN!!! Game ending!", board, 2);
                memset(buffer, 0, DEFAULT_BUFLEN);
                finalMsg2.serialize(buffer);
                send(player2.getAcceptedSocket(), buffer, DEFAULT_BUFLEN, 0);

                closesocket(player1.getAcceptedSocket());
                closesocket(player2.getAcceptedSocket());
                return;
            }

            MessageForMove msg12(SERVER_ID, player1.getIdPlayer(), player1.getIdGame(), win2, true,
                message2, board, 1);
            memset(buffer, 0, DEFAULT_BUFLEN);
            msg12.serialize(buffer);
            send(player1.getAcceptedSocket(), buffer, DEFAULT_BUFLEN, 0);

            MessageForMove msg22(SERVER_ID, player2.getIdPlayer(), player2.getIdGame(), win2, false,
                message2, board, 2);
            memset(buffer, 0, DEFAULT_BUFLEN);
            msg22.serialize(buffer);
            send(player2.getAcceptedSocket(), buffer, DEFAULT_BUFLEN, 0);

        }

        closesocket(player1.getAcceptedSocket());
        closesocket(player2.getAcceptedSocket());
    }
}