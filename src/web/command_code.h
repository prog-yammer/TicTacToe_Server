#pragma once

enum InCommandCode {
    AUTH        = 0,
    CREATE_GAME = 1,
    GET_GAMES   = 2,
    JOIN_GAME   = 3,
    LEAVE_GAME  = 4,
    MOVE        = 5,
};

enum OutCommandCode {
    ERROR           = -1,
    PLAYER_AUTHED   = 0,
    GAME_CREATED    = 1,
    GAME_LIST       = 2,
    JOINED_GAME     = 3,
    LEFT_GAME       = 4,
    MOVED           = 5,
    OPPONENT_JOINED = 6,
    OPPONENT_LEFT   = 7,
    OPPONENT_MOVED  = 8,
    GAME_ENDED      = 9,
};

enum ErrorCode {
    UNKNOWN_COMMAND    = 0,
    INCORRECT_FORMAT   = 1,
    ERROR_NOT_AUTH     = 2,
    ERROR_ALREADY_AUTH = 2,
    ERROR_JOIN         = 3,
    ERROR_CREATE       = 4,
    ERROR_LEAVE        = 5,
    ERROR_MOVE         = 6,
};
