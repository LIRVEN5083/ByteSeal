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
#include <glm/ext/quaternion_float.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/matrix_decompose.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <glm/gtc/packing.hpp>
#include <gli/gli.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/string_cast.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>
#include <bit>

#include "imgui.h"
#include "imgui_internal.h"
#include "imgui_impl_sdl3.h"
#include "imgui_impl_vulkan.h"
#include "ImGuizmo.h"
#include "ImSequencer.h"


#include "transcoder/basisu_transcoder.h"
#include "../Controller/movement.hpp"
#include <../Utils/shader_compile.h>

#define VK_CHECK(x)                                                     \
    do {                                                                \
        VkResult err = x;                                               \
        if (err) {                                                      \
            fmt::print("Detected Vulkan error: {}", string_VkResult(err)); \
            abort();                                                    \
        }                                                                      \
    } while (0)

// Очередь удаления
// Короче мы используем функцию для создание и пишем лямбду на удаление.
// А когда вызываем flush то удаляем накопленую очередь функций (которые удаляют созданные обьекты)
struct DeletionQueue
{
    std::deque<std::function<void()>> deletors;

    void push_function(std::function<void()>&& function) {
        deletors.push_back(function);
    }

    void flush() {
        // reverse iterate the deletion queue to execute all the functions
        for (auto it = deletors.rbegin(); it != deletors.rend(); it++) {
            (*it)(); //call functors
        }

        deletors.clear();
    }
};

struct AllocatedImage {
    VkImage image;
    VkImageView imageView;
    VmaAllocation allocation;
    VkExtent3D imageExtent;
    VkFormat imageFormat;
};

// Структура которая хранит буфер и его аллокатор
struct AllocatedBuffer {
    VkBuffer buffer;            // Указатель на буфер
    VmaAllocation allocation;   // Помнит конкретно выделенное место в памяти, нужен для удаления буфера
    VmaAllocationInfo info;     // Обязательно нужен для того что-бы копировать данные через memcpy
};

// Делим ебанные обьекты по типу аллокации
// И времени их существования на сцене

// Static - arena-allocator
// Dynamic - динамическое выделение
enum class ModelLifetime : uint8_t{
    Static,
    Dynamic
};

struct GPUMeshBuffers {

    AllocatedBuffer indexBuffer;
    AllocatedBuffer vertexBuffer;
    VkDeviceAddress vertexBufferAddress;

    ModelLifetime lifetime{ ModelLifetime::Dynamic };
};

struct GPUTexture {
    AllocatedImage image;
    uint32_t globalIndex{ 0 };
    uint32_t mipLevels;

    VkSampler sampler;

    VkDescriptorSet imguiDescriptorSet{VK_NULL_HANDLE};
    ModelLifetime lifetime{ ModelLifetime::Dynamic };
};


namespace VK_INIT_ENGINE {
    struct _inited_engine{
        struct SDL_Window* _window{ nullptr };
        VkExtent2D _windowExtent;
        VkInstance _instance;
        VkDebugUtilsMessengerEXT _debug_messenger;
        VkPhysicalDevice _chosenGPU;
        VkPhysicalDeviceFeatures _deviceFeatures;
        VkDevice _device;
        VkSurfaceKHR _surface;


        VkQueue _graphicsQueue;
        uint32_t _graphicsQueueFamily;

        VkQueue _computeQueue;
        uint32_t _computeQueueFamily;

        VmaAllocator _allocator;

        // swapChain - Буфер кадра
        VkSwapchainKHR _swapchain; // Физическое обьявление SwapChain
        VkFormat _swapchainImageFormat; // Формат/Инструкция как работает SwapChain

        std::vector<VkImage> _swapchainImages; // Сырые изображения (просто байтовые комбинации) к примеру: 0, 1, 2 (Тройная буферизация)
        std::vector<VkImageView> _swapchainImageViews; // Инструкция к каждому кадру (Сырой картинки из swapChainImages)
        VkExtent2D _swapchainExtent;

        // Холст цветной куда шейдеры выводят изображение
        AllocatedImage _drawImage;
        // Буфер глубины
        AllocatedImage _depthImage;
        // Буфер движения
        AllocatedImage _velocityImage;
        // Буфер нормалей
        AllocatedImage _normalImage;
        // Буфер истории кадров
        AllocatedImage _historyImages[2];

        // Набор для передачи данных в шейдер (Для загрузки glTF)
        // for immediate_submit
        VkFence _immFence;
        VkCommandBuffer _immCommandBuffer;
        VkCommandPool _immCommandPool;

        // Базовый набор для renderLoop
        std::vector<VkFence> _renderFence;
        std::vector<VkSemaphore> _swapchainSemaphores;
        std::vector<VkSemaphore> _renderSemaphores;


        bool _isInitialized = false;
    };
}

enum class RenderPassType : uint8_t {
    Forward,        // Base render
    Grid,           // Editor grid
    ShadowCSM,      // SCM
    Skybox,         // Sky
    Compute
};

enum class ComputePassType : uint8_t{
    IBL,
    TonMapping,
    ColorCorrection,
    TAA,
    GTAO,
    BLOOM
};

enum class PipelineOpacity{
    Opaque,
    AlphaTested,
    Transparent
};