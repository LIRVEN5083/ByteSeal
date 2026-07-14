#pragma once

#include <memory>
#include <optional>
#include <string>
#include <vector>
#include <span>
#include <array>
#include <functional>
#include <deque>
#include <chrono>

#include <vulkan/vulkan.h>
#include <vulkan/vk_enum_string_helper.h>
#include <vk_mem_alloc.h>

#include <fmt/core.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/mat4x4.hpp>
#include <glm/vec4.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <gli/gli.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/string_cast.hpp>

#include "transcoder/basisu_transcoder.h"


#define VK_CHECK(x)                                                     \
    do {                                                                \
        VkResult err = x;                                               \
        if (err) {                                                      \
            fmt::print("Detected Vulkan error: {}", string_VkResult(err)); \
            abort();                                                    \
        }                                                                      \
    } while (0)

// Структура которая хранит буфер и его аллокатор
struct AllocatedBuffer {
    VkBuffer buffer;            // Указатель на буфер
    VmaAllocation allocation;   // Помнит конкретно выделенное место в памяти, нужен для удаления буфера
    VmaAllocationInfo info;     // Обязательно нужен для того что-бы копировать данные через memcpy
};

// push constants для работы
struct GPUDrawPushConstants {
    glm::mat4 worldMatrix;          // Обычная матрица преобразований
    VkDeviceAddress vertexBuffer;   // Вершинный буфер который мы алоцировали и получили адресс дляс передачи
};

struct GPUSceneData {
    glm::mat4 view;
    glm::mat4 proj;
    glm::mat4 viewproj;
    glm::vec4 ambientColor;
    glm::vec4 sunlightDirection; // w for sun power
    glm::vec4 sunlightColor;
};