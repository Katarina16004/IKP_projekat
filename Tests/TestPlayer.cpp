#include "pch.h"
#include <gtest/gtest.h>
#include "../CommonLib/Player.h"

#pragma region PositiveTests

// default konstruktor ispravno postavlja polja
TEST(PlayerTest, DefaultConstructedIsValid) {
    Player player;
    EXPECT_EQ(player.getIdStruct(), 5);
    EXPECT_EQ(player.getIdGame(), 0);
    EXPECT_EQ(player.getAcceptedSocket(), (SOCKET)NULL);
    EXPECT_EQ(player.getIdPlayer(), 0);
    EXPECT_STREQ(player.getUsername(), "");
}

// parametarski konstruktor postavlja sve
TEST(PlayerTest, ParamConstructorSetsEverything) {
    Player player("TestUser", (SOCKET)1234, 2, 77, 3);
    EXPECT_EQ(player.getIdStruct(), 5);
    EXPECT_EQ(player.getAcceptedSocket(), (SOCKET)1234);
    EXPECT_EQ(player.getIdGame(), 2);
    EXPECT_EQ(player.getIdPlayer(), 77);
    EXPECT_STREQ(player.getUsername(), "TestUser");
}

// setUsername pravilno postavlja i menja korisnicko ime
TEST(PlayerTest, UsernameSetterAndGetterWorks) {
    Player player;
    player.setUsername("Korisnik123");
    EXPECT_STREQ(player.getUsername(), "Korisnik123");
}

// Da li serialize()/deserialize() prenosi sve podatke
TEST(PlayerTest, SerializationRoundtrip) {
    Player original("Ana", (SOCKET)999, 15, 44, 2);
    char buffer[sizeof(Player)]{};
    original.serialize(buffer);

    Player copy;
    copy.deserialize(buffer);

    EXPECT_EQ(copy.getIdStruct(), 5);
    EXPECT_STREQ(copy.getUsername(), "Ana");
    EXPECT_EQ(copy.getAcceptedSocket(), (SOCKET)999);
    EXPECT_EQ(copy.getIdGame(), 15);
    EXPECT_EQ(copy.getIdPlayer(), 44);
}

#pragma endregion

#pragma region NegativeTests

// setUsername sa nullptr daje prazan string
TEST(PlayerTest, UsernameWithNullptrIsEmptyString) {
    Player player("Bob", (SOCKET)1, 2, 3, 4);
    player.setUsername(nullptr);
    EXPECT_STREQ(player.getUsername(), "");
}

// deserialize iz korumpiranog buffera (username pokvaren)
TEST(PlayerTest, DeserializeFromCorruptedBuffer) {
    Player p("ValidUser", (SOCKET)11, 2, 3, 4);
    char buf[sizeof(Player)]{};
    p.serialize(buf);

    // Korumpiramo korisničko ime (prvi bajt username sekcije)
    buf[5] ^= 0xFF;

    Player corrupted;
    corrupted.deserialize(buf);

    ASSERT_STRNE(corrupted.getUsername(), "ValidUser");
}

#pragma endregion