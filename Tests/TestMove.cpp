#include "pch.h"
#include <gtest/gtest.h>
#include "../CommonLib/Move.h"

#pragma region PositiveTests

// da li podrazumevani konstruktor postavlja ocekivane vrednosti
TEST(MoveTest, DefaultConstructedIsValid) {
    Move move;
    EXPECT_EQ(move.getIdStruct(), 4);
    EXPECT_EQ(move.getIdSource(), 0);
    EXPECT_EQ(move.getIdDest(), 0);
    EXPECT_EQ(move.getIdGame(), 0);
    EXPECT_EQ(move.getX(), 0);
    EXPECT_EQ(move.getY(), 0);
    EXPECT_EQ(move.getChecksum(), 0);
}

// da li parametarski konstruktor ispravno postavlja podatke i izracunava checksum
TEST(MoveTest, ParamConstructorAndChecksum) {
    Move move(2, 5, 11, 1, 2);
    EXPECT_EQ(move.getIdStruct(), 4);
    EXPECT_EQ(move.getIdSource(), 2);
    EXPECT_EQ(move.getIdDest(), 5);
    EXPECT_EQ(move.getIdGame(), 11);
    EXPECT_EQ(move.getX(), 1);
    EXPECT_EQ(move.getY(), 2);
    EXPECT_TRUE(move.validateChecksum());
}

// Serialize/Deserialize - svi podaci su isti i checksum OK
TEST(MoveTest, SerializationRoundtrip) {
    Move original(7, 8, 20, 1, 0);
    char buffer[sizeof(Move)]{};
    original.serialize(buffer);

    Move restored;
    restored.deserialize(buffer);

    EXPECT_EQ(restored.getIdStruct(), 4);
    EXPECT_EQ(restored.getIdSource(), 7);
    EXPECT_EQ(restored.getIdDest(), 8);
    EXPECT_EQ(restored.getIdGame(), 20);
    EXPECT_EQ(restored.getX(), 1);
    EXPECT_EQ(restored.getY(), 0);
    EXPECT_TRUE(restored.validateChecksum());
}

#pragma endregion

#pragma region NegativeTests

// rucno pokvaren checksum: menjamo poslednji bajt (checksum)
TEST(MoveTest, ChecksumInvalidWhenCorrupted) {
    Move move(1, 2, 3, 4, 5);
    char buffer[sizeof(Move)]{};
    move.serialize(buffer);

    // pokvarimo checksum -> poslednjih sizeof(int) bajtova je checksum
    ((int*)(buffer + sizeof(Move) - sizeof(int)))[0] += 12345; 

    Move broken;
    broken.deserialize(buffer);

    EXPECT_FALSE(broken.validateChecksum());
}

// rucno menjanje jednog podatka (x), checksum vise ne vazi
TEST(MoveTest, CorruptFieldDetectedByChecksum) {
    Move move(9, 1, 7, 1, 1);
    char buffer[sizeof(Move)]{};
    move.serialize(buffer);

    // menjamo x koordinatu: nalazi se iza 4 int-a
    size_t xOffset = sizeof(int) * 4;
    buffer[xOffset]++;

    Move corrupted;
    corrupted.deserialize(buffer);

    EXPECT_FALSE(corrupted.validateChecksum());
}

#pragma endregion