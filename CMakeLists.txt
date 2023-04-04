cmake_minimum_required(VERSION 3.25)
project(BattleShip_Game C)

set(CMAKE_C_STANDARD 11)

set(SDL2_INCLUDE_DIRS "C:/SDL2-mingw/x86_64-w64-mingw32/include/SDL2")
set(SDL2_LIBRARIES "C:/SDL2-mingw/x86_64-w64-mingw32/lib/libSDL2.dll.a" "C:/SDL2-mingw/x86_64-w64-mingw32/lib/libSDL2main.a")
set(SDL2_IMAGE_LIBRARY "C:/SDL2_image/lib/x64/SDL2_image.lib")

include_directories(${SDL2_INCLUDE_DIRS} ${PROJECT_SOURCE_DIR}/include)

add_executable(BattleShip_Game main.c)

target_link_libraries(BattleShip_Game ${SDL2_IMAGE_LIBRARY})
target_link_libraries(BattleShip_Game ${SDL2_LIBRARIES})
