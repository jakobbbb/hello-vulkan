add_library(engine
    engine.cpp
    pipeline_builder.cpp
    vk_init.cpp
    vk_mesh.cpp
)
target_link_libraries(engine ${LIBRARIES})

add_executable(main
    main.cpp
    hello_engine.cpp
)
target_link_libraries(main ${LIBRARIES} engine)
