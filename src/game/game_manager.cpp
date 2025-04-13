#include "game_manager.h"

#include <algorithm>

const Id& GameManager::createGame()
{
    std::unique_lock lock(mutex_);
    auto gameId = getNewId();
    auto game = std::make_shared<Game>(gameId);
    games_.emplace(gameId, game);

    return game->id();
}

bool GameManager::addPlayerToGame(std::shared_ptr<Player> player, const Id& gameId)
{
    if (player->isInGame())
        return false;

    auto game = getGame(gameId);
    if (!game)
        return false;

    bool result = game->join(player);

    return result;
}

bool GameManager::leavePlayerFromGame(std::shared_ptr<Player> player)
{
    if (!player->isInGame())
        return false;

    auto game = getGame(*player->curGameId());
    if (!game)
        return false;

    bool result = game->leave(player);
    return result;
}

bool GameManager::makeMove(std::shared_ptr<Player> player, int x, int y)
{
    if (!player->isInGame())
        return false;

    auto game = getGame(*player->curGameId());
    if (!game)
        return false;

    bool result = game->makeMove(player->id(), x, y);

    if (result && game->isOver()) {
        removeGame(game->id());
    }

    return result;
}

std::vector<std::shared_ptr<Game>> GameManager::getWaitingGames() const
{
    std::shared_lock lock(mutex_);
    std::vector<std::shared_ptr<Game>> result;
    for (const auto& [_, game] : games_) {
        if (!game->isOver() && game->player1() && !game->player2()) {
            result.push_back(game);
        }
    }
    return result;
}

std::shared_ptr<Game> GameManager::getGame(const Id& gameId) const
{
    std::shared_lock lock(mutex_);
    auto it = games_.find(gameId);
    if (it == games_.end())
        return nullptr;

    return it->second;
}

void GameManager::removeGame(const Id& gameId)
{
    std::unique_lock lock(mutex_);
    games_.erase(gameId);
}
