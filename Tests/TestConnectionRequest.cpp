#include "pch.h"
#include <gtest/gtest.h>
#include "../CommonLib/ConnectionRequest.h"

//da li je podrazumevani objekat ispravno postavljen
TEST(ConnectionRequestTest, DefaultConstructedIsValid) {
    ConnectionRequest req;
    EXPECT_EQ(req.getIdStruct(), 1);
    EXPECT_EQ(req.getIdSource(), 0);
    EXPECT_EQ(req.getIdDest(), 0);
    EXPECT_STREQ(req.getUsername(), "");
}

//da li konstruktor i racunanje checksume radi ispravno
TEST(ConnectionRequestTest, ConstructionAndChecksumValidation) {
    ConnectionRequest req(42, 77, "Tester");
    EXPECT_STREQ(req.getUsername(), "Tester");
    EXPECT_TRUE(req.validateChecksum());
}

//da li serialize() i deserialize() funkcije tacno prebacuju ceo objekat u bajtni format i nazad
TEST(ConnectionRequestTest, SerializationRoundtrip) {
    ConnectionRequest req(4, 5, "Alice");
    char buf[sizeof(ConnectionRequest)]{};
    req.serialize(buf);

    ConnectionRequest other;
    other.deserialize(buf);
    EXPECT_EQ(other.getIdStruct(), 1);
    EXPECT_STREQ(other.getUsername(), "Alice");
    EXPECT_TRUE(other.validateChecksum());
}

#pragma region NegativeTests

// rucno menjanje checksume
TEST(ConnectionRequestTest, InvalidChecksumShouldFailValidation) {
    ConnectionRequest req(10, 20, "BadTest");

    // nepravilno menjamo podatke posle izracunavanja checksuma
    char buf[sizeof(ConnectionRequest)]{};
    req.serialize(buf);

    // rucno sabotiranje checksume
    ConnectionRequest broken;
    broken.deserialize(buf);

    *((int*)((char*)&broken + sizeof(ConnectionRequest) - sizeof(int))) += 1;

    EXPECT_FALSE(broken.validateChecksum());
}

// pravimo pravilan objekat pri serijalizaciji i u medjuvremenu ga pokvarimo, da proverimo deserijalizaciju
TEST(ConnectionRequestTest, DeserializeFromCorruptedBuffer) {
    ConnectionRequest req(15, 25, "Corrupted");
    char buf[sizeof(ConnectionRequest)]{};
    req.serialize(buf);

    buf[5] ^= 0xFF; // flipujemo jedan bajt

    ConnectionRequest broken;
    broken.deserialize(buf);

    EXPECT_FALSE(broken.validateChecksum());
}

#pragma endregion