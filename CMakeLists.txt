cmake_minimum_required(VERSION 3.25)
project(BattleShip_Game C)

set(CMAKE_C_STANDARD 11)

set(SDL2_INCLUDE_DIRS "SDL/SDL2/include")
set(SDL2_LIBRARY_DIRS "SDL/SDL2/lib/x64")
set(SDL2-mingw_INCLUDE_DIRS "SDL/SDL2-mingw/x86_64-w64-mingw32/include/SDL2")
set(SDL2-mingw_LIBRARY_DIRS "SDL/SDL2-mingw/x86_64-w64-mingw32/lib")
set(SDL2_IMAGE_INCLUDE_DIRS "SDL/SDL2_image/x86_64-w64-mingw32/include/SDL2")
set(SDL2_IMAGE_LIBRARY_DIRS "SDL/SDL2_image/x86_64-w64-mingw32/lib")
set(SDL2_TTF_INCLUDE_DIRS "SDL/SDL2_ttf/x86_64-w64-mingw32/include/SDL2")
set(SDL2_TTF_LIBRARY_DIRS "SDL/SDL2_ttf/x86_64-w64-mingw32/lib")

include_directories(${SDL2_INCLUDE_DIRS} ${SDL2_IMAGE_INCLUDE_DIRS} ${SDL2_TTF_INCLUDE_DIRS} ${PROJECT_SOURCE_DIR}/include)
link_directories(${SDL2_LIBRARY_DIRS} ${SDL2_IMAGE_LIBRARY_DIRS} ${SDL2_TTF_LIBRARY_DIRS})

add_executable(BattleShip_Game
        main.c
        pcg_basic.c
        )

target_link_libraries(BattleShip_Game SDL2_image SDL2 SDL2main SDL2_ttf)

set(DLL_SRC_DIRS
        "SDL/SDL2/lib/x64"
        "SDL/SDL2_image/x86_64-w64-mingw32/bin"
        "SDL/SDL2_ttf/x86_64-w64-mingw32/bin"
        )

foreach(DLL_SRC_DIR ${DLL_SRC_DIRS})
    file(GLOB_RECURSE DLL_FILES LIST_DIRECTORIES false "${DLL_SRC_DIR}/*.dll")
    foreach(DLL_FILE ${DLL_FILES})
        add_custom_command(TARGET BattleShip_Game POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${DLL_FILE}"
                $<TARGET_FILE_DIR:BattleShip_Game>
                )
    endforeach()
endforeach()

set_target_properties(BattleShip_Game PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
        )