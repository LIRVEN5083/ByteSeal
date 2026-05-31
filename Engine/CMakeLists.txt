cmake_minimum_required(VERSION 3.10 FATAL_ERROR)

set(PROJECT_NAME ByteSeal)
project (${PROJECT_NAME})

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Vulkan REQUIRED)

add_executable(${PROJECT_NAME} 
    src/main.cpp

    src/GLSL_files/load_GLSL.hpp
    src/GLSL_files/load_GLSL.cpp
    src/InitClass.cpp
)

target_include_directories(${PROJECT_NAME} PUBLIC
     ${Vulkan_INCLUDE_DIRS}
      ${CMAKE_CURRENT_SOURCE_DIR}/src/external/stb-master
)

set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "Build examples" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "Build tests" FORCE)
set(GLFW_BUILD_DOCS OFF CACHE BOOL "Build docs" FORCE)
set(GLFW_INSTALL OFF CACHE BOOL "Install GLFW" FORCE)

add_subdirectory(src/external/glfw)

target_link_libraries(${PROJECT_NAME} 
    glfw
    Vulkan::Vulkan
)