cmake_minimum_required(VERSION 3.20 FATAL_ERROR)

file(GLOB IMGUI_SOURCE RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} imgui/*.cpp)

message("IMGUI_SOURCE is ${IMGUI_SOURCE}")

add_library(imgui STATIC
    ${IMGUI_SOURCE}
)
