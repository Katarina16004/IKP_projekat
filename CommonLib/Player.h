#pragma once
#include <iostream>
#include <cstring>
#include <WinSock2.h>

//#define PORT 8097

class Player {
private:
    int idStruct;                // Structure identifier
    char username[50];           // Player username
    SOCKET acceptedSocket;       // Socket for communication with player
    int idGame;                  // Game ID

public:
    // Constructor
    Player();
    Player(const char* user, SOCKET soc, int game);

    // Getters
    int getIdStruct() const { return idStruct; }
    int getIdGame() const { return idGame; }
    SOCKET getAcceptedSocket() const { return acceptedSocket; }
    const char* getUsername() const { return username; }

    // Setters
    void setIdGame(int game) { idGame = game; }
    void setAcceptedSocket(SOCKET soc) { acceptedSocket = soc; }
    void setUsername(const char* user);

    // For sending/receiving
    void serialize(char* buffer) const;
    void deserialize(const char* buffer);
};
