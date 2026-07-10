#include "vk_init_engine.h"

#define VMA_IMPLEMENTATION
#include "vk_mem_alloc.h"

void VK_INIT_ENGINE::VulkanInitEngine::init_cleanup(){
    // Ожидание
    if (ready_init._device != VK_NULL_HANDLE) {
        vkDeviceWaitIdle(ready_init._device);
    }

    for (uint32_t i = 0; i < FRAME_OVERLAP; i++) {
        vkDestroyFence(ready_init._device, ready_init._renderFence[i], nullptr);
    }

    for (uint32_t i = 0; i < static_cast<uint32_t>(ready_init._swapchainImages.size()); i++) {
        vkDestroySemaphore(ready_init._device, ready_init._swapchainSemaphores[i], nullptr);
        vkDestroySemaphore(ready_init._device, ready_init._renderSemaphores[i], nullptr);
    }

    if (ready_init._immFence != VK_NULL_HANDLE){
        vkDestroyFence(ready_init._device, ready_init._immFence, nullptr);
    }
    if (ready_init._immCommandPool != VK_NULL_HANDLE){
        vkDestroyCommandPool(ready_init._device, ready_init._immCommandPool, nullptr);
    }

    // Очистка swapchain
    if (ready_init._allocator != VK_NULL_HANDLE) {
        if (ready_init._drawImage.imageView != VK_NULL_HANDLE) {
            vkDestroyImageView(ready_init._device, ready_init._drawImage.imageView, nullptr);
        }
        if (ready_init._depthImage.imageView != VK_NULL_HANDLE) {
            vkDestroyImageView(ready_init._device, ready_init._depthImage.imageView, nullptr);
        }
        if (ready_init._drawImage.image != VK_NULL_HANDLE) {
            vmaDestroyImage(ready_init._allocator, ready_init._drawImage.image, ready_init._drawImage.allocation);
        }
        if (ready_init._depthImage.image != VK_NULL_HANDLE) {
            vmaDestroyImage(ready_init._allocator, ready_init._depthImage.image, ready_init._depthImage.allocation);
        }
    }

    for (auto imageView : ready_init._swapchainImageViews) {
        if (imageView != VK_NULL_HANDLE) {
            vkDestroyImageView(ready_init._device, imageView, nullptr);
        }
    }

    ready_init._swapchainImageViews.clear();
    ready_init._swapchainImages.clear();

    // Уничтожаем сам Swapchain
    if (ready_init._swapchain != VK_NULL_HANDLE) {
        vkDestroySwapchainKHR(ready_init._device, ready_init._swapchain, nullptr);
    }


    // Базовая чистка
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

void VK_INIT_ENGINE::VulkanInitEngine::create_swapchain(uint32_t width, uint32_t height){
    // Создаём структуру для создания swapChain (VkPhysicalDevice, VkLogicalDevice, VkSurface)
    vkb::SwapchainBuilder swapchainBuilder{ ready_init._chosenGPU, ready_init._device, ready_init._surface };

    // Это тупа формат свап чейна, лол
    ready_init._swapchainImageFormat = VK_FORMAT_B8G8R8A8_UNORM;

    vkb::Swapchain vkbSwapchain = swapchainBuilder
        // .use_default_format_selection() - типо хер его, просто заполнение структуры на формат SwapChain
        .set_desired_format(VkSurfaceFormatKHR{ .format = ready_init._swapchainImageFormat, .colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR })
        // use vsync present mode - флажок FIFO_KHR - означает у нас будет крайне жёсткая верт.синхронизация
        .set_desired_present_mode(VK_PRESENT_MODE_FIFO_KHR)
        // Это размер буфера. Ну тип можно поставить FULL HD но оно же тип будет образаться и растягиваться от размера окна
        .set_desired_extent(width, height)
        // DST_BIT - картинка может быть приемником для копирования. SRC_BIT - картинка может быть источником для копирования
        .add_image_usage_flags(VK_IMAGE_USAGE_TRANSFER_DST_BIT)
        .build()
        .value();

    // Реальному полю вулкан мы присваеваем херню которую писали выше .set_desired_extent(width, height)
    ready_init._swapchainExtent = vkbSwapchain.extent;
    // Реальное поле со свап чейн теперь есть и заполнено темой сверху
    ready_init._swapchain = vkbSwapchain.swapchain;
    // Просто инициация массива с картинками
    ready_init._swapchainImages = vkbSwapchain.get_images().value();
    // Просто инициация массива обёрток на массив с картинками
    ready_init._swapchainImageViews = vkbSwapchain.get_image_views().value();
}

void VK_INIT_ENGINE::VulkanInitEngine::init_swapchain(){
    create_swapchain(ready_init._windowExtent.width, ready_init._windowExtent.height);
    //draw image size will match the window
    VkExtent3D drawImageExtent = {
        ready_init._windowExtent.width,
        ready_init._windowExtent.height,
        1
    };

    //hardcoding the draw format to 32 bit float
    ready_init._drawImage.imageFormat = VK_FORMAT_R16G16B16A16_SFLOAT;
    ready_init._drawImage.imageExtent = drawImageExtent;

    VkImageUsageFlags drawImageUsages{};
    drawImageUsages |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    drawImageUsages |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    drawImageUsages |= VK_IMAGE_USAGE_STORAGE_BIT;
    drawImageUsages |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    VkImageCreateInfo rimg_info = vkinit::image_create_info(ready_init._drawImage.imageFormat, drawImageUsages, drawImageExtent);

    //for the draw image, we want to allocate it from gpu local memory
    VmaAllocationCreateInfo rimg_allocinfo = {};
    rimg_allocinfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
    rimg_allocinfo.requiredFlags = VkMemoryPropertyFlags(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    //allocate and create the image
    vmaCreateImage(ready_init._allocator, &rimg_info, &rimg_allocinfo, &ready_init._drawImage.image, &ready_init._drawImage.allocation, nullptr);

    //build a image-view for the draw image to use for rendering
    VkImageViewCreateInfo rview_info = vkinit::imageview_create_info(ready_init._drawImage.imageFormat, ready_init._drawImage.image, VK_IMAGE_ASPECT_COLOR_BIT);

    VK_CHECK(vkCreateImageView(ready_init._device, &rview_info, nullptr, &ready_init._drawImage.imageView));

    ready_init._depthImage.imageFormat = VK_FORMAT_D32_SFLOAT;
    ready_init._depthImage.imageExtent = drawImageExtent;
    VkImageUsageFlags depthImageUsages{};
    depthImageUsages |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;

    VkImageCreateInfo dimg_info = vkinit::image_create_info(ready_init._depthImage.imageFormat, depthImageUsages, drawImageExtent);

    //allocate and create the image
    vmaCreateImage(ready_init._allocator, &dimg_info, &rimg_allocinfo, &ready_init._depthImage.image, &ready_init._depthImage.allocation, nullptr);

    //build a image-view for the draw image to use for rendering
    VkImageViewCreateInfo dview_info = vkinit::imageview_create_info(ready_init._depthImage.imageFormat, ready_init._depthImage.image, VK_IMAGE_ASPECT_DEPTH_BIT);

    VK_CHECK(vkCreateImageView(ready_init._device, &dview_info, nullptr, &ready_init._depthImage.imageView));
}

void VK_INIT_ENGINE::VulkanInitEngine::init_sync_structures(){
    VkFenceCreateInfo fenceCreateInfo = vkinit::fence_create_info(VK_FENCE_CREATE_SIGNALED_BIT);
    VkSemaphoreCreateInfo semaphoreCreateInfo = vkinit::semaphore_create_info(0);

    uint32_t swapchainImageCount = static_cast<uint32_t>(ready_init._swapchainImages.size());
    ready_init._swapchainSemaphores.resize(swapchainImageCount);
    ready_init._renderSemaphores.resize(swapchainImageCount);
    ready_init._renderFence.resize(FRAME_OVERLAP);


    for (uint32_t i = 0; i < FRAME_OVERLAP; i++) {
        VK_CHECK(vkCreateFence(ready_init._device, &fenceCreateInfo, nullptr, &ready_init._renderFence[i]));
    }

    for (uint32_t i = 0; i < swapchainImageCount; i++) {
        VK_CHECK(vkCreateSemaphore(ready_init._device, &semaphoreCreateInfo, nullptr, &ready_init._swapchainSemaphores[i]));
        VK_CHECK(vkCreateSemaphore(ready_init._device, &semaphoreCreateInfo, nullptr, &ready_init._renderSemaphores[i]));
    }

    // Создать Fence для одноразовых команд загрузки ресурсов
    VK_CHECK(vkCreateFence(ready_init._device, &fenceCreateInfo, nullptr, &ready_init._immFence));

}

void VK_INIT_ENGINE::_inited_engine::immediate_submit(std::function<void(VkCommandBuffer cmd)>&& function){
    VK_CHECK(vkResetFences(_device, 1, &_immFence));
    VK_CHECK(vkResetCommandBuffer(_immCommandBuffer, 0));

    VkCommandBuffer cmd = _immCommandBuffer;

    VkCommandBufferBeginInfo cmdBeginInfo = vkinit::command_buffer_begin_info(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

    VK_CHECK(vkBeginCommandBuffer(cmd, &cmdBeginInfo));

    function(cmd);

    VK_CHECK(vkEndCommandBuffer(cmd));

    VkCommandBufferSubmitInfo cmdinfo = vkinit::command_buffer_submit_info(cmd);
    VkSubmitInfo2 submit = vkinit::submit_info(&cmdinfo, nullptr, nullptr);

    // submit command buffer to the queue and execute it.
    //  _renderFence will now block until the graphic commands finish execution
    VK_CHECK(vkQueueSubmit2(_graphicsQueue, 1, &submit, _immFence));

    VK_CHECK(vkWaitForFences(_device, 1, &_immFence, true, 9999999999));
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

    // Создание структур для immidiate_submit
    VkCommandPoolCreateInfo commandPoolInfo = vkinit::command_pool_create_info(ready_init._graphicsQueueFamily, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
    VK_CHECK(vkCreateCommandPool(ready_init._device, &commandPoolInfo, nullptr, &ready_init._immCommandPool));

    // allocate the command buffer for immediate submits
    VkCommandBufferAllocateInfo cmdAllocInfo = vkinit::command_buffer_allocate_info(ready_init._immCommandPool, 1);

    VK_CHECK(vkAllocateCommandBuffers(ready_init._device, &cmdAllocInfo, &ready_init._immCommandBuffer));

    init_swapchain();

    init_sync_structures();

    this->ready_init._isInitialized = true;
}
