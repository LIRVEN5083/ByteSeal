cmake_minimum_required(VERSION 3.10 FATAL_ERROR)

set(PROJECT_NAME ByteSeal)
project (${PROJECT_NAME})

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_POLICY_DEFAULT_CMP0148 OLD)
set(DRACO_BUILD_ANIM_SEQUENCER OFF CACHE BOOL "" FORCE)
set(DRACO_BUILD_TRANSCODER OFF CACHE BOOL "" FORCE)
set(DRACO_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(DRACO_BUILD_TESTS OFF CACHE BOOL "Disable Draco tests" FORCE)
set(DRACO_TESTS OFF CACHE BOOL "Disable Draco tests" FORCE)
set(DRACO_BUILD_SHARED OFF CACHE BOOL "Disable Draco shared library" FORCE)

set(NFD_BUILD_TESTS OFF CACHE INTERNAL "")
set(NFD_BUILD_EXAMPLES OFF CACHE INTERNAL "")

find_package(Vulkan REQUIRED)

if(WIN32)
    set(SDL3_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src/external/SDL3/include")
    set(SDL3_LINK_TARGET "${CMAKE_CURRENT_SOURCE_DIR}/src/external/SDL3/lib/x64/SDL3.lib")
else()
    find_package(SDL3 REQUIRED CONFIG)
    set(SDL3_INCLUDE_DIR "")
    set(SDL3_LINK_TARGET SDL3::SDL3-shared)
endif()

if(UNIX AND NOT APPLE)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(GTK3 REQUIRED gtk+-3.0)
endif()

add_executable(${PROJECT_NAME}
        src/Vulkan/main.cpp
        src/Vulkan/vk_init_engine.cpp
        src/Vulkan/vk_initializers.cpp
        src/Vulkan/vk_images.cpp
        src/Vulkan/vk_pipelines.cpp
        src/Vulkan/vk_descriptors.cpp
        src/Vulkan/vk_types.h
        src/Vulkan/vk_application.cpp
        src/Vulkan/vk_glTF_loading.cpp
        src/Controller/movement.cpp
        src/Utils/File_loader.cpp
        src/Utils/shader_compile.cpp
        src/Vulkan/vk_scene.cpp
        src/Vulkan/vk_imgui.cpp

    src/external/vk-bootstrap/src/VkBootstrap.h
    src/external/vk-bootstrap/src/VkBootstrap.cpp

    src/external/imgui/imgui.cpp
    src/external/imgui/imgui_draw.cpp
    src/external/imgui/imgui_tables.cpp
    src/external/imgui/imgui_widgets.cpp

    src/external/ImGuizmo/src/ImGuizmo.cpp
    src/external/ImGuizmo/src/ImSequencer.cpp

    src/external/imgui/backends/imgui_impl_sdl3.cpp
    src/external/imgui/backends/imgui_impl_vulkan.cpp

    src/external/basisu/transcoder/basisu_transcoder.cpp

    src/external/tiny_obj_loader.h
)

if(MINGW)
    target_link_options(ByteSeal PRIVATE
            "-static-libgcc"
            "-static-libstdc++"
            "LINKER:-Bstatic"
            "LINKER:--whole-archive"
            "-lwinpthread"
            "LINKER:--no-whole-archive"
    )
endif()

add_subdirectory(src/external/nfd-extended)
add_subdirectory(src/external/fastgltf)
add_subdirectory(src/external/draco)

target_include_directories(${PROJECT_NAME} PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src/external

    ${CMAKE_CURRENT_SOURCE_DIR}/src/external/ImGuizmo/src

    ${CMAKE_CURRENT_SOURCE_DIR}/src/external/stb
     ${CMAKE_CURRENT_SOURCE_DIR}/src/external/glm

    ${CMAKE_CURRENT_SOURCE_DIR}/src/external/fmt/include

     ${CMAKE_CURRENT_SOURCE_DIR}/src/external/VulkanMemoryAllocator/include

     ${CMAKE_CURRENT_SOURCE_DIR}/src/external/vk-bootstrap/src

     ${CMAKE_CURRENT_SOURCE_DIR}/src/external/imgui
     ${CMAKE_CURRENT_SOURCE_DIR}/src/external/imgui/backends

     ${SDL3_INCLUDE_DIR}

     ${CMAKE_CURRENT_SOURCE_DIR}/src/Vulkan

     ${CMAKE_CURRENT_SOURCE_DIR}/src/external/basisu

     ${CMAKE_CURRENT_SOURCE_DIR}/src/external/gli

     ${CMAKE_CURRENT_SOURCE_DIR}/src/external/nfd-extended/src/include

     ${Vulkan_INCLUDE_DIRS}
)

target_compile_definitions(${PROJECT_NAME} PRIVATE
    VK_BOOTSTRAP_VULKAN_HEADER_VERSION=350
    VMA_STATIC_VULKAN_FUNCTIONS=1
    VMA_DYNAMIC_VULKAN_FUNCTIONS=0
    FMT_HEADER_ONLY
    BASISD_SUPPORT_KTX2_ZSTD=0
    FASTGLTF_ENABLE_DRACO=1
    GLM_ENABLE_EXPERIMENTAL
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

if(WIN32)
    set(SHADERC_LIB "C:/VulkanSDK/1.4.357.0/Bin/shaderc_shared.dll")
    set(SHADERC_DEPS "")

    target_include_directories(${PROJECT_NAME} PUBLIC "C:/VulkanSDK/1.4.357.0/Include")

    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${SHADERC_LIB}"
            $<TARGET_FILE_DIR:${PROJECT_NAME}>
    )
else()
    find_library(SHADERC_LIB NAMES shaderc_combined PATHS ${Vulkan_LIBRARY_DIRS} REQUIRED)

    find_package(glslang REQUIRED)
    find_package(SPIRV-Tools REQUIRED)

    set(SHADERC_DEPS
            glslang::glslang
            glslang::SPIRV
            SPIRV-Tools-opt
            SPIRV-Tools
    )
endif()

target_link_libraries(${PROJECT_NAME}
    Vulkan::Vulkan
    Vulkan::Headers
    ${SDL3_LINK_TARGET}
    draco_static
    fastgltf::fastgltf
    nfd
    ${SHADERC_LIB}
    ${SHADERC_DEPS}
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
