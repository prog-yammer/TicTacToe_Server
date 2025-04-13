#define BOOST_TEST_MODULE WsTest
#include <boost/test/included/unit_test.hpp>
#include <boost/format.hpp>

#include "../src/web/server.h"
#include "../src/web/command_code.h"

using tcp = boost::asio::ip::tcp;
namespace websocket = boost::beast::websocket;

struct Message {
    OutCommandCode code;
    std::string message;
};

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
        auto message = receiveInputMessage();
        return {
            .code = message.first,
            .message = message.second,
        };
    }

    ErrorCode receiveError()
    {
        auto message = receiveInputMessage();
        if (message.first != OutCommandCode::ERROR)
            throw std::exception();
        return static_cast<ErrorCode>(std::stoi(message.second));
    }

private:
    std::pair<OutCommandCode, std::string> receiveInputMessage()
    {
        boost::beast::flat_buffer buffer;
        ws_.read(buffer);
        auto data = boost::beast::buffers_to_string(buffer.data());
        size_t pos = data.find(' ');
        if (pos != std::string::npos)
            return {static_cast<OutCommandCode>(std::stoi(data.substr(0, pos))), data.substr(pos + 1)};

        return {static_cast<OutCommandCode>(std::stoi(data)), ""};
    }

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
        clientId1 = client1.receiveMessage().message;
        client2.connect();
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
};

BOOST_GLOBAL_FIXTURE(WsTestGlobalFixture);

BOOST_FIXTURE_TEST_CASE(ConnectionTest, WsTestFixture)
{
    client1.connect();
    auto message = client1.receiveMessage();

    BOOST_CHECK_EQUAL(message.code, OutCommandCode::PLAYER_CREATED);
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
    BOOST_CHECK_EQUAL(message.message, clientId2);
}

BOOST_FIXTURE_TEST_CASE(IncorrectCommandTest, WsTestFixture) // TODO
{
    connectClients();

    client1.sendMessage("what's what?");
    auto errorCode = client1.receiveError();
    BOOST_CHECK_EQUAL(errorCode, ErrorCode::INCORRECT_FORMAT);

    client1.sendMessage(InCommandCode::JOIN_GAME);
    errorCode = client1.receiveError();
    BOOST_CHECK_EQUAL(errorCode, ErrorCode::INCORRECT_FORMAT);

    client1.sendMessage(InCommandCode::JOIN_GAME, "gameId");
    errorCode = client1.receiveError();
    BOOST_CHECK_EQUAL(errorCode, ErrorCode::INCORRECT_FORMAT);

    createGame();

    client1.sendMessage(InCommandCode::MOVE);
    errorCode = client1.receiveError();
    BOOST_CHECK_EQUAL(errorCode, ErrorCode::INCORRECT_FORMAT);

    client1.sendMessage(InCommandCode::MOVE, "0");
    errorCode = client1.receiveError();
    BOOST_CHECK_EQUAL(errorCode, ErrorCode::INCORRECT_FORMAT);

    client1.sendMessage(InCommandCode::MOVE, "0 zero");
    errorCode = client1.receiveError();
    BOOST_CHECK_EQUAL(errorCode, ErrorCode::INCORRECT_FORMAT);
}

BOOST_FIXTURE_TEST_CASE(InGameCommandOutsideGameErrorsTest, WsTestFixture)
{
    connectClients();

    client1.sendMessage(InCommandCode::LEAVE_GAME);
    auto errorCode = client1.receiveError();
    BOOST_CHECK_EQUAL(errorCode, ErrorCode::ERROR_LEAVE);

    client1.sendMessage(InCommandCode::MOVE, "0 0");
    errorCode = client1.receiveError();
    BOOST_CHECK_EQUAL(errorCode, ErrorCode::ERROR_MOVE);
}

BOOST_FIXTURE_TEST_CASE(OutGameCommandInsideGameErrorsTest, WsTestFixture)
{
    connectClients();
    auto gameId = createGame();

    client1.sendMessage(InCommandCode::CREATE_GAME);
    auto errorCode = client1.receiveError();
    BOOST_CHECK_EQUAL(errorCode, ErrorCode::ERROR_CREATE);

    client1.sendMessage(InCommandCode::JOIN_GAME, gameId);
    errorCode = client1.receiveError();
    BOOST_CHECK_EQUAL(errorCode, ErrorCode::ERROR_JOIN);
}

BOOST_FIXTURE_TEST_CASE(LeaveGameTest, WsTestFixture)
{
    connectClients();
    auto gameId = createGame();

    client1.sendMessage(InCommandCode::LEAVE_GAME);
    auto message = client1.receiveMessage();
    BOOST_CHECK_EQUAL(message.code, OutCommandCode::GAME_ENDED);
    BOOST_CHECK_EQUAL(message.message, clientId2);

    message = client1.receiveMessage();
    BOOST_CHECK_EQUAL(message.code, OutCommandCode::LEFT_GAME);

    message = client2.receiveMessage();
    BOOST_CHECK_EQUAL(message.code, OutCommandCode::OPPONENT_LEFT);
    BOOST_CHECK_EQUAL(message.message, clientId1);

    message = client2.receiveMessage();
    BOOST_CHECK_EQUAL(message.code, OutCommandCode::GAME_ENDED);
    BOOST_CHECK_EQUAL(message.message, clientId2);
}

BOOST_FIXTURE_TEST_CASE(DisconnectFromGameTest, WsTestFixture)
{
    connectClients();
    createGame();

    client1.disconnect();

    auto message = client2.receiveMessage();
    BOOST_CHECK_EQUAL(message.code, OutCommandCode::OPPONENT_LEFT);
    BOOST_CHECK_EQUAL(message.message, clientId1);

    message = client2.receiveMessage();
    BOOST_CHECK_EQUAL(message.code, OutCommandCode::GAME_ENDED);
    BOOST_CHECK_EQUAL(message.message, clientId2);
}

BOOST_FIXTURE_TEST_CASE(MakeMoveTest, WsTestFixture)
{
    connectClients();
    createGame();

    client1.sendMessage(InCommandCode::MOVE, "0 0");
    auto message = client1.receiveMessage();
    BOOST_CHECK_EQUAL(message.code, OutCommandCode::MOVED);

    message = client2.receiveMessage();
    BOOST_CHECK_EQUAL(message.code, OutCommandCode::OPPONENT_MOVED);
    BOOST_CHECK_EQUAL(message.message, "0 0 X " + clientId1);

    client2.sendMessage(InCommandCode::MOVE, "0 1");
    message = client2.receiveMessage();
    BOOST_CHECK_EQUAL(message.code, OutCommandCode::MOVED);

    message = client1.receiveMessage();
    BOOST_CHECK_EQUAL(message.code, OutCommandCode::OPPONENT_MOVED);
    BOOST_CHECK_EQUAL(message.message, "0 1 O " + clientId2);
}

BOOST_FIXTURE_TEST_CASE(TurnMoveTest, WsTestFixture)
{
    connectClients();
    createGame();

    client2.sendMessage(InCommandCode::MOVE, "0 0");
    auto errorCode = client2.receiveError();
    BOOST_CHECK_EQUAL(errorCode, ErrorCode::ERROR_MOVE);

    client1.sendMessage(InCommandCode::MOVE, "0 0");
    client1.receiveMessage();

    client1.sendMessage(InCommandCode::MOVE, "0 1");
    errorCode = client1.receiveError();
    BOOST_CHECK_EQUAL(errorCode, ErrorCode::ERROR_MOVE);

    auto message = client2.receiveMessage();
    BOOST_CHECK_EQUAL(message.code, OutCommandCode::OPPONENT_MOVED);
    BOOST_CHECK_EQUAL(message.message, "0 0 X " + clientId1);
}

BOOST_FIXTURE_TEST_CASE(IncorrectMoveTest, WsTestFixture)
{
    connectClients();
    createGame();

    client1.sendMessage(InCommandCode::MOVE, "-1 0");
    auto errorCode = client1.receiveError();
    BOOST_CHECK_EQUAL(errorCode, ErrorCode::ERROR_MOVE);

    client1.sendMessage(InCommandCode::MOVE, "0 -1");
    errorCode = client1.receiveError();
    BOOST_CHECK_EQUAL(errorCode, ErrorCode::ERROR_MOVE);

    client1.sendMessage(InCommandCode::MOVE, "4 0");
    errorCode = client1.receiveError();
    BOOST_CHECK_EQUAL(errorCode, ErrorCode::ERROR_MOVE);

    client1.sendMessage(InCommandCode::MOVE, "0 4");
    errorCode = client1.receiveError();
    BOOST_CHECK_EQUAL(errorCode, ErrorCode::ERROR_MOVE);

    client1.sendMessage(InCommandCode::MOVE, "0 0");
    client1.receiveMessage();
    client2.receiveMessage();

    client2.sendMessage(InCommandCode::MOVE, "0 0");
    errorCode = client2.receiveError();
    BOOST_CHECK_EQUAL(errorCode, ErrorCode::ERROR_MOVE);
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
    client2.receiveMessage();

    auto message = client1.receiveMessage();
    BOOST_CHECK_EQUAL(message.code, OutCommandCode::GAME_ENDED);
    BOOST_CHECK_EQUAL(message.message, clientId1);

    message = client2.receiveMessage();
    BOOST_CHECK_EQUAL(message.code, OutCommandCode::GAME_ENDED);
    BOOST_CHECK_EQUAL(message.message, clientId1);
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
    client2.receiveMessage();

    auto message = client1.receiveMessage();
    BOOST_CHECK_EQUAL(message.code, OutCommandCode::GAME_ENDED);
    BOOST_CHECK_EQUAL(message.message, "draw");

    message = client2.receiveMessage();
    BOOST_CHECK_EQUAL(message.code, OutCommandCode::GAME_ENDED);
    BOOST_CHECK_EQUAL(message.message, "draw");
}
