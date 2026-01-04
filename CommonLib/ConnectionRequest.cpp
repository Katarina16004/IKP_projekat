#include "ConnectionRequest.h"

ConnectionRequest::ConnectionRequest()
    : idStruct(1), idSource(0), idDest(0), checksum(0) {
    username[0] = '\0';
}

ConnectionRequest::ConnectionRequest(int source, int dest, const char* user)
    : idStruct(1), idSource(source), idDest(dest) {
    setUsername(user);
    calculateChecksum();
}

void ConnectionRequest::setUsername(const char* user) {
    if (user != nullptr) {
        strncpy_s(username, sizeof(username), user, _TRUNCATE);
    }
    else {
        username[0] = '\0';
    }
}

void ConnectionRequest::calculateChecksum() {
    checksum = 0;
    checksum += idStruct;
    checksum += idSource;
    checksum += idDest;

    for (size_t i = 0; i < strlen(username); i++) {
        checksum += username[i];
    }
}

bool ConnectionRequest::validateChecksum() const {
    int calculated = 0;
    calculated += idStruct;
    calculated += idSource;
    calculated += idDest;

    for (size_t i = 0; i < strlen(username); i++) {
        calculated += username[i];
    }

    return calculated == checksum;
}

void ConnectionRequest::serialize(char* buffer) const {
    memcpy(buffer, this, sizeof(ConnectionRequest));
}
void ConnectionRequest::deserialize(const char* buffer) {
    memcpy(this, buffer, sizeof(ConnectionRequest));
}