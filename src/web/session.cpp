#include "session.h"
#include "common/command_code.h"

#include <boost/algorithm/string.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <boost/format.hpp>

#include <thread>
#include <iostream>
#include <utility>

Session::Session(std::shared_ptr<ws::stream<boost::beast::tcp_stream>> ws,
                 std::shared_ptr<PlayerManager> playerManager,
                 std::shared_ptr<GameManager> gameManager)
    : ws_(std::move(ws))
    , playerManager_(playerManager)
    , gameManager_(gameManager)
{}

Session::~Session()
{
    if (player_)
        gameManager_->leavePlayerFromGame(player_);
}

void Session::start()
{
    ws_->async_accept([self = shared_from_this(), ws = ws_](const boost::beast::error_code& ec) {
        if (ec) {
            if (ec == boost::asio::error::operation_aborted) {
                return;
            }
            std::cerr << ec.message() << std::endl;
            return;
        }

        self->onReadAsync();
    });
}

void Session::onReadAsync()
{
    ws_->async_read(buf_, [self = shared_from_this()] (boost::beast::error_code ec, std::size_t)
        {
            if (ec) {
                if (ec == boost::asio::error::operation_aborted) {
                    return;
                }
                if (ec == ws::error::closed) {
                    std::cout << "client socket shutdown" << std::endl;
                    return;
                }
                std::cerr<< ec.message() << std::endl;
                return;
            }

            auto data = boost::beast::buffers_to_string(self->buf_.data());
            self->buf_.consume(self->buf_.size());

            self->writeAsync(processCommand(data, self));
            self->onReadAsync();
        });
}

void Session::onWriteAsync()
{
    if (writeMessages_.empty())
        return;
    ws_->text(ws_->got_text());
    ws_->async_write(boost::asio::buffer(writeMessages_.front()), [self = shared_from_this()](boost::system::error_code ec, size_t)
        {
            std::lock_guard lock(self->writeMessagesMutex_);
            self->writeMessages_.pop_front();
            if (!self->writeMessages_.empty())
                self->onWriteAsync();

            if (ec) {
                if (ec == boost::asio::error::operation_aborted) {
                    return;
                }
                std::cerr<< ec.message() << std::endl;
            }
        });
}

void Session::writeAsync(const std::string& message)
{
    std::lock_guard lock(writeMessagesMutex_);
    writeMessages_.push_back(message);
    if (writeMessages_.size() == 1)
        onWriteAsync();
}

std::string Session::processCommand(const std::string& command, std::shared_ptr<Session> session)
{
    std::stringstream ss;
    std::vector<std::string> parts;
    boost::split(parts, command, boost::is_any_of(" "));

    try {
        auto code = static_cast<InCommandCode>(std::stoi(parts[0]));

        if (code != InCommandCode::AUTH && !session->player_) {
            ss << OutCommandCode::ERROR << ' ' << ErrorCode::ERROR_NOT_AUTH;
            return ss.str();
        }

        switch (code) {
            case AUTH: {
                if (session->player_) {
                    ss << OutCommandCode::ERROR << ' ' << ErrorCode::ERROR_ALREADY_AUTH;
                    break;
                }
                if (parts.size() < 2) {
                    ss << OutCommandCode::ERROR << ' ' << ErrorCode::INCORRECT_FORMAT;
                    break;
                }
                session->player_ = session->playerManager_->createPlayer(parts[1]);
                session->player_->setNotificationHandler([weakSelf = std::weak_ptr(session)](const Notification& notification)
                    {
                        if (auto self = weakSelf.lock())
                            self->writeAsync(processNotification(notification, self));
                    });

                ss << OutCommandCode::PLAYER_AUTHED << ' ' << session->player_->id();
                break;
            }
            case CREATE_GAME: {
                if (session->player_->isInGame()) {
                    ss << OutCommandCode::ERROR << ' ' << ErrorCode::ERROR_CREATE;
                    break;
                }
                auto gameId = session->gameManager_->createGame();
                bool res = session->gameManager_->addPlayerToGame(session->player_, gameId);
                if (!res) {
                    ss << OutCommandCode::ERROR << ' ' << ErrorCode::ERROR_CREATE;
                } else {
                    ss << OutCommandCode::GAME_CREATED << ' ' << gameId;
                }
                break;
            }
            case GET_GAMES: {
                ss << OutCommandCode::GAME_LIST;
                auto games = session->gameManager_->getWaitingGames();
                for (const auto& game : games) {
                    ss << ' ' << game->id() << '|' << game->player1()->nickname();
                }
                break;
            }
            case JOIN_GAME: {
                auto gameId = session->uuidStrGen_(parts[1]);
                bool res = session->gameManager_->addPlayerToGame(session->player_, gameId);
                if (!res) {
                    ss << OutCommandCode::ERROR << ' ' << ErrorCode::ERROR_JOIN;
                } else {
                    ss << OutCommandCode::JOINED_GAME;
                }
                break;
            }
            case LEAVE_GAME: {
                bool res = session->gameManager_->leavePlayerFromGame(session->player_);
                if (!res) {
                    ss << OutCommandCode::ERROR << ' ' << ErrorCode::ERROR_LEAVE;
                } else {
                    ss << OutCommandCode::LEFT_GAME;
                }
                break;
            }
            case MOVE: {
                if (parts.size() < 3) {
                    ss << OutCommandCode::ERROR << ' ' << ErrorCode::INCORRECT_FORMAT;
                    break;
                }
                auto x = std::stoi(parts[1]);
                auto y = std::stoi(parts[2]);
                bool res = session->gameManager_->makeMove(session->player_, x, y);
                if (!res) {
                    ss << OutCommandCode::ERROR << ' ' << ErrorCode::ERROR_MOVE;
                } else {
                    ss << OutCommandCode::MOVED;
                }
                break;
            }
            default:
                ss << OutCommandCode::ERROR << ' ' << ErrorCode::UNKNOWN_COMMAND;
        }
    } catch (std::exception& e) {
        ss << OutCommandCode::ERROR << ' ' << ErrorCode::INCORRECT_FORMAT;
    }

    return ss.str();
}

std::string Session::processNotification(const Notification& notification, std::shared_ptr<Session> session)
{
    std::stringstream ss;
    std::string opponentIdStr = (notification.playerId ? boost::uuids::to_string(*notification.playerId) : "");
    switch (notification.type) {
        case Notification::Type::PlayerJoined:
            ss << OutCommandCode::OPPONENT_JOINED << ' ' << opponentIdStr;
            break;
        case Notification::Type::PlayerLeft:
            ss << OutCommandCode::OPPONENT_LEFT << ' ' << opponentIdStr;
            break;
        case Notification::Type::PlayerMoved:
            ss << OutCommandCode::OPPONENT_MOVED << ' ' << notification.extraInfo << ' ' << opponentIdStr;
            break;
        case Notification::Type::GameEnded:
            ss << OutCommandCode::GAME_ENDED << ' ' << (opponentIdStr.empty() ? "draw" : opponentIdStr);
            break;
    }

    return ss.str();
}
