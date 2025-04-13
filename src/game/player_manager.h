#pragma once

#include "player.h"

#include <boost/uuid/random_generator.hpp>

#include <memory>

class PlayerManager {
public:
    std::shared_ptr<Player> createPlayer(const std::string& nickname)
    {
        return std::make_shared<Player>(getNewId(), nickname);
    }

private:
    uint32_t getNewId()
    {
        return idCounter_++;
    }

    std::atomic<uint32_t> idCounter_;
};
