#include "vk_init_engine.h"

#define VMA_IMPLEMENTATION
#include "vk_mem_alloc.h"

void VK_INIT_ENGINE::VulkanInitEngine::init_cleanup(){
    if (ready_init._allocator != VK_NULL_HANDLE) {
        vmaDestroyAllocator(ready_init._allocator);
    }
    if (ready_init._device != VK_NULL_HANDLE) {
        vkDestroyDevice(ready_init._device, nullptr);
    }
    if (ready_init._surface != VK_NULL_HANDLE) {
        vkDestroySurfaceKHR(ready_init._instance, ready_init._surface, nullptr);
    }
    if (ready_init._debug_messenger != VK_NULL_HANDLE) {
        vkb::destroy_debug_utils_messenger(ready_init._instance, ready_init._debug_messenger);
    }
    if (ready_init._instance != VK_NULL_HANDLE) {
        vkDestroyInstance(ready_init._instance, nullptr);
    }

    if (ready_init._window != nullptr) {
        SDL_DestroyWindow(ready_init._window);
    }
    SDL_Quit();
}

VK_INIT_ENGINE::_inited_engine& VK_INIT_ENGINE::VulkanInitEngine::Get(){
    return this->ready_init;
}

VK_INIT_ENGINE::VulkanInitEngine::VulkanInitEngine(bool Validation_layers){
    this->ready_init._windowExtent = applicationSize;
    SDL_Init(SDL_INIT_VIDEO);
    SDL_WindowFlags window_flags = (SDL_WindowFlags)(SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE);
    ready_init._window = SDL_CreateWindow(
        "ByteSeal Engine",
        this->ready_init._windowExtent.width, this->ready_init._windowExtent.height,
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

    this->ready_init._isInitialized = true;
}
