#pragma once

#include "game.h"
#include "common.h"

#include <boost/uuid/random_generator.hpp>

#include <shared_mutex>
#include <unordered_map>
#include <iostream>

class GameManager {
public:
    const Id& createGame();

    bool addPlayerToGame(std::shared_ptr<Player> player, const Id& gameId);
    bool leavePlayerFromGame(std::shared_ptr<Player> player);

    bool makeMove(std::shared_ptr<Player> player, int x, int y);
    std::vector<std::shared_ptr<Game>> getWaitingGames() const;

    std::shared_ptr<Game> getGame(const Id& gameId) const;
    void removeGame(const Id& gameId);
private:
    uint32_t getNewId()
    {
        return idCounter_++;
    }

    std::unordered_map<Id, std::shared_ptr<Game>> games_;
    mutable std::shared_mutex mutex_;

    std::atomic<uint32_t> idCounter_;
};
