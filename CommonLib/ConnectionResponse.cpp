#include "pch.h"
#include "ConnectionResponse.h"

ConnectionResponse::ConnectionResponse()
	: idStruct(2), idSource(0), idDest(0), accepted(false), idGame(0), checksum(0)
{
	message[0] = '\0';
}

ConnectionResponse::ConnectionResponse(int source, int dest, bool acc, const char* mess, int game)
	: idStruct(2), idSource(source), idDest(dest), accepted(acc), idGame(game), checksum(0)
{
	setMessage(mess);
	calculateChecksum();
}

void ConnectionResponse::setMessage(const char* mess) {
    if (mess != nullptr) {
        strncpy_s(message, sizeof(message), mess, _TRUNCATE);
    }
    else {
        message[0] = '\0';
    }
}

void ConnectionResponse::calculateChecksum() {
    checksum = 0;
    checksum += idStruct;
    checksum += idSource;
    checksum += idDest;
    checksum += idGame;

    for (size_t i = 0; i < strlen(message); i++) {
        checksum += message[i];
    }
}

bool ConnectionResponse::validateChecksum() const {
    int calculated = 0;
    calculated += idStruct;
    calculated += idSource;
    calculated += idDest;
    calculated += idGame;

    for (size_t i = 0; i < strlen(message); i++) {
        calculated += message[i];
    }

    return calculated == checksum;
}

void ConnectionResponse::serialize(char* buffer) const {
    memcpy(buffer, this, sizeof(ConnectionResponse));
}
void ConnectionResponse::deserialize(const char* buffer) {
    memcpy(this, buffer, sizeof(ConnectionResponse));
}