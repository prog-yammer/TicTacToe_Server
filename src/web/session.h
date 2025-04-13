#pragma once

#include "../game/player.h"
#include "../game/game_manager.h"
#include "../game/player_manager.h"

#include <boost/beast.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/uuid/string_generator.hpp>

#include <iostream>
#include <deque>
#include <memory>

namespace ws = boost::beast::websocket;

class Session : public std::enable_shared_from_this<Session> {
public:
    explicit Session(
            std::shared_ptr<ws::stream<boost::beast::tcp_stream>> ws,
            std::shared_ptr<PlayerManager> player,
            std::shared_ptr<GameManager> gameManager);
    ~Session();

    void start();

private:
    void onReadAsync();
    void onWriteAsync();
    void writeAsync(const std::string& message);

    static std::string processCommand(const std::string& command, std::shared_ptr<Session> session);
    static std::string processNotification(const Notification& notification, std::shared_ptr<Session> session);

    std::shared_ptr<ws::stream<boost::beast::tcp_stream>> ws_;
    boost::beast::flat_buffer buf_;

    std::mutex writeMessagesMutex_;
    std::deque<std::string> writeMessages_;

    std::shared_ptr<Player> player_;
    std::shared_ptr<PlayerManager> playerManager_;
    std::shared_ptr<GameManager> gameManager_;

    boost::uuids::string_generator uuidStrGen_;
};
