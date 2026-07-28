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
#include <bit>

#include "imgui.h"
#include "imgui_impl_sdl3.h"
#include "imgui_impl_vulkan.h"

#include "transcoder/basisu_transcoder.h"
#include "../Controller/movement.hpp"

#define VK_CHECK(x)                                                     \
    do {                                                                \
        VkResult err = x;                                               \
        if (err) {                                                      \
            fmt::print("Detected Vulkan error: {}", string_VkResult(err)); \
            abort();                                                    \
        }                                                                      \
    } while (0)

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