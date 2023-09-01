add_executable(hello
    hello.cpp
)

target_link_libraries(hello ${LIBRARIES})

# generate compile_commands.json
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
