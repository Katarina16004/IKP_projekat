#include "pch.h"
#include "MessageForMove.h"

MessageForMove::MessageForMove()
	: idStruct(3), idSource(0), idDest(0), idGame(0), end(true), playing(false), checksum(0), move(0)
{
	message[0] = '\0';
	
	for (int i = 0; i < 3; i++)
	{
		for (int j = 0; j < 3; j++)
		{
			board[i][j] = '0';
		}
	}
}
MessageForMove::MessageForMove(int source, int dest, int game, bool e, bool pl, const char* mess, const int(&b)[3][3], int _move)
	: idStruct(3), idSource(source), idDest(dest), idGame(game), end(e), playing(pl), move(_move)
{
    setMessage(mess);
	setBoard(b);
	calculateChecksum();
}

void MessageForMove::setMessage(const char* mess) {
    if (mess != nullptr) {
        strncpy_s(message, sizeof(message), mess, _TRUNCATE);
    }
    else {
        message[0] = '\0';
    }
}

void MessageForMove::calculateChecksum() {
    checksum = 0;
    checksum += idStruct;
    checksum += idSource;
    checksum += idDest;
    checksum += idGame;

    for (size_t i = 0; i < strlen(message); i++) {
        checksum += message[i];
    }
}

bool MessageForMove::validateChecksum() const {
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

void MessageForMove::serialize(char* buffer) const {
    memcpy(buffer, this, sizeof(MessageForMove));
}
void MessageForMove::deserialize(const char* buffer) {
    memcpy(this, buffer, sizeof(MessageForMove));
}
