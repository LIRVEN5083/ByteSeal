#pragma once
#include "vk_types.h"
#include "VkBootstrap.h"
#include "vk_initializers.h"

#include <chrono>
#include <thread>

constexpr unsigned int FRAME_OVERLAP = 2;

namespace VK_GUI{
    void apply_theme();
}

namespace VK_INIT_ENGINE{

    class VulkanInitEngine{
    public:
        _inited_engine& Get();

        VulkanInitEngine(bool Validation_layers = true);
        void init_cleanup();

    private:
        VkExtent2D applicationSize{1200, 800};
        _inited_engine ready_init;
        void create_swapchain(uint32_t width, uint32_t height);
        void init_swapchain();
        void init_sync_structures();

        VkDescriptorPool imguiPool;
        void init_imgui();
    };
}
