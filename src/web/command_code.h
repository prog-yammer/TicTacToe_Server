#pragma once

enum InCommandCode {
    CREATE_GAME = 0,
    JOIN_GAME   = 1,
    LEAVE_GAME  = 2,
    MOVE        = 3,
};

enum OutCommandCode {
    ERROR           = -1,
    PLAYER_CREATED  = 0,
    GAME_CREATED    = 1,
    JOINED_GAME     = 2,
    LEFT_GAME       = 3,
    MOVED           = 4,
    OPPONENT_JOINED = 5,
    OPPONENT_LEFT   = 6,
    OPPONENT_MOVED  = 7,
    GAME_ENDED      = 8,
};

enum ErrorCode {
    UNKNOWN_COMMAND  = 0,
    INCORRECT_FORMAT = 1,
    ERROR_JOIN       = 2,
    ERROR_CREATE     = 3,
    ERROR_LEAVE      = 4,
    ERROR_MOVE       = 5,
};
