#pragma once

#include "common.h"

#include <functional>
#include <string>

struct Notification {
    enum class Type {
        PlayerJoined,
        PlayerLeft,
        PlayerMoved,
        GameEnded,
    };

    Type type;
    std::optional<Id> playerId;
    std::string extraInfo;
};

using NotificationHandler = std::function<void(const Notification&)>;
