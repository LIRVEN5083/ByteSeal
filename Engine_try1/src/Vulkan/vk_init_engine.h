#pragma once
#include <vk_types.h>
#include "VkBootstrap.h"

#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>

#include "imgui.h"
#include "imgui_impl_sdl3.h"
#include "imgui_impl_vulkan.h"

#include <chrono>
#include <thread>

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
        bool _isInitialized = false;
    };

    class VulkanInitEngine{
    public:
        _inited_engine& Get();

        VulkanInitEngine(bool Validation_layers = true);
        void init_cleanup();
    private:
        VkExtent2D applicationSize{800, 500};
        _inited_engine ready_init;
    };
}
