#pragma once
#include <iostream>

//#define PORT 8097

class Move {
private:
    int idStruct;                // Structure identifier
    int idSource;                // Source ID
    int idDest;                  // Destination ID
    int idGame;                  // Game ID
    int x;                       // X coordinate
    int y;                       // Y coordinate
    int checksum;                // Checksum for validation

public:
    // Constructor
    Move();
    Move(int source, int dest, int game, int _x, int _y);

    // Getters
    int getIdStruct() const { return idStruct; }
    int getIdSource() const { return idSource; }
    int getIdDest() const { return idDest; }
    int getIdGame() const { return idGame; }
    int getX() const { return x; }
    int getY() const { return y; }
    int getChecksum() const { return checksum; }

    // Setters
    void setIdSource(int source) { idSource = source; }
    void setIdDest(int dest) { idDest = dest; }
    void setIdGame(int game) { idGame = game; }
    void setX(int _x) { x = _x; }
    void setY(int _y) { y = _y; }

    // Methods
    void calculateChecksum();
    bool validateChecksum() const;

    // For sending/receiving
    void serialize(char* buffer) const;
    void deserialize(const char* buffer);
};
