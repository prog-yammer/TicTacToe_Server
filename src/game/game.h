#pragma once

#include "player.h"
#include "common.h"

#include <array>
#include <atomic>
#include <string>
#include <mutex>

class Game {
public:
    enum Cell { None, X, O, };

    explicit Game(Id gameId);
    ~Game();

    const Id& id() const;
    bool isOver() const;

    std::shared_ptr<Player> player1() const;
    std::shared_ptr<Player> player2() const;

    bool join(std::shared_ptr<Player> player);
    bool leave(std::shared_ptr<Player> player);
    bool makeMove(Id playerId, int x, int y);

private:
    bool isValidMove(int x, int y) const;
    Cell getCurCell() const;

    void switchPlayer();

    std::optional<Cell> checkFinish() const;
    void endGame();

    Id id_;
    std::shared_ptr<Player> player1_;
    std::shared_ptr<Player> player2_;
    Id curPlayerId_;
    std::optional<Id> winnerId;

    std::atomic_bool isOver_;
    std::array<std::array<Cell, 3>, 3> board_{};

    mutable std::mutex gameMutex_;
};