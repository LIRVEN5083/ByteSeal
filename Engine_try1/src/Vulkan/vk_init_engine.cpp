#include "vk_init_engine.h"

#define VMA_IMPLEMENTATION
#include "vk_mem_alloc.h"

void VK_INIT_ENGINE::VulkanInitEngine::init_cleanup(){
    // Ожидание
    if (ready_init._device != VK_NULL_HANDLE) {
        vkDeviceWaitIdle(ready_init._device);
    }

    ImGui_ImplVulkan_Shutdown();
    vkDestroyDescriptorPool(ready_init._device, imguiPool, nullptr);

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

    if (ready_init._allocator != VK_NULL_HANDLE) {
        // Цвет 1х
        if (ready_init._drawImage.imageView != VK_NULL_HANDLE) {
            vkDestroyImageView(ready_init._device, ready_init._drawImage.imageView, nullptr);
            ready_init._drawImage.imageView = VK_NULL_HANDLE;
        }
        if (ready_init._drawImage.image != VK_NULL_HANDLE) {
            vmaDestroyImage(ready_init._allocator, ready_init._drawImage.image, ready_init._drawImage.allocation);
            ready_init._drawImage.image = VK_NULL_HANDLE;
            ready_init._drawImage.allocation = VK_NULL_HANDLE;
        }

        // MSAA
        if (ready_init._msaaColorImage.imageView != VK_NULL_HANDLE) {
            vkDestroyImageView(ready_init._device, ready_init._msaaColorImage.imageView, nullptr);
            ready_init._msaaColorImage.imageView = VK_NULL_HANDLE;
        }
        if (ready_init._msaaColorImage.image != VK_NULL_HANDLE) {
            vmaDestroyImage(ready_init._allocator, ready_init._msaaColorImage.image, ready_init._msaaColorImage.allocation);
            ready_init._msaaColorImage.image = VK_NULL_HANDLE;
            ready_init._msaaColorImage.allocation = VK_NULL_HANDLE;
        }

        if (ready_init._msaaDepthImage.imageView != VK_NULL_HANDLE) {
            vkDestroyImageView(ready_init._device, ready_init._msaaDepthImage.imageView, nullptr);
            ready_init._msaaDepthImage.imageView = VK_NULL_HANDLE;
        }
        if (ready_init._msaaDepthImage.image != VK_NULL_HANDLE) {
            vmaDestroyImage(ready_init._allocator, ready_init._msaaDepthImage.image, ready_init._msaaDepthImage.allocation);
            ready_init._msaaDepthImage.image = VK_NULL_HANDLE;
            ready_init._msaaDepthImage.allocation = VK_NULL_HANDLE;
        }
    }

    for (auto imageView : ready_init._swapchainImageViews) {
        if (imageView != VK_NULL_HANDLE) {
            vkDestroyImageView(ready_init._device, imageView, nullptr);
        }
    }
    ready_init._swapchainImageViews.clear();
    ready_init._swapchainImages.clear();

    if (ready_init._swapchain != VK_NULL_HANDLE) {
        vkDestroySwapchainKHR(ready_init._device, ready_init._swapchain, nullptr);
        ready_init._swapchain = VK_NULL_HANDLE;
    }

    // Базовая чистка систем Vulkan
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
    NFD::Quit();
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
        .set_desired_present_mode(VK_PRESENT_MODE_MAILBOX_KHR)
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

    VkExtent3D drawImageExtent = {
        ready_init._windowExtent.width,
        ready_init._windowExtent.height,
        1
    };

    // 1. Получаем поддерживаемое видеокартой количество сэмплов для MSAA
    VkSampleCountFlagBits msaaSamples = vkinit::max_samples(ready_init);

    // ==========================================
    // Обычная плоская картинка (_drawImage, 1 sample)
    // ==========================================
    ready_init._drawImage.imageFormat = VK_FORMAT_R16G16B16A16_SFLOAT;
    ready_init._drawImage.imageExtent = drawImageExtent;

    VkImageUsageFlags drawImageUsages{};
    drawImageUsages |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    drawImageUsages |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    drawImageUsages |= VK_IMAGE_USAGE_STORAGE_BIT;
    drawImageUsages |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT; // Оставляем, так как сюда сбрасывается Resolve и рисуется UI

    VkImageCreateInfo rimg_info = vkinit::image_create_info(ready_init._drawImage.imageFormat, drawImageUsages, drawImageExtent);
    // Для обычной картинки сэмплы всегда равны 1
    rimg_info.samples = VK_SAMPLE_COUNT_1_BIT;

    VmaAllocationCreateInfo rimg_allocinfo = {};
    rimg_allocinfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
    rimg_allocinfo.requiredFlags = VkMemoryPropertyFlags(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    vmaCreateImage(ready_init._allocator, &rimg_info, &rimg_allocinfo, &ready_init._drawImage.image, &ready_init._drawImage.allocation, nullptr);

    VkImageViewCreateInfo rview_info = vkinit::imageview_create_info(ready_init._drawImage.imageFormat, ready_init._drawImage.image, VK_IMAGE_ASPECT_COLOR_BIT);
    VK_CHECK(vkCreateImageView(ready_init._device, &rview_info, nullptr, &ready_init._drawImage.imageView));


    // Новая многовыборочная цветная картинка (_msaaColorImage)

    ready_init._msaaColorImage.imageFormat = ready_init._drawImage.imageFormat; // Формат совпадает с основным холстом
    ready_init._msaaColorImage.imageExtent = drawImageExtent;

    VkImageUsageFlags msaaColorUsages{};
    msaaColorUsages |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT; // Используем как таргет для 3D сцены
    msaaColorUsages |= VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT; // Оптимизация GPU: память может не выделяться на диске физически, она временная

    VkImageCreateInfo msaa_img_info = vkinit::image_create_info(ready_init._msaaColorImage.imageFormat, msaaColorUsages, drawImageExtent);
    msaa_img_info.samples = msaaSamples;

    // Выделяем память
    vmaCreateImage(ready_init._allocator, &msaa_img_info, &rimg_allocinfo, &ready_init._msaaColorImage.image, &ready_init._msaaColorImage.allocation, nullptr);

    VkImageViewCreateInfo msaa_view_info = vkinit::imageview_create_info(ready_init._msaaColorImage.imageFormat, ready_init._msaaColorImage.image, VK_IMAGE_ASPECT_COLOR_BIT);
    VK_CHECK(vkCreateImageView(ready_init._device, &msaa_view_info, nullptr, &ready_init._msaaColorImage.imageView));

    // Новая многовыборочная глубина (_msaaDepthImage)

    ready_init._msaaDepthImage.imageFormat = VK_FORMAT_D32_SFLOAT;
    ready_init._msaaDepthImage.imageExtent = drawImageExtent;

    VkImageUsageFlags depthImageUsages{};
    depthImageUsages |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    depthImageUsages |= VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT;

    VkImageCreateInfo dimg_info = vkinit::image_create_info(ready_init._msaaDepthImage.imageFormat, depthImageUsages, drawImageExtent);
    dimg_info.samples = msaaSamples;

    vmaCreateImage(ready_init._allocator, &dimg_info, &rimg_allocinfo, &ready_init._msaaDepthImage.image, &ready_init._msaaDepthImage.allocation, nullptr);

    VkImageViewCreateInfo dview_info = vkinit::imageview_create_info(ready_init._msaaDepthImage.imageFormat, ready_init._msaaDepthImage.image, VK_IMAGE_ASPECT_DEPTH_BIT);
    VK_CHECK(vkCreateImageView(ready_init._device, &dview_info, nullptr, &ready_init._msaaDepthImage.imageView));
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

void VK_INIT_ENGINE::VulkanInitEngine::init_imgui(){
    // 1: Создаём пул дескрипторов для IMGUI
    //  the size of the pool is very oversize, but it's copied from imgui demo
    //  itself.
    VkDescriptorPoolSize pool_sizes[] = { { VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
        { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
        { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
        { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
        { VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
        { VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
        { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
        { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
        { VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 } };

    VkDescriptorPoolCreateInfo pool_info = {};
    pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    pool_info.maxSets = 1000;
    pool_info.poolSizeCount = (uint32_t)std::size(pool_sizes);
    pool_info.pPoolSizes = pool_sizes;


    VK_CHECK(vkCreateDescriptorPool(ready_init._device, &pool_info, nullptr, &imguiPool));

    // 2: initialize imgui library

    // this initializes the core structures of imgui
    ImGui::CreateContext();

    VK_GUI::apply_theme();

    // Инициация IMGUI под SDL3 и при этом это чудо само подтянет текстуры шрифтов
    ImGui_ImplSDL3_InitForVulkan(ready_init._window);

    // this initializes imgui for Vulkan
    ImGui_ImplVulkan_InitInfo init_info = {};
    init_info.Instance = ready_init._instance;
    init_info.PhysicalDevice = ready_init._chosenGPU;
    init_info.Device = ready_init._device;
    init_info.Queue = ready_init._graphicsQueue;
    init_info.DescriptorPool = imguiPool;
    init_info.MinImageCount = 3;
    init_info.ImageCount = 3;
    init_info.UseDynamicRendering = true;

    // Это для динамического рендера и трабл в том что в для SDL3 теперь нужно писать через PipelineInfoMain.<чота там>
    init_info.PipelineInfoMain.PipelineRenderingCreateInfo = { .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO };
    init_info.PipelineInfoMain.PipelineRenderingCreateInfo.colorAttachmentCount = 1;
    VkFormat drawImageFormat = ready_init._drawImage.imageFormat;
    init_info.PipelineInfoMain.PipelineRenderingCreateInfo.pColorAttachmentFormats = &drawImageFormat;
    init_info.PipelineInfoMain.PipelineRenderingCreateInfo.depthAttachmentFormat = VK_FORMAT_UNDEFINED;

    init_info.PipelineInfoMain.MSAASamples = VK_SAMPLE_COUNT_1_BIT;

    ImGui_ImplVulkan_Init(&init_info);
}

VK_INIT_ENGINE::_inited_engine& VK_INIT_ENGINE::VulkanInitEngine::Get(){
    return this->ready_init;
}

VK_INIT_ENGINE::VulkanInitEngine::VulkanInitEngine(bool Validation_layers){
    this->ready_init._windowExtent = applicationSize;
    SDL_Init(SDL_INIT_VIDEO);
    SDL_WindowFlags window_flags = (SDL_WindowFlags)(SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY);
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
    features.shaderDemoteToHelperInvocation = VK_TRUE;

    VkPhysicalDeviceVulkan12Features features12{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES };
    features12.descriptorBindingSampledImageUpdateAfterBind = VK_TRUE;
    features12.descriptorBindingPartiallyBound = VK_TRUE;
    features12.runtimeDescriptorArray = VK_TRUE;
    features12.bufferDeviceAddress = VK_TRUE;
    features12.shaderOutputLayer = VK_TRUE;
    features12.shaderOutputViewportIndex = VK_TRUE;

    VkPhysicalDeviceFeatures baseFeatures{};
    baseFeatures.samplerAnisotropy = VK_TRUE;
    baseFeatures.geometryShader = VK_TRUE;

    vkb::PhysicalDeviceSelector selector{ vkb_inst };
    vkb::PhysicalDevice physicalDevice = selector
    .set_minimum_version(1, 3)
    .set_required_features(baseFeatures)
    .set_required_features_13(features)
    .set_required_features_12(features12)
    .set_surface(this->ready_init._surface)
    .add_required_extension(VK_EXT_SHADER_VIEWPORT_INDEX_LAYER_EXTENSION_NAME)
    .select()
    .value();

    vkb::DeviceBuilder deviceBuilder{ physicalDevice };
    vkb::Device vkbDevice = deviceBuilder.build().value();

    this->ready_init._device = vkbDevice.device;
    this->ready_init._chosenGPU = physicalDevice.physical_device;

    this->ready_init._graphicsQueue = vkbDevice.get_queue(vkb::QueueType::graphics).value();
    this->ready_init._graphicsQueueFamily = vkbDevice.get_queue_index(vkb::QueueType::graphics).value();

    this->ready_init._computeQueue = vkbDevice.get_queue(vkb::QueueType::compute).value();
    this->ready_init._computeQueueFamily = vkbDevice.get_queue_index(vkb::QueueType::compute).value();

    vkGetPhysicalDeviceFeatures(ready_init._chosenGPU, &ready_init._deviceFeatures);

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

    if (NFD::Init() != NFD_OKAY) {
        std::cerr << "Failed to init NFD: " << NFD::GetError() << std::endl;
    }

    init_swapchain();

    init_sync_structures();

    init_imgui();

    this->ready_init._isInitialized = true;
}
