cmake_minimum_required(VERSION 3.16)
project(ChessForge3D VERSION 0.2.2 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(PkgConfig REQUIRED)
pkg_check_modules(SDL2 REQUIRED IMPORTED_TARGET sdl2)

add_executable(chessforge3d
    src/main.cpp
    src/chess.cpp
    src/engine.cpp
    src/font.cpp
    src/i18n.cpp
    src/input_map.cpp
    src/pieces3d.cpp
)

target_include_directories(chessforge3d PRIVATE src)
target_link_libraries(chessforge3d PRIVATE PkgConfig::SDL2)
target_compile_options(chessforge3d PRIVATE -Wall -Wextra -Wpedantic -Wconversion)

option(CHESSFORGE_BUILD_TESTS "Build the standalone regression tests (chess rules, translations, button mapping, 3D pieces)" OFF)
if(CHESSFORGE_BUILD_TESTS)
    add_executable(chess_rules_tests
        tests/chess_tests.cpp
        src/chess.cpp
    )
    target_include_directories(chess_rules_tests PRIVATE src)
    target_compile_options(chess_rules_tests PRIVATE -Wall -Wextra -Wpedantic -Wconversion)

    add_executable(ui_tests
        tests/ui_tests.cpp
        src/chess.cpp
        src/font.cpp
        src/i18n.cpp
        src/input_map.cpp
        src/pieces3d.cpp
    )
    target_include_directories(ui_tests PRIVATE src)
    target_compile_options(ui_tests PRIVATE -Wall -Wextra -Wpedantic -Wconversion)
endif()

install(TARGETS chessforge3d RUNTIME DESTINATION bin)
