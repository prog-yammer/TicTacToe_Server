#define BOOST_TEST_MODULE WsTest
#include <boost/test/included/unit_test.hpp>
#include <boost/format.hpp>

#include "../src/web/server.h"
#include "../src/web/common/command_code.h"
#include "../src/web/common/tools.h"

using tcp = boost::asio::ip::tcp;
namespace websocket = boost::beast::websocket;

struct TestClient {
    explicit TestClient(boost::asio::io_context& ioc)
        : resolver_(ioc), ws_(ioc)
    {}

    void connect()
    {
        auto const results = resolver_.resolve("localhost", "8080");
        boost::asio::connect(ws_.next_layer(), results);
        ws_.handshake("localhost", "/");
    }

    void disconnect()
    {
        ws_.close(websocket::close_code::normal);
    }

    void sendMessage(InCommandCode commandCode, const std::string& message = "")
    {
        ws_.write(boost::asio::buffer((boost::format("%d%s") % commandCode % ((message.empty() ? "" : " ") + message)).str()));
    }

    void sendMessage(const std::string& message)
    {
        ws_.write(boost::asio::buffer(message));
    }

    Message receiveMessage()
    {
        boost::beast::flat_buffer buffer;
        ws_.read(buffer);
        auto data = boost::beast::buffers_to_string(buffer.data());
        
        return getInMessage(data);
    }

private:
    tcp::resolver resolver_;
    websocket::stream<tcp::socket> ws_;
};

struct WsTestGlobalFixture {
    WsTestGlobalFixture()
        : server(2, 8080)
    {
        server_thread = std::thread([this]() {
            server.start();
        });
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    ~WsTestGlobalFixture()
    {
        server.stop();
        server_thread.join();
    }

    Server server;
    std::thread server_thread;
};

struct WsTestFixture {
    WsTestFixture()
        : client1(ioc)
        , client2(ioc)
    {}

    void connectClients()
    {
        client1.connect();
        client1.sendMessage(InCommandCode::AUTH, nickname1);
        clientId1 = client1.receiveMessage().message;
        client2.connect();
        client2.sendMessage(InCommandCode::AUTH, nickname2);
        clientId2 = client2.receiveMessage().message;
    }

    std::string createGame()
    {
        client1.sendMessage(InCommandCode::CREATE_GAME);
        auto gameId = client1.receiveMessage().message;
        client2.sendMessage(InCommandCode::JOIN_GAME, gameId);
        client2.receiveMessage();
        client1.receiveMessage();

        return gameId;
    }

    boost::asio::io_context ioc;
    TestClient client1;
    TestClient client2;

    std::string clientId1;
    std::string clientId2;

    std::string nickname1 = "p1";
    std::string nickname2 = "p2";
};

BOOST_GLOBAL_FIXTURE(WsTestGlobalFixture);

BOOST_FIXTURE_TEST_CASE(AuthTest, WsTestFixture)
{
    client1.connect();
    client1.sendMessage(InCommandCode::AUTH, "player");
    auto message = client1.receiveMessage();
    BOOST_CHECK_EQUAL(message.code, OutCommandCode::PLAYER_AUTHED);
}

BOOST_FIXTURE_TEST_CASE(JoinGameTest, WsTestFixture)
{
    connectClients();

    client1.sendMessage(InCommandCode::CREATE_GAME);
    auto message = client1.receiveMessage();
    BOOST_CHECK_EQUAL(message.code, OutCommandCode::GAME_CREATED);

    auto gameId = message.message;
    client2.sendMessage(InCommandCode::JOIN_GAME, gameId);

    message = client2.receiveMessage();
    BOOST_CHECK_EQUAL(message.code, OutCommandCode::JOINED_GAME);

    message = client1.receiveMessage();
    BOOST_CHECK_EQUAL(message.code, OutCommandCode::OPPONENT_JOINED);
    BOOST_CHECK_EQUAL(message.message, nickname2);
}

BOOST_FIXTURE_TEST_CASE(IncorrectCommandTest, WsTestFixture)
{
    connectClients();

    client1.sendMessage("what's what?");
    auto message = client1.receiveMessage();
    BOOST_CHECK_EQUAL(*message.errorCode, ErrorCode::INCORRECT_FORMAT);

    client1.sendMessage(InCommandCode::JOIN_GAME);
    message = client1.receiveMessage();
    BOOST_CHECK_EQUAL(*message.errorCode, ErrorCode::INCORRECT_FORMAT);

    client1.sendMessage(InCommandCode::JOIN_GAME, "gameId");
    message = client1.receiveMessage();
    BOOST_CHECK_EQUAL(*message.errorCode, ErrorCode::INCORRECT_FORMAT);

    createGame();

    client1.sendMessage(InCommandCode::MOVE);
    message = client1.receiveMessage();
    BOOST_CHECK_EQUAL(*message.errorCode, ErrorCode::INCORRECT_FORMAT);

    client1.sendMessage(InCommandCode::MOVE, "0");
    message = client1.receiveMessage();
    BOOST_CHECK_EQUAL(*message.errorCode, ErrorCode::INCORRECT_FORMAT);

    client1.sendMessage(InCommandCode::MOVE, "0 zero");
    message = client1.receiveMessage();
    BOOST_CHECK_EQUAL(*message.errorCode, ErrorCode::INCORRECT_FORMAT);
}

BOOST_FIXTURE_TEST_CASE(InGameCommandOutsideGameErrorsTest, WsTestFixture)
{
    connectClients();

    client1.sendMessage(InCommandCode::LEAVE_GAME);
    auto message = client1.receiveMessage();
    BOOST_CHECK_EQUAL(*message.errorCode, ErrorCode::ERROR_LEAVE);

    client1.sendMessage(InCommandCode::MOVE, "0 0");
    message = client1.receiveMessage();
    BOOST_CHECK_EQUAL(*message.errorCode, ErrorCode::ERROR_MOVE);
}

BOOST_FIXTURE_TEST_CASE(OutGameCommandInsideGameErrorsTest, WsTestFixture)
{
    connectClients();
    auto gameId = createGame();

    client1.sendMessage(InCommandCode::CREATE_GAME);
    auto message = client1.receiveMessage();
    BOOST_CHECK_EQUAL(*message.errorCode, ErrorCode::ERROR_CREATE);

    client1.sendMessage(InCommandCode::JOIN_GAME, gameId);
    message = client1.receiveMessage();
    BOOST_CHECK_EQUAL(*message.errorCode, ErrorCode::ERROR_JOIN);
}

BOOST_FIXTURE_TEST_CASE(LeaveGameTest, WsTestFixture)
{
    connectClients();
    auto gameId = createGame();

    client1.sendMessage(InCommandCode::LEAVE_GAME);
    auto message = client1.receiveMessage();
    BOOST_CHECK_EQUAL(message.code, OutCommandCode::LEFT_GAME);

    message = client2.receiveMessage();
    BOOST_CHECK_EQUAL(message.code, OutCommandCode::GAME_ENDED);
    BOOST_CHECK_EQUAL(std::stoi(message.message), GameEndedCode::OPPONENT_LEFT);
}

BOOST_FIXTURE_TEST_CASE(DisconnectFromGameTest, WsTestFixture)
{
    connectClients();
    createGame();

    client1.disconnect();

    auto message = client2.receiveMessage();
    BOOST_CHECK_EQUAL(message.code, OutCommandCode::GAME_ENDED); // TODO
    BOOST_CHECK_EQUAL(std::stoi(message.message), GameEndedCode::OPPONENT_LEFT);
}

BOOST_FIXTURE_TEST_CASE(MakeMoveTest, WsTestFixture)
{
    connectClients();
    createGame();

    client1.sendMessage(InCommandCode::MOVE, "0 0");
    auto message = client1.receiveMessage();
    BOOST_CHECK_EQUAL(message.code, OutCommandCode::MOVED);

    message = client2.receiveMessage();
    BOOST_CHECK_EQUAL(message.code, OutCommandCode::MOVED);
    BOOST_CHECK_EQUAL(message.message, "0 0 X " + nickname1);

    client2.sendMessage(InCommandCode::MOVE, "0 1");
    message = client2.receiveMessage();
    BOOST_CHECK_EQUAL(message.code, OutCommandCode::MOVED);

    message = client1.receiveMessage();
    BOOST_CHECK_EQUAL(message.code, OutCommandCode::MOVED);
    BOOST_CHECK_EQUAL(message.message, "0 1 O " + nickname2);
}

BOOST_FIXTURE_TEST_CASE(TurnMoveTest, WsTestFixture)
{
    connectClients();
    createGame();

    client2.sendMessage(InCommandCode::MOVE, "0 0");
    auto message = client2.receiveMessage();
    BOOST_CHECK_EQUAL(*message.errorCode, ErrorCode::ERROR_MOVE);

    client1.sendMessage(InCommandCode::MOVE, "0 0");
    client1.receiveMessage();

    client1.sendMessage(InCommandCode::MOVE, "0 1");
    message = client1.receiveMessage();
    BOOST_CHECK_EQUAL(*message.errorCode, ErrorCode::ERROR_MOVE);

    message = client2.receiveMessage();
    BOOST_CHECK_EQUAL(message.code, OutCommandCode::MOVED);
    BOOST_CHECK_EQUAL(message.message, "0 0 X " + nickname1);
}

BOOST_FIXTURE_TEST_CASE(IncorrectMoveTest, WsTestFixture)
{
    connectClients();
    createGame();

    client1.sendMessage(InCommandCode::MOVE, "-1 0");
    auto message = client1.receiveMessage();
    BOOST_CHECK_EQUAL(*message.errorCode, ErrorCode::ERROR_MOVE);

    client1.sendMessage(InCommandCode::MOVE, "0 -1");
    message = client1.receiveMessage();
    BOOST_CHECK_EQUAL(*message.errorCode, ErrorCode::ERROR_MOVE);

    client1.sendMessage(InCommandCode::MOVE, "4 0");
    message = client1.receiveMessage();
    BOOST_CHECK_EQUAL(*message.errorCode, ErrorCode::ERROR_MOVE);

    client1.sendMessage(InCommandCode::MOVE, "0 4");
    message = client1.receiveMessage();
    BOOST_CHECK_EQUAL(*message.errorCode, ErrorCode::ERROR_MOVE);

    client1.sendMessage(InCommandCode::MOVE, "0 0");
    client1.receiveMessage();
    client2.receiveMessage();

    client2.sendMessage(InCommandCode::MOVE, "0 0");
    message = client2.receiveMessage();
    BOOST_CHECK_EQUAL(*message.errorCode, ErrorCode::ERROR_MOVE);
}

BOOST_FIXTURE_TEST_CASE(WinGameTest, WsTestFixture)
{
    connectClients();
    createGame();

    client1.sendMessage(InCommandCode::MOVE, "0 0");
    client1.receiveMessage();
    client2.receiveMessage();

    client2.sendMessage(InCommandCode::MOVE, "1 0");
    client1.receiveMessage();
    client2.receiveMessage();

    client1.sendMessage(InCommandCode::MOVE, "0 1");
    client1.receiveMessage();
    client2.receiveMessage();

    client2.sendMessage(InCommandCode::MOVE, "1 1");
    client1.receiveMessage();
    client2.receiveMessage();

    client1.sendMessage(InCommandCode::MOVE, "0 2");
    client1.receiveMessage();
    client2.receiveMessage();

    auto message = client1.receiveMessage();
    BOOST_CHECK_EQUAL(message.code, OutCommandCode::GAME_ENDED);
    BOOST_CHECK_EQUAL(message.message, std::to_string(GameEndedCode::WIN) + ' ' + nickname1);

    message = client2.receiveMessage();
    BOOST_CHECK_EQUAL(message.code, OutCommandCode::GAME_ENDED);
    BOOST_CHECK_EQUAL(message.message, std::to_string(GameEndedCode::WIN) + ' ' + nickname1);
}

BOOST_FIXTURE_TEST_CASE(DrawGameTest, WsTestFixture)
{
    connectClients();
    createGame();

    client1.sendMessage(InCommandCode::MOVE, "0 0");
    client1.receiveMessage();
    client2.receiveMessage();

    client2.sendMessage(InCommandCode::MOVE, "0 1");
    client1.receiveMessage();
    client2.receiveMessage();

    client1.sendMessage(InCommandCode::MOVE, "0 2");
    client1.receiveMessage();
    client2.receiveMessage();

    client2.sendMessage(InCommandCode::MOVE, "1 1");
    client1.receiveMessage();
    client2.receiveMessage();

    client1.sendMessage(InCommandCode::MOVE, "1 0");
    client1.receiveMessage();
    client2.receiveMessage();

    client2.sendMessage(InCommandCode::MOVE, "1 2");
    client1.receiveMessage();
    client2.receiveMessage();

    client1.sendMessage(InCommandCode::MOVE, "2 1");
    client1.receiveMessage();
    client2.receiveMessage();

    client2.sendMessage(InCommandCode::MOVE, "2 0");
    client1.receiveMessage();
    client2.receiveMessage();

    client1.sendMessage(InCommandCode::MOVE, "2 2");
    client1.receiveMessage();
    client2.receiveMessage();

    auto message = client1.receiveMessage();
    BOOST_CHECK_EQUAL(message.code, OutCommandCode::GAME_ENDED);
    BOOST_CHECK_EQUAL(std::stoi(message.message), GameEndedCode::DRAW);

    message = client2.receiveMessage();
    BOOST_CHECK_EQUAL(message.code, OutCommandCode::GAME_ENDED);
    BOOST_CHECK_EQUAL(std::stoi(message.message), GameEndedCode::DRAW);
}
