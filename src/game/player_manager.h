#pragma once

#include "player.h"

#include <boost/uuid/random_generator.hpp>

#include <memory>

class PlayerManager {
public:
    std::shared_ptr<Player> createPlayer()
    {
        return std::make_shared<Player>(generator_());
    }

private:
    boost::uuids::random_generator generator_;
};
