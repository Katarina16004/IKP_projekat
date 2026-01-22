#include "pch.h"
#include <gtest/gtest.h>
#include "../CommonLib/MessageForMove.h"

#pragma region PositiveTests

// da li podrazumevani konstruktor postavlja ocekivane vrednosti
TEST(MessageForMoveTest, DefaultConstructedIsValid) {
    MessageForMove msg;
    EXPECT_EQ(msg.getIdStruct(), 3);
    EXPECT_EQ(msg.getIdSource(), 0);
    EXPECT_EQ(msg.getIdDest(), 0);
    EXPECT_EQ(msg.getIdGame(), 0);
    EXPECT_TRUE(msg.getEnd());
    EXPECT_FALSE(msg.getPlaying());
    EXPECT_EQ(msg.getMove(), 0);
    EXPECT_STREQ(msg.getMessage(), "");
    // Tablu proveravamo da je svuda '0'
    const int (*board)[3] = msg.getBoard();
    for (int i = 0; i < 3; i++)
        for (int j = 0; j < 3; j++)
            EXPECT_EQ(board[i][j], '0');
}

// parametarski konstruktor sa svim podacima
TEST(MessageForMoveTest, ParamConstructorSetsAllFields) {
    int customBoard[3][3] = { {1,2,1},{0,1,2},{2,2,1} };
    MessageForMove msg(7, 5, 150, false, true, "Igrac X na potezu", customBoard, 2);

    EXPECT_EQ(msg.getIdStruct(), 3);
    EXPECT_EQ(msg.getIdSource(), 7);
    EXPECT_EQ(msg.getIdDest(), 5);
    EXPECT_EQ(msg.getIdGame(), 150);
    EXPECT_FALSE(msg.getEnd());
    EXPECT_TRUE(msg.getPlaying());
    EXPECT_EQ(msg.getMove(), 2);
    EXPECT_STREQ(msg.getMessage(), "Igrac X na potezu");
    const int (*board)[3] = msg.getBoard();
    for (int i = 0; i < 3; i++)
        for (int j = 0; j < 3; j++)
            EXPECT_EQ(board[i][j], customBoard[i][j]);
    EXPECT_TRUE(msg.validateChecksum());
}

// Serijalizacija i deserijalizacija rade ispravno
TEST(MessageForMoveTest, SerializationRoundtrip) {
    int customBoard[3][3] = { {1,1,0},{2,1,0},{0,2,2} };
    MessageForMove original(3, 4, 50, false, false, "Pocinje krug", customBoard, 1);
    char buffer[sizeof(MessageForMove)]{};
    original.serialize(buffer);

    MessageForMove copy;
    copy.deserialize(buffer);

    EXPECT_EQ(copy.getIdStruct(), 3);
    EXPECT_EQ(copy.getIdSource(), 3);
    EXPECT_EQ(copy.getIdDest(), 4);
    EXPECT_EQ(copy.getIdGame(), 50);
    EXPECT_STREQ(copy.getMessage(), "Pocinje krug");
    EXPECT_EQ(copy.getMove(), 1);
    const int (*board)[3] = copy.getBoard();
    for (int i = 0; i < 3; i++)
        for (int j = 0; j < 3; j++)
            EXPECT_EQ(board[i][j], customBoard[i][j]);
    EXPECT_TRUE(copy.validateChecksum());
}

#pragma endregion

#pragma region NegativeTests

// rucno izmenjen checksum
TEST(MessageForMoveTest, InvalidChecksumAfterManualCorruption) {
    int tabla[3][3] = { {1,0,0},{2,1,2},{2,1,0} };
    MessageForMove msg(2, 8, 33, false, false, "Cheksum sabotiran", tabla, 2);
    char buffer[sizeof(MessageForMove)]{};
    msg.serialize(buffer);

    // sabotiramo checksum (pretpostavljamo da je na kraju strukture) 
    ((int*)(buffer + sizeof(MessageForMove) - sizeof(int)))[0] += 123;

    MessageForMove cor;
    cor.deserialize(buffer);
    EXPECT_FALSE(cor.validateChecksum());
}

// buffer korumpiran na message polju
TEST(MessageForMoveTest, CorruptMessageDetectedByChecksum) {
    int tabla[3][3] = { {0,0,0},{0,0,0},{0,0,0} };
    MessageForMove msg(1, 2, 3, true, true, "Poruka", tabla, 1);
    char buffer[sizeof(MessageForMove)]{};
    msg.serialize(buffer);

    // korumpiramo jedan karakter iz message niza
    const int msgOffset = sizeof(int) * 6 + sizeof(bool) * 2; // idStruct, idSource, idDest, idGame, move, board, 2x bool
    buffer[msgOffset] = 'X';

    MessageForMove broken;
    broken.deserialize(buffer);

    EXPECT_FALSE(broken.validateChecksum());
}

#pragma endregion