cmake_minimum_required(VERSION 3.20)

project(PBR VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)

include_directories("external/vulkan/include")
include_directories("external/glfw/include")
include_directories("external/glm")
include_directories("external/assimp/include")
include_directories("external/assimp/contrib/stb")

add_subdirectory("external/glfw")
add_subdirectory("external/assimp")

link_directories("external/vulkan/lib")

add_executable(PBR src/sandbox/main.cpp)

target_link_libraries(PBR glfw vulkan-1)