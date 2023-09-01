cmake_minimum_required(VERSION 3.20 FATAL_ERROR)

add_executable(hello
    hello.cpp
)

message(LIBRARIES=${LIBRARIES})

target_link_libraries(hello ${LIBRARIES})
