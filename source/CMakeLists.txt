add_executable(hello
    hello.cpp
)

target_link_libraries(hello ${LIBRARIES})

add_executable(main
    main.cpp
    engine.cpp
    pipeline_builder.cpp
    vk_init.cpp
    vk_mesh.cpp
)

target_link_libraries(main ${LIBRARIES})
