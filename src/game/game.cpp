#include "game.h"

#include <boost/format.hpp>

Game::Game(Id gameId)
    : id_(gameId)
    , player1_(nullptr)
    , player2_(nullptr)
    , isOver_(false)
{
    board_.fill({Cell::None, Cell::None, Cell::None });
}

Game::~Game()
{
    endGame();
}

const Id& Game::id() const
{
    return id_;
}

bool Game::isOver() const
{
    return isOver_;
}

std::shared_ptr<Player> Game::player1() const
{
    return player1_;
}

std::shared_ptr<Player> Game::player2() const
{
    return player2_;
}

bool Game::join(std::shared_ptr<Player> player)
{
    std::lock_guard lock(gameMutex_);
    if (player1_ == nullptr) {
        player1_ = player;
        curPlayerId_ = player->id();
    } else if (player2_ == nullptr) {
        if (player1_ == player)
            return false;
        player2_ = player;
        player1_->notify(Notification {
            .type = Notification::Type::PlayerJoined,
            .playerId = player->id(),
        });
    } else {
        return false;
    }

    player->joinGame(id_);
    return true;
}

bool Game::leave(std::shared_ptr<Player> player) {
    std::lock_guard lock(gameMutex_);
    if (isOver_ || (player1_ != player && player2_ != player))
        return false;

    if (player1_ != nullptr && player2_ != nullptr) {
        auto winner = (player == player1_) ? player2_ : player1_;
        winnerId = winner->id();
        winner->notify(Notification{
                .type = Notification::Type::PlayerLeft,
                .playerId = player->id(),
        });
    }

    player->leaveGame();
    endGame();
    return true;
}

bool Game::makeMove(Id playerId, int x, int y)
{
    std::lock_guard lock(gameMutex_);

    if (isOver_ || player1_ == nullptr || player2_ == nullptr || playerId != curPlayerId_ || !isValidMove(x, y)) {
        return false;
    }

    board_[x][y] = getCurCell();
    auto otherPlayer = (player1_->id() == playerId) ? player2_ : player1_;
    otherPlayer->notify(Notification {
        .type = Notification::Type::PlayerMoved,
        .playerId = playerId,
        .extraInfo = (boost::format("%d %d %s") % x % y % (board_[x][y] == Cell::X ? 'X' : 'O')).str()
    });

    auto gameStatus = checkFinish();
    if (gameStatus) {
        if (gameStatus != Cell::None)
            winnerId = (gameStatus == Cell::X) ? player1_->id() : player2_->id();
        endGame();
    } else {
        switchPlayer();
    }
    return true;
}

bool Game::isValidMove(int x, int y) const
{
    return x >= 0 && x < 3 && y >= 0 && y < 3 && board_[x][y] == Cell::None;
}

Game::Cell Game::getCurCell() const
{
    return (curPlayerId_ == player1_->id()) ? Cell::X : Cell::O;
}

void Game::switchPlayer()
{
    curPlayerId_ = (curPlayerId_ == player1_->id()) ? player2_->id() : player1_->id();
}

std::optional<Game::Cell> Game::checkFinish() const
{
    bool boardFull = true;
    for (int i = 0; i < 3; ++i) {
        if (board_[i][0] != Cell::None && board_[i][0] == board_[i][1] && board_[i][0] == board_[i][2]) return board_[i][0];
        if (board_[0][i] != Cell::None && board_[0][i] == board_[1][i] && board_[0][i] == board_[2][i]) return board_[0][i];
        for (int j = 0; j < 3; ++j) {
            if (board_[i][j] == Cell::None) boardFull = false;
        }
    }

    if (board_[0][0] != Cell::None && board_[0][0] == board_[1][1] && board_[0][0] == board_[2][2]) return board_[0][0];
    if (board_[0][2] != Cell::None && board_[0][2] == board_[1][1] && board_[0][2] == board_[2][0]) return board_[0][2];

    return boardFull ? std::make_optional(Cell::None) : std::nullopt;
}

void Game::endGame()
{
    if (isOver_)
        return;

    isOver_ = true;

    Notification notification {
        .type = Notification::Type::GameEnded,
        .playerId = winnerId,
    };

    if (player1_) {
        player1_->leaveGame();
        player1_->notify(notification);
    }
    if (player2_) {
        player2_->leaveGame();
        player2_->notify(notification);
    }
}
