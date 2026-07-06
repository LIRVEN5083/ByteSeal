#pragma once
#include <vk_types.h>

struct _inited_engine{
    struct SDL_Window* _window{ nullptr };
    VkInstance _instance;
    VkDebugUtilsMessengerEXT _debug_messenger;
    VkPhysicalDevice _chosenGPU;
    VkDevice _device;
    VkSurfaceKHR _surface;
    VkQueue _graphicsQueue;
    uint32_t _graphicsQueueFamily;
    VmaAllocator _allocator;
};
namespace VK_INIT_ENGINE{
    class VulkanInitEngine{
    public:
        VkExtent2D _windowExtent{ 800, 500 };
        _inited_engine& Get();

        VulkanInitEngine(bool Validation_layers = true);
        ~VulkanInitEngine();
    private:
        _inited_engine ready_init;
    };
}
