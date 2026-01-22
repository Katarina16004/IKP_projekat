#include "pch.h"
#include "Move.h"

Move::Move()
	: idStruct(4), idSource(0), idDest(0), idGame(0), x(0), y(0), checksum(0)
{}

Move::Move(int source, int dest, int game, int _x, int _y)
	: idStruct(4), idSource(source), idDest(dest), idGame(game), x(_x), y(_y)
{
    calculateChecksum();
}

void Move::calculateChecksum() {
    checksum = 0;
    checksum += idStruct;
    checksum += idSource;
    checksum += idDest;
    checksum += idGame;
    checksum += x; 
    checksum += y;

}

bool Move::validateChecksum() const {
    int calculated = 0;
    calculated += idStruct;
    calculated += idSource;
    calculated += idDest;
    calculated += idGame;
    calculated += x;
    calculated += y;

    return calculated == checksum;
}

void Move::serialize(char* buffer) const {
    memcpy(buffer, this, sizeof(Move));
}
void Move::deserialize(const char* buffer) {
    memcpy(this, buffer, sizeof(Move));
}