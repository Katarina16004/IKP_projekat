#pragma once
#include <iostream>
#include <cstring>

//#define PORT 8095

class MessageForMove {
private:
    int idStruct;                // Structure identifier
    int idSource;                // Source ID
    int idDest;                  // Destination ID
    int idGame;                  // Game ID
    bool end;                    // End of game
    bool playing;                // Signalizes if player is on the move or not
    int move;                    // 1 for X, 2 for O
    char message[70];            // Message about connection request status
    int board[3][3];             // Board with moves from both players
    int checksum;                // Checksum for validation

public:
    // Constructor
    MessageForMove();
    MessageForMove(int source, int dest, int game, bool e, bool pl, const char* mess, const int(&b)[3][3], int _move);

    // Getters
    int getIdStruct() const { return idStruct; }
    int getIdSource() const { return idSource; }
    int getIdDest() const { return idDest; }
    int getIdGame() const { return idGame; }
    const char* getMessage() const { return message; }
    bool getEnd() const { return end; }
    bool getPlaying() const { return playing; }
    const int (*getBoard() const)[3] { return board; }    
    int getChecksum() const { return checksum; }
    int getMove() const { return move; }

    // Setters
    void setIdSource(int source) { idSource = source; }
    void setIdDest(int dest) { idDest = dest; }
    void setMessage(const char* mess);
    void setEnd(bool e) { end = e; }
    void setPlaying(bool pl) { playing = pl; }
    void setBoard(const int(&b)[3][3]) { memcpy(board, b, sizeof(board)); }
    void setIdGame(int game) { idGame = game; }
    void setMove(int _move) { move = _move; }

    // Methods
    void calculateChecksum();
    bool validateChecksum() const;

    // For sending/receiving
    void serialize(char* buffer) const;
    void deserialize(const char* buffer);
};
