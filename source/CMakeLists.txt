add_executable(hello
    hello.cpp
)

message(LIBRARIES=${LIBRARIES})

target_link_libraries(hello ${LIBRARIES})
