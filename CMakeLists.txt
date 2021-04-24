cmake_minimum_required(VERSION 3.10)
project(minifs C)

set(CMAKE_C_STANDARD 99)

set(BUILD_DIR ${CMAKE_CURRENT_SOURCE_DIR}/build)
message(${BUILD_DIR})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${BUILD_DIR})

set(CMAKE_C_FLAGS -fsanitize=address,undefined)

include_directories(include)

add_executable(server src/server/server.c src/common/tokenizer.c
        src/server/fs.c src/server/adapter.c src/common/net_utils.c)
target_link_libraries(server pthread)

add_executable(client src/client/client.c src/common/tokenizer.c
        src/common/net_utils.c)