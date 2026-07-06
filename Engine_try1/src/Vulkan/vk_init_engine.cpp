#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#include "vk_init_engine.h"

#define VMA_IMPLEMENTATION
#include "vk_mem_alloc.h"

#include "VkBootstrap.h"

#include "imgui.h"
#include "imgui_impl_sdl3.h"
#include "imgui_impl_vulkan.h"

#include <chrono>
#include <thread>


_inited_engine& VK_INIT_ENGINE::VulkanInitEngine::Get(){
    return this->ready_init;
}

VK_INIT_ENGINE::VulkanInitEngine::VulkanInitEngine(bool Validation_layers){
    SDL_Init(SDL_INIT_VIDEO);
    SDL_WindowFlags window_flags = (SDL_WindowFlags)(SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE);
    ready_init._window = SDL_CreateWindow(
        "ByteSeal Engine",
        _windowExtent.width, _windowExtent.height,
        window_flags
    );

    vkb::InstanceBuilder builder;

    auto inst_ret = builder.set_app_name("ByteSeal_Instance")
    .request_validation_layers(Validation_layers)
    .use_default_debug_messenger()
    .require_api_version(1, 3, 0)
    .build();

    vkb::Instance vkb_inst = inst_ret.value();

    this->ready_init._instance = vkb_inst.instance;
    this->ready_init._debug_messenger = vkb_inst.debug_messenger;

    SDL_Vulkan_CreateSurface(ready_init._window, this->ready_init._instance, nullptr, &this->ready_init._surface);

    VkPhysicalDeviceVulkan13Features features{ .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES };
    features.dynamicRendering = true;
    features.synchronization2 = true;

    VkPhysicalDeviceVulkan12Features features12{ .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES };
    features12.bufferDeviceAddress = true;
    features12.descriptorIndexing = true;

    vkb::PhysicalDeviceSelector selector{ vkb_inst };
    vkb::PhysicalDevice physicalDevice = selector
    .set_minimum_version(1, 3)
    .set_required_features_13(features)
    .set_required_features_12(features12)
    .set_surface(this->ready_init._surface)
    .select()
    .value();

    vkb::DeviceBuilder deviceBuilder{ physicalDevice };
    vkb::Device vkbDevice = deviceBuilder.build().value();

    this->ready_init._device = vkbDevice.device;
    this->ready_init._chosenGPU = physicalDevice.physical_device;

    this->ready_init._graphicsQueue = vkbDevice.get_queue(vkb::QueueType::graphics).value();

    this->ready_init._graphicsQueueFamily = vkbDevice.get_queue_index(vkb::QueueType::graphics).value();

    VmaAllocatorCreateInfo allocatorInfo = {};
    allocatorInfo.physicalDevice = this->ready_init._chosenGPU;
    allocatorInfo.device = this->ready_init._device;
    allocatorInfo.instance = this->ready_init._instance;

    allocatorInfo.flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;
    vmaCreateAllocator(&allocatorInfo, &this->ready_init._allocator);
}

VK_INIT_ENGINE::VulkanInitEngine::~VulkanInitEngine() {
    if (this->ready_init._allocator != VK_NULL_HANDLE) {
        vmaDestroyAllocator(this->ready_init._allocator);
    }
    if (this->ready_init._device != VK_NULL_HANDLE) {
        vkDestroyDevice(this->ready_init._device, nullptr);
    }
    if (this->ready_init._surface != VK_NULL_HANDLE) {
        vkDestroySurfaceKHR(this->ready_init._instance, this->ready_init._surface, nullptr);
    }
    if (this->ready_init._debug_messenger != VK_NULL_HANDLE) {
        vkb::destroy_debug_utils_messenger(this->ready_init._instance, this->ready_init._debug_messenger);
    }

    if (this->ready_init._instance != VK_NULL_HANDLE) {
        vkDestroyInstance(this->ready_init._instance, nullptr);
    }

    if (this->ready_init._window != nullptr) {
        SDL_DestroyWindow(this->ready_init._window);
    }
    SDL_Quit();
}
