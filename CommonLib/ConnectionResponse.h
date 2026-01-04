#pragma once
#include <iostream>
#include <cstring>

//#define PORT 8090

// TOZ: Server thread responsible for receiving and processing connection request
class ConnectionResponse {
private:
    int idStruct;                // Structure identifier
    int idSource;                // Source ID
    int idDest;                  // Destination ID
    bool accepted;               // Status of connection request - approved or not
    char message[50];            // Message about connection request status
    int idGame;                  // Game ID
    int checksum;                // Checksum for validation

public:
    // Constructor
    ConnectionResponse();
    ConnectionResponse(int source, int dest, bool acc, const char* mess, int game);

    // Getters
    int getIdStruct() const { return idStruct; }
    int getIdSource() const { return idSource; }
    int getIdDest() const { return idDest; }
    bool getAccepted() const { return accepted; }
    const char* getMessage() const { return message; }
    int getIdGame() const { return idGame; }
    int getChecksum() const { return checksum; }

    // Setters
    void setIdSource(int source) { idSource = source; }
    void setIdDest(int dest) { idDest = dest; }
    void setMessage(const char* mess);
    void setAccepted(bool acc) { accepted = acc; }
    void setIdGame(int game) { idGame = game; }

    // Methods
    void calculateChecksum();
    bool validateChecksum() const;

    // For sending/receiving
    void serialize(char* buffer) const;
    void deserialize(const char* buffer);
};