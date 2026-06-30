#pragma once

#include <memory>
#include <optional>
#include <string>
#include <vector>
#include <span>
#include <array>
#include <functional>
#include <deque>

#include <vulkan/vulkan.h>
#include <vulkan/vk_enum_string_helper.h>
#include <vk_mem_alloc.h>

#include <fmt/core.h>

#include <glm/mat4x4.hpp>
#include <glm/vec4.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp> // Чистые функции перемещения, вращения, масштабирования
#include <glm/gtc/type_ptr.hpp>         // Для получения указателя на данные матриц (если нужно)


// Только щяс понял это проверка которую мы писали в Vk-tutorial типо if(что-то там) то выбрасываем исключение.
// Это просто удобных макрос, чтобы по сто раз не писать
#define VK_CHECK(x)                                                     \
    do {                                                                \
        VkResult err = x;                                               \
        if (err) {                                                      \
             fmt::print("Detected Vulkan error: {}", string_VkResult(err)); \
            abort();                                                    \
        }                                                               \
    } while (0)

// Структура которая хранит буфер и его аллокатор
struct AllocatedBuffer {
    VkBuffer buffer;            // Указатель на буфер
    VmaAllocation allocation;   // Помнит конкретно выделенное место в памяти, нужен для удаления буфера
    VmaAllocationInfo info;     // Обязательно нужен для того что-бы копировать данные через memcpy
};


// Простейшая структура, которая хранит данные о треугольнике (vertex, uv)
struct Vertex {

    glm::vec3 position;
    float uv_x;
    glm::vec3 normal;
    float uv_y;
    glm::vec4 color;
};

// Короче: Это типо крутой умны система на Vulkan 1.3
// вместо того что-бы связывать дескрипторами, связывам напрямую указателем на 64 бита
// это называеться BDA и в BDA чаще пихают pushConstant
// Передаёшь указатель своей GPU и уже шейдер отлавливает адресс и работает с данными
struct GPUMeshBuffers {

    AllocatedBuffer indexBuffer;
    AllocatedBuffer vertexBuffer;
    VkDeviceAddress vertexBufferAddress;
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