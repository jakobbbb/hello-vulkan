cmake_minimum_required(VERSION 3.20 FATAL_ERROR)
project(HelloVulkan)

# generate compile_commands.json
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# external libraries
set(EXTERNAL_DIR external)
add_subdirectory(${EXTERNAL_DIR})

# GLM
include_directories(${EXTERNAL_DIR}/glm)

# glfw
find_package(glfw3 REQUIRED)
include_directories(${GLFW_INCLUDE_DIRS})
set(LIBRARIES ${LIBRARIES} glfw)

# ImGui
include_directories(${EXTERNAL_DIR}/imgui)
set(LIBRARIES ${LIBRARIES} imgui)

# STB
include_directories(${EXTERNAL_DIR}/stb)

# tinyobjloader
include_directories(${EXTERNAL_DIR}/tinyobjloader)

# vk-bootstrap
include_directories(${EXTERNAL_DIR}/vk-bootstrap)
set(LIBRARIES ${LIBRARIES} vk-bootstrap::vk-bootstrap)

# vma
set(VulkanMemoryAllocator_DIR external/VulkanMemoryAllocator/build/install/share/cmake/VulkanMemoryAllocator)
find_package(VulkanMemoryAllocator CONFIG REQUIRED)
set(LIBRARIES ${LIBRARIES} GPUOpen::VulkanMemoryAllocator)

# vulkan
find_package(Vulkan REQUIRED)
set(LIBRARIES ${LIBRARIES} Vulkan::Vulkan)

#check for CMakeLists.txt files in subdirectory source (where your applications are)
add_subdirectory(source)
add_subdirectory(shaders)
