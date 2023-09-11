add_executable(hello
    hello.cpp
)

target_link_libraries(hello ${LIBRARIES})


add_executable(main
    main.cpp
    engine.cpp
)

target_link_libraries(main ${LIBRARIES})
