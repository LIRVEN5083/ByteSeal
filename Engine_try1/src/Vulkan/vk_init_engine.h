#pragma once
#include <vk_types.h>
#include "VkBootstrap.h"
#include <vk_initializers.h>

#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>

#include "imgui.h"
#include "imgui_impl_sdl3.h"
#include "imgui_impl_vulkan.h"

#include <chrono>
#include <thread>

constexpr unsigned int FRAME_OVERLAP = 2;

struct AllocatedImage {
    VkImage image;
    VkImageView imageView;
    VmaAllocation allocation;
    VkExtent3D imageExtent;
    VkFormat imageFormat;
};

namespace VK_INIT_ENGINE{
    struct _inited_engine{
        struct SDL_Window* _window{ nullptr };
        VkExtent2D _windowExtent;
        VkInstance _instance;
        VkDebugUtilsMessengerEXT _debug_messenger;
        VkPhysicalDevice _chosenGPU;
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

        void immediate_submit(std::function<void(VkCommandBuffer cmd)>&& function);
    };

    class VulkanInitEngine{
    public:
        _inited_engine& Get();

        VulkanInitEngine(bool Validation_layers = true);
        void init_cleanup();
    private:
        VkExtent2D applicationSize{800, 500};
        _inited_engine ready_init;
        void create_swapchain(uint32_t width, uint32_t height);
        void init_swapchain();
        void init_sync_structures();
    };
}
