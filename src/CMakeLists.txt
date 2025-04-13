find_package(Boost REQUIRED COMPONENTS system program_options regex)
include_directories(${Boost_INCLUDE_DIRS})

add_library(TicTacToe_lib
        web/server.h         web/server.cpp
        web/session.h         web/session.cpp
        game/player.h         game/player.cpp
        game/game.h           game/game.cpp
        game/game_manager.h   game/game_manager.cpp
        game/player_manager.h
        game/notification.h
        game/common.h
        web/command_code.h
)

add_executable(${PROJECT_NAME} main.cpp)

target_link_libraries(${PROJECT_NAME} TicTacToe_lib
        ${Boost_PROGRAM_OPTIONS_LIBRARY} ${Boost_REGEX_LIBRARY} ${Boost_SYSTEM_LIBRARY})
