cmake_minimum_required(VERSION 3.10 FATAL_ERROR)

set(PROJECT_NAME ByteSeal)
project (${PROJECT_NAME})

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Vulkan REQUIRED)

if(WIN32)
    set(SDL3_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src/external/SDL3/include")
    set(SDL3_LINK_TARGET "${CMAKE_CURRENT_SOURCE_DIR}/src/external/SDL3/lib/x64/SDL3.lib")
else()
    find_package(SDL3 REQUIRED CONFIG)
    set(SDL3_INCLUDE_DIR "") 
    set(SDL3_LINK_TARGET SDL3::SDL3-shared)
endif()

add_executable(${PROJECT_NAME}
    src/Vulkan/main.cpp
    src/Vulkan/vk_engine.cpp
    src/Vulkan/vk_initializers.cpp
    src/Vulkan/vk_images.cpp
    src/Vulkan/vk_pipelines.cpp
    src/Vulkan/vk_descriptors.cpp
    src/Vulkan/vk_loader.cpp
    src/Vulkan/vk_types.h
    
    src/external/vk-bootstrap/src/VkBootstrap.h
    src/external/vk-bootstrap/src/VkBootstrap.cpp

    src/external/imgui/imgui.cpp
    src/external/imgui/imgui_draw.cpp
    src/external/imgui/imgui_tables.cpp
    src/external/imgui/imgui_widgets.cpp
    src/external/imgui/imgui_demo.cpp

    src/external/imgui/backends/imgui_impl_sdl3.cpp
    src/external/imgui/backends/imgui_impl_vulkan.cpp

    src/external/tiny_obj_loader.h
)

target_include_directories(${PROJECT_NAME} PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src/external

     ${CMAKE_CURRENT_SOURCE_DIR}/src/external/stb
     ${CMAKE_CURRENT_SOURCE_DIR}/src/external/glm

    ${CMAKE_CURRENT_SOURCE_DIR}/src/external/fmt/include

     ${CMAKE_CURRENT_SOURCE_DIR}/src/external/VulkanMemoryAllocator/include 
  
     ${CMAKE_CURRENT_SOURCE_DIR}/src/external/vk-bootstrap/src

     ${CMAKE_CURRENT_SOURCE_DIR}/src/external/imgui
     ${CMAKE_CURRENT_SOURCE_DIR}/src/external/imgui/backends

     ${SDL3_INCLUDE_DIR}

     ${CMAKE_CURRENT_SOURCE_DIR}/src/Vulkan
)

target_compile_definitions(${PROJECT_NAME} PRIVATE 
    VK_BOOTSTRAP_VULKAN_HEADER_VERSION=350
    VMA_STATIC_VULKAN_FUNCTIONS=1
    VMA_DYNAMIC_VULKAN_FUNCTIONS=0
    FMT_HEADER_ONLY
)

if(WIN32)
    target_compile_definitions(${PROJECT_NAME} PRIVATE
    VK_USE_PLATFORM_WIN32_KHR
    )
elseif(UNIX AND NOT APPLE)
    target_compile_definitions(${PROJECT_NAME} PRIVATE
    VK_USE_PLATFORM_XLIB_KHR
    VK_USE_PLATFORM_WAYLAND_KHR
    )
endif()

target_link_libraries(${PROJECT_NAME} 
    Vulkan::Vulkan
    Vulkan::Headers
    ${SDL3_LINK_TARGET} 
)

if(WIN32)
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_CURRENT_SOURCE_DIR}/src/external/SDL3/lib/x64/SDL3.dll"
        $<TARGET_FILE_DIR:${PROJECT_NAME}>
    )
endif()

# Отключаем предупреждение о nullability-типах для компилятора Clang
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    target_compile_options(${PROJECT_NAME} PRIVATE -Wno-nullability-completeness)
endif()

if(MSVC)
    # Если компилятор родной MSVC от Microsoft (или Clang-CL в режиме MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE "/utf-8")
elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang" OR CMAKE_CXX_COMPILER_ID MATCHES "GNU")
    # Если сборка идет через чистый Clang или GCC
    target_compile_options(${PROJECT_NAME} PRIVATE "-finput-charset=utf-8" "-fexec-charset=utf-8")
endif()
