cmake_minimum_required(VERSION 3.31)
project(chip-8 C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/build/bin)

set (SOURCE
    ${CMAKE_CURRENT_SOURCE_DIR}/src/main.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/chip-8.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/sdl.c
)

add_executable(${PROJECT_NAME} ${SOURCE})

target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/SDL2/include
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/SDL2/lib/libSDL2.a
    m
    pthread
    dl
)
