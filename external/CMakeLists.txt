file(GLOB IMGUI_SOURCE RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} imgui/*.cpp)
add_library(imgui STATIC
    ${IMGUI_SOURCE}
)

add_subdirectory(vk-bootstrap)

add_subdirectory(VulkanMemoryAllocator)
