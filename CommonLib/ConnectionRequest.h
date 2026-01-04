#pragma once
#include <iostream>
#include <cstring>

//#define PORT 8080

// TK: Client thread responsible for sending connection request
class ConnectionRequest {
private:
    int idStruct;                // Structure identifier
    int idSource;                // Source ID
    int idDest;                  // Destination ID
    char username[50];           // Client username
    int checksum;                // Checksum for validation

public:
    // Constructor
    ConnectionRequest();
    ConnectionRequest(int source, int dest, const char* user);

    // Getters
    int getIdStruct() const { return idStruct; }
    int getIdSource() const { return idSource; }
    int getIdDest() const { return idDest; }
    const char* getUsername() const { return username; }
    int getChecksum() const { return checksum; }

    // Setters
    void setIdSource(int source) { idSource = source; }
    void setIdDest(int dest) { idDest = dest; }
    void setUsername(const char* user);

    // Methods
    void calculateChecksum();
    bool validateChecksum() const;

    // For sending/receiving
    void serialize(char* buffer) const;
    void deserialize(const char* buffer);
};