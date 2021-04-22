cmake_minimum_required(VERSION 3.10)
project(minifs C)

set(CMAKE_C_STANDARD 99)

set(BUILD_DIR /home/debugger/code/mipt-linux-minifs-extended/build)

message(${BUILD_DIR})

set(CMAKE_C_FLAGS -fsanitize=address,undefined)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${BUILD_DIR})

add_executable(minifs main.c src/fs.c src/tokenizer.c src/adapter.c)

include_directories(include)