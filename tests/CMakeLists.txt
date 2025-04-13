add_executable(GameTests test_game.cpp)
add_executable(WsTests test_ws.cpp)

find_package(Boost REQUIRED COMPONENTS unit_test_framework)

enable_testing()

target_link_libraries(GameTests PRIVATE TicTacToe_lib Boost::unit_test_framework)
target_link_libraries(WsTests PRIVATE TicTacToe_lib Boost::unit_test_framework)

add_test(NAME GameTests COMMAND GameTests)
add_test(NAME WsTests COMMAND WsTests)
