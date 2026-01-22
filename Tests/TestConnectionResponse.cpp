#include "pch.h"
#include <gtest/gtest.h>
#include "../CommonLib/ConnectionResponse.h"

#pragma region PositiveTests

//da li konstruktor i racunanje checksume radi ispravno
TEST(ConnectionResponseTest, ConstructAndValidateChecksum) {
    ConnectionResponse resp(100, 200, true, "Success", 93);
    EXPECT_EQ(resp.getIdSource(), 100);
    EXPECT_TRUE(resp.validateChecksum());
    EXPECT_STREQ(resp.getMessage(), "Success");
}

//da li serialize() i deserialize() funkcije tacno prebacuju ceo objekat u bajtni format i nazad
TEST(ConnectionResponseTest, SerializationRoundtrip) {
    ConnectionResponse resp(1, 2, false, "Invalid username", 999);
    char buf[sizeof(ConnectionResponse)]{};
    resp.serialize(buf);

    ConnectionResponse r2;
    r2.deserialize(buf);

    EXPECT_EQ(r2.getIdGame(), 999);
    EXPECT_FALSE(r2.getAccepted());
    EXPECT_STREQ(r2.getMessage(), "Invalid username");
    EXPECT_TRUE(r2.validateChecksum());
}

//da li je podrazumevani objekat ispravno postavljen
TEST(ConnectionResponseTest, DefaultConstructedIsValid) {
    ConnectionResponse resp;
    EXPECT_EQ(resp.getIdStruct(), 2);
    EXPECT_EQ(resp.getIdSource(), 0);
    EXPECT_EQ(resp.getIdDest(), 0);
    EXPECT_STREQ(resp.getMessage(), "");
}

#pragma endregion

#pragma region NegativeTests

// rucno menjanje checksume
TEST(ConnectionResponseTest, InvalidChecksumShouldFailValidation) {
    ConnectionResponse resp(10, 20, true, "BadTest", 30);

    // nepravilno menjamo podatke posle izracunavanja checksuma
    char buf[sizeof(ConnectionResponse)]{};
    resp.serialize(buf);

    // rucno sabotiranje checksume
    ConnectionResponse broken;
    broken.deserialize(buf);

    *((int*)((char*)&broken + sizeof(ConnectionResponse) - sizeof(int))) += 1;

    EXPECT_FALSE(broken.validateChecksum());
}

// pravimo pravilan objekat pri serijalizaciji i u medjuvremenu ga pokvarimo, da proverimo deserijalizaciju
TEST(ConnectionResponseTest, DeserializeFromCorruptedBuffer) {
    ConnectionResponse resp(10, 20, true, "Corrupted", 30);
    char buf[sizeof(ConnectionResponse)]{};
    resp.serialize(buf);

    buf[5] ^= 0xFF; // flipujemo jedan bajt

    ConnectionResponse broken;
    broken.deserialize(buf);

    EXPECT_FALSE(broken.validateChecksum());
}

#pragma endregion