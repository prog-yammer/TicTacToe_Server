#include "player.h"

Player::Player(Id id, const std::string& nickname)
    : id_(std::move(id))
    , nickname_(nickname)
    , curGameId_(std::nullopt)
{}

const Id& Player::id() const
{
    return id_;
}

const std::string& Player::nickname() const {
    return nickname_;
}

bool Player::isInGame() const
{
    return curGameId_.has_value();
}

std::optional<Id> Player::curGameId() const
{
    return curGameId_;
}

bool Player::joinGame(const Id& gameId)
{
    if (isInGame())
        return false;

    curGameId_ = gameId;
    return true;
}

bool Player::leaveGame()
{
    bool result = isInGame();
    if (result)
        curGameId_.reset();

    return result;
}

void Player::setNotificationHandler(NotificationHandler notificationHandler)
{
    notificationHandler_ = std::move(notificationHandler);
}

void Player::notify(const Notification& notification)
{
    if (notificationHandler_)
        notificationHandler_(notification);
}
