#pragma once

#include "../game/player_manager.h"
#include "../game/game_manager.h"

#include <boost/asio.hpp>
#include <boost/beast.hpp>

namespace ip = boost::asio::ip;

class Server {
public:
    Server(size_t threadCount, size_t port);
    void start();
    void stop();

private:
    void onAcceptAsync();

    boost::asio::thread_pool pool_;
    boost::asio::io_context ioc_;
    ip::tcp::acceptor acceptor_;

    std::shared_ptr<PlayerManager> playerManager_;
    std::shared_ptr<GameManager> gameManager_;

    size_t threadCount_;
    size_t port_;
};
