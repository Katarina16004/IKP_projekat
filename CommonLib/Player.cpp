#include "Player.h"

Player::Player()
	: idStruct(5), idGame(0), acceptedSocket(NULL), idPlayer(0), playerMove(0)
{
	username[0] = '\0';
}

Player::Player(const char* user, SOCKET soc, int game, int player, int move)
	: idStruct(5), acceptedSocket(soc), idGame(game), idPlayer(player), playerMove(move)
{	
	setUsername(user);
}

void Player::setUsername(const char* user) {
    if (user != nullptr) {
        strncpy_s(username, sizeof(username), user, _TRUNCATE);
    }
    else {
        username[0] = '\0';
    }
}

void Player::serialize(char* buffer) const {
    memcpy(buffer, this, sizeof(Player));
}
void Player::deserialize(const char* buffer) {
    memcpy(this, buffer, sizeof(Player));
}