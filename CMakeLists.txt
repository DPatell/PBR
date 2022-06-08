cmake_minimum_required(VERSION 3.20)

project(PBR VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)

include_directories("external/vulkan/include")
include_directories("external/glfw/include")
include_directories("external/glm")
include_directories("external/assimp/include")
include_directories("external/assimp/contrib/stb")
include_directories("build/external/assimp/include")

add_subdirectory("external/glfw")
add_subdirectory("external/assimp")

link_directories("external/vulkan/lib")

file(GLOB PBR_SANDBOX_SOURCES
        src/sandbox/*.hpp
        src/sandbox/*.cpp
)

add_executable(PBR ${PBR_SANDBOX_SOURCES})

target_link_libraries(PBR glfw vulkan-1 assimp)