#define _WINSOCK_DEPRECATED_NO_WARNINGS 
#define _CRT_SECURE_NO_WARNINGS
#define NOMINMAX

#include <iostream>
#include <vector>
#include <thread>
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <atomic>
#include <chrono>
#include <algorithm>
#include <random>
#include <string>

#include "../CommonLib/ConnectionRequest.h"
#include "../CommonLib/ConnectionResponse.h"
#include "../CommonLib/MessageForMove.h"
#include "../CommonLib/Move.h"

#pragma comment(lib, "ws2_32.lib")

using namespace std;

atomic<int> successCount(0);
atomic<int> rejectedCount(0);
atomic<int> errorCount(0);
atomic<long long> totalLatencyMs(0);
atomic<int> activeGames(0);
atomic<int> finishedGames(0);
atomic<int> totalMovesMade(0);

void loginRoutine(int id, string serverIp, int port) {
    SOCKET s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (s == INVALID_SOCKET) {
        errorCount++;
        return;
    }

    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = inet_addr(serverIp.c_str());
    serverAddr.sin_port = htons(port);

    auto start = chrono::high_resolution_clock::now();

    if (connect(s, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        errorCount++;
        closesocket(s);
        return;
    }

    string username = "LoginBot_" + to_string(id);
    ConnectionRequest req(id, 100, username.c_str());

    char buffer[512];
    memset(buffer, 0, 512);
    req.serialize(buffer);

    send(s, buffer, 512, 0);
    int bytes = recv(s, buffer, 512, 0);

    auto end = chrono::high_resolution_clock::now();

    if (bytes > 0) {
        ConnectionResponse res;
        res.deserialize(buffer);

        if (res.getAccepted()) {
            successCount++;
            auto duration = chrono::duration_cast<chrono::milliseconds>(end - start).count();
            totalLatencyMs += duration;
        }
        else {
            rejectedCount++;
        }
    }
    else {
        errorCount++;
    }

    closesocket(s);
}

void runLoginFlood(int numClients) {
    successCount = 0;
    rejectedCount = 0;
    errorCount = 0;
    totalLatencyMs = 0;

    cout << "\n[SISTEM] Pokretanje LOGIN FLOOD testa sa " << numClients << " klijenata..." << endl;
    auto testStart = chrono::high_resolution_clock::now();

    vector<thread> threads;
    for (int i = 0; i < numClients; i++) {
        threads.push_back(thread(loginRoutine, i, "127.0.0.1", 27016));
    }

    for (auto& t : threads) {
        if (t.joinable()) t.join();
    }

    auto testEnd = chrono::high_resolution_clock::now();
    chrono::duration<double> totalDuration = testEnd - testStart;

    cout << "\n================ REZULTATI TESTA ================" << endl;
    cout << " Tip testa:             LOGIN FLOOD" << endl;
    cout << " Broj klijenata:        " << numClients << endl;
    cout << " Ukupno vreme:          " << totalDuration.count() << " s" << endl;
    cout << "------------------------------------------------" << endl;
    cout << " Uspesne prijave:       " << successCount << endl;
    cout << " Odbijene prijave:      " << rejectedCount << endl;
    cout << " Greske u mrezi:        " << errorCount << endl;
    if (successCount > 0)
        cout << " Prosecno kasnjenje:    " << (totalLatencyMs / successCount) << " ms" << endl;
    cout << "================================================\n" << endl;
}

void gameRoutine(int id) {
    SOCKET s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (s == INVALID_SOCKET) return;

    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
    serverAddr.sin_port = htons(27016);

    if (connect(s, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        errorCount++;
        closesocket(s);
        return;
    }

    string name = "GameBot_" + to_string(id);
    ConnectionRequest req(id, 100, name.c_str());
    char buffer[512];
    memset(buffer, 0, 512);
    req.serialize(buffer);
    send(s, buffer, 512, 0);

    recv(s, buffer, 512, 0);

    mt19937 rng(static_cast<unsigned int>(time(0) + id));
    bool gameStartedCounted = false;

    bool gameRunning = true;
    while (gameRunning) {
        memset(buffer, 0, 512);
        int bytes = recv(s, buffer, 512, 0);
        if (bytes <= 0) break;

        MessageForMove msg;
        msg.deserialize(buffer);

        if (!gameStartedCounted && !msg.getEnd()) {
            activeGames++;
            gameStartedCounted = true;
        }

        if (msg.getEnd()) {
            finishedGames++;
            if (gameStartedCounted) activeGames--;
            gameRunning = false;
            break;
        }

        if (msg.getPlaying()) {
            struct Pos { int r, c; };
            vector<Pos> freeSpots;
            const int (*board)[3] = msg.getBoard();
            for (int i = 0; i < 3; i++) {
                for (int j = 0; j < 3; j++) {
                    if (board[i][j] == 0) freeSpots.push_back({ i, j });
                }
            }

            if (!freeSpots.empty()) {
                uniform_int_distribution<int> dist(0, (int)freeSpots.size() - 1);
                Pos choice = freeSpots[dist(rng)];
                Move playedMove(msg.getIdDest(), msg.getIdSource(), msg.getIdGame(), choice.r, choice.c);
                memset(buffer, 0, 512);
                playedMove.serialize(buffer);
                uniform_int_distribution<int> waitDist(50, 150);
                this_thread::sleep_for(chrono::milliseconds(waitDist(rng)));
                send(s, buffer, 512, 0);
                totalMovesMade++;
            }
        }
    }
    closesocket(s);
}

void runGameSimulation(int numBots) {
    activeGames = 0;
    finishedGames = 0;
    totalMovesMade = 0;
    errorCount = 0;

    cout << "\n[SISTEM] Pokretanje GAME SIMULATION testa sa " << numBots << " botova..." << endl;
    auto testStart = chrono::high_resolution_clock::now();

    vector<thread> threads;
    for (int i = 0; i < numBots; i++) {
        threads.push_back(thread(gameRoutine, i));
        this_thread::sleep_for(chrono::milliseconds(100));
    }
    for (auto& t : threads) {
        if (t.joinable()) t.join();
    }

    auto testEnd = chrono::high_resolution_clock::now();
    chrono::duration<double> totalDuration = testEnd - testStart;

    cout << "\n================ REZULTATI TESTA ================" << endl;
    cout << " Tip testa:             GAME SIMULATION" << endl;
    cout << " Broj botova:           " << numBots << endl;
    cout << " Ukupno vreme:          " << totalDuration.count() << " s" << endl;
    cout << "------------------------------------------------" << endl;
    cout << " Zavrsene partije:      " << finishedGames / 2 << endl;
    cout << " Ukupno poteza:         " << totalMovesMade << endl;
    cout << " Greske u mrezi:        " << errorCount << endl;
    cout << "================================================\n" << endl;
}

int main() {
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);

    int choice = 0;
    while (choice != 3) {
        cout << "========== TIC TAC TOE STRESS TOOL ==========" << endl;
        cout << " 1. Login Flood Test" << endl;
        cout << " 2. Full Game Simulation" << endl;
        cout << " 3. Izlaz" << endl;
        cout << "---------------------------------------------" << endl;
        cout << " Vas izbor: ";
        cin >> choice;

        if (choice == 1) runLoginFlood(100);
        else if (choice == 2) runGameSimulation(50);
    }

    WSACleanup();
    return 0;
}