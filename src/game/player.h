#pragma once

#include "notification.h"
#include "common.h"

#include <boost/uuid/uuid.hpp>

#include <optional>
#include <memory>

class Player {
public:
    explicit Player(Id id, const std::string& nickname);

    const Id& id() const;
    const std::string& nickname() const;
    bool isInGame() const;
    std::optional<Id> curGameId() const;

    bool joinGame(const Id& gameId);
    bool leaveGame();

    void setNotificationHandler(NotificationHandler notificationHandler);
    void notify(const Notification& notification);

private:
    Id id_;
    std::string nickname_;
    std::optional<Id> curGameId_;

    NotificationHandler notificationHandler_;
};
