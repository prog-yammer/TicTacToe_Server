#define BOOST_TEST_MODULE GameTest
#include <boost/test/included/unit_test.hpp>

#include "../src/game/player_manager.h"
#include "../src/game/game_manager.h"

struct GameTestFixture {
    GameTestFixture()
    {
        player1 = playerManager.createPlayer();
        player2 = playerManager.createPlayer();

        player1->setNotificationHandler([this](const Notification& notification) {
            notifications1.push_back(notification);
        });
        player2->setNotificationHandler([this](const Notification& notification) {
            notifications2.push_back(notification);
        });
    }

    ~GameTestFixture()
    {
        notifications1.clear();
        notifications2.clear();
    }

    std::shared_ptr<Player> player1;
    std::shared_ptr<Player> player2;

    std::vector<Notification> notifications1;
    std::vector<Notification> notifications2;

    PlayerManager playerManager;
    GameManager gameManager;
};

BOOST_FIXTURE_TEST_CASE(JoinTest, GameTestFixture)
{
    auto gameId = gameManager.createGame();
    bool status = gameManager.addPlayerToGame(player1, gameId);
    BOOST_TEST(status);
    BOOST_TEST(notifications1.empty());

    status = gameManager.addPlayerToGame(player2, gameId);
    BOOST_TEST(status);

    BOOST_TEST_CHECK(notifications1.size(), 1);
    BOOST_CHECK(notifications1[0].type == Notification::Type::PlayerJoined);
    BOOST_CHECK(notifications1[0].playerId == player2->id());

    BOOST_TEST(notifications2.empty());

    BOOST_CHECK(player1->isInGame());
    BOOST_CHECK(player2->isInGame());
}

BOOST_FIXTURE_TEST_CASE(JoinWhileInGameTest, GameTestFixture)
{
    auto gameId1 = gameManager.createGame();
    bool status = gameManager.addPlayerToGame(player1, gameId1);
    BOOST_TEST(status);

    BOOST_CHECK(player1->isInGame());

    status = gameManager.addPlayerToGame(player1, gameId1);
    BOOST_TEST(!status);

    auto gameId2 = gameManager.createGame();
    status = gameManager.addPlayerToGame(player1, gameId2);
    BOOST_TEST(!status);
}

BOOST_FIXTURE_TEST_CASE(LeaveTest, GameTestFixture)
{
    auto gameId = gameManager.createGame();
    bool status = gameManager.addPlayerToGame(player1, gameId);
    BOOST_TEST(status);

    status = gameManager.addPlayerToGame(player2, gameId);
    BOOST_TEST(status);
    notifications1.clear(); // remove PlayerLeft notification

    status = gameManager.leavePlayerFromGame(player2);
    BOOST_TEST(status);

    BOOST_CHECK(!player1->isInGame());
    BOOST_CHECK(!player2->isInGame());

    BOOST_TEST_CHECK(notifications1.size(), 2);
    BOOST_CHECK(notifications1[0].type == Notification::Type::PlayerLeft);
    BOOST_CHECK(notifications1[0].playerId == player2->id());

    BOOST_CHECK(notifications1[1].type == Notification::Type::GameEnded);
    BOOST_CHECK(notifications1[1].playerId == player1->id());

    BOOST_TEST_CHECK(notifications1.size(), 1);
    BOOST_CHECK(notifications2[0].type == Notification::Type::GameEnded);
    BOOST_CHECK(notifications2[0].playerId == player1->id());
}

BOOST_FIXTURE_TEST_CASE(JoinAfterLeaveTest, GameTestFixture)
{
    auto gameId1 = gameManager.createGame();
    bool status = gameManager.addPlayerToGame(player1, gameId1);
    BOOST_TEST(status);

    status = gameManager.addPlayerToGame(player2, gameId1);
    BOOST_TEST(status);

    status = gameManager.leavePlayerFromGame(player1);
    BOOST_TEST(status);

    auto gameId2 = gameManager.createGame();
    status = gameManager.addPlayerToGame(player1, gameId2);
    BOOST_TEST(status);

    status = gameManager.addPlayerToGame(player2, gameId2);
    BOOST_TEST(status);
}

BOOST_FIXTURE_TEST_CASE(LeaveWhileNotInGameTest, GameTestFixture)
{
    BOOST_CHECK(!player1->isInGame());

    bool status = gameManager.leavePlayerFromGame(player1);
    BOOST_TEST(!status);
}

BOOST_FIXTURE_TEST_CASE(MakeMoveTest, GameTestFixture)
{
    auto gameId = gameManager.createGame();
    bool status = gameManager.addPlayerToGame(player1, gameId);
    BOOST_TEST(status);

    status = gameManager.addPlayerToGame(player2, gameId);
    BOOST_TEST(status);
    notifications1.clear(); // remove PlayerLeft notification

    status = gameManager.makeMove(player1, 0, 0);
    BOOST_TEST(status);
    BOOST_TEST_CHECK(notifications2.size(), 1);
    BOOST_CHECK(notifications2[0].type == Notification::Type::PlayerMoved);
    BOOST_CHECK(notifications2[0].playerId == player1->id());
    BOOST_CHECK(notifications2[0].extraInfo == "0 0 X");

    status = gameManager.makeMove(player2, 0, 1);
    BOOST_TEST(status);
    BOOST_TEST_CHECK(notifications1.size(), 1);
    BOOST_CHECK(notifications1[0].type == Notification::Type::PlayerMoved);
    BOOST_CHECK(notifications1[0].playerId == player2->id());
    BOOST_CHECK(notifications1[0].extraInfo == "0 1 O");

    status = gameManager.makeMove(player1, 1, 0);
    BOOST_TEST(status);
    BOOST_TEST(status);
    BOOST_TEST_CHECK(notifications2.size(), 2);
    BOOST_CHECK(notifications2[1].type == Notification::Type::PlayerMoved);
    BOOST_CHECK(notifications2[1].playerId == player1->id());
    BOOST_CHECK(notifications2[1].extraInfo == "1 0 X");

    status = gameManager.makeMove(player2, 1, 1);
    BOOST_TEST(status);
    BOOST_TEST_CHECK(notifications1.size(), 2);
    BOOST_CHECK(notifications1[1].type == Notification::Type::PlayerMoved);
    BOOST_CHECK(notifications1[1].playerId == player2->id());
    BOOST_CHECK(notifications1[1].extraInfo == "1 1 O");
}

BOOST_FIXTURE_TEST_CASE(NotTurnMakeMoveTest, GameTestFixture)
{
    auto gameId = gameManager.createGame();
    bool status = gameManager.addPlayerToGame(player1, gameId);
    BOOST_TEST(status);

    status = gameManager.addPlayerToGame(player2, gameId);
    BOOST_TEST(status);

    status = gameManager.makeMove(player1, 0, 0);
    BOOST_TEST(status);

    status = gameManager.makeMove(player1, 0, 1);
    BOOST_TEST(!status);

    status = gameManager.makeMove(player2, 0, 1);
    BOOST_TEST(status);

    status = gameManager.makeMove(player2, 0, 2);
    BOOST_TEST(!status);
}

BOOST_FIXTURE_TEST_CASE(IncorrectMovesTest, GameTestFixture)
{
    auto gameId = gameManager.createGame();
    bool status = gameManager.addPlayerToGame(player1, gameId);
    BOOST_TEST(status);

    status = gameManager.addPlayerToGame(player2, gameId);
    BOOST_TEST(status);

    status = gameManager.makeMove(player1, -1, 0);
    BOOST_TEST(!status);

    status = gameManager.makeMove(player1, 0, -1);
    BOOST_TEST(!status);

    status = gameManager.makeMove(player1, 4, 0);
    BOOST_TEST(!status);

    status = gameManager.makeMove(player1, 0, 4);
    BOOST_TEST(!status);

    status = gameManager.makeMove(player1, 0, 0);
    BOOST_TEST(status);

    status = gameManager.makeMove(player2, 0, 0);
    BOOST_TEST(!status);

    status = gameManager.makeMove(player2, 0, 1);
    BOOST_TEST(status);
}

BOOST_FIXTURE_TEST_CASE(WinGameTest, GameTestFixture)
{
    auto gameId = gameManager.createGame();
    bool status = gameManager.addPlayerToGame(player1, gameId);
    BOOST_TEST(status);

    status = gameManager.addPlayerToGame(player2, gameId);
    BOOST_TEST(status);
    notifications1.clear(); // remove PlayerLeft notification

    status = gameManager.makeMove(player1, 0, 0);
    BOOST_TEST(status);
    status = gameManager.makeMove(player2, 1, 0);
    BOOST_TEST(status);
    status = gameManager.makeMove(player1, 0, 1);
    BOOST_TEST(status);
    status = gameManager.makeMove(player2, 1, 1);
    BOOST_TEST(status);
    status = gameManager.makeMove(player1, 0, 2);
    BOOST_TEST(status);

    BOOST_TEST(notifications1.size(), 3); // 2 opponent's moves, 1 GameEnded notification
    BOOST_CHECK(notifications1.back().type == Notification::Type::GameEnded);
    BOOST_CHECK(notifications1.back().playerId == player1->id());

    BOOST_TEST(notifications2.size(), 4); // 3 opponent's moves, 1 GameEnded notification
    BOOST_CHECK(notifications2.back().type == Notification::Type::GameEnded);
    BOOST_CHECK(notifications2.back().playerId == player1->id());
}

BOOST_FIXTURE_TEST_CASE(DrawGameTest, GameTestFixture)
{
    auto gameId = gameManager.createGame();
    bool status = gameManager.addPlayerToGame(player1, gameId);
    BOOST_TEST(status);

    status = gameManager.addPlayerToGame(player2, gameId);
    BOOST_TEST(status);
    notifications1.clear(); // remove PlayerLeft notification

    status = gameManager.makeMove(player1, 0, 0);
    BOOST_TEST(status);
    status = gameManager.makeMove(player2, 0, 1);
    BOOST_TEST(status);
    status = gameManager.makeMove(player1, 0, 2);
    BOOST_TEST(status);
    status = gameManager.makeMove(player2, 1, 1);
    BOOST_TEST(status);
    status = gameManager.makeMove(player1, 1, 0);
    BOOST_TEST(status);
    status = gameManager.makeMove(player2, 1, 2);
    BOOST_TEST(status);
    status = gameManager.makeMove(player1, 2, 1);
    BOOST_TEST(status);
    status = gameManager.makeMove(player2, 2, 0);
    BOOST_TEST(status);
    status = gameManager.makeMove(player1, 2, 2);
    BOOST_TEST(status);

    BOOST_TEST(notifications1.size(), 5); // 4 opponent's moves, 1 GameEnded notification
    BOOST_CHECK(notifications1.back().type == Notification::Type::GameEnded);
    BOOST_CHECK(!notifications1.back().playerId.has_value());

    BOOST_TEST(notifications2.size(), 6); // 5 opponent's moves, 1 GameEnded notification
    BOOST_CHECK(notifications2.back().type == Notification::Type::GameEnded);
    BOOST_CHECK(!notifications2.back().playerId.has_value());
}
