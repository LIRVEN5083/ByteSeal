#include "vk_application.h"

#include <iostream>

VK_APPLICATION::VulkanApplication::VulkanApplication(VK_INIT_ENGINE::_inited_engine& inited_engine)
: _init(inited_engine){}

void VK_APPLICATION::VulkanApplication::cleanup(){
    if (_init._isInitialized){
        vkDeviceWaitIdle(_init._device);
    }

    for (int i = 0; i < FRAME_OVERLAP; i++) {
        _frames[i]._deletionQueue.flush();
    }

    _lightManager->cleanUp();
    _pipelineManager->cleanup();
    _activeScene->DestroyAllEntites();
    _modelManager.destroy_all();
    _meshManager.DestroyAllocationData();
    _textureManager.DestroyAllocationData();

    for (int i = 0; i < FRAME_OVERLAP; i++) {
        vkinit::destroy_buffer(_frames[i].gpuSceneDataBuffer, _init._allocator);

        _frames[i]._frameDescriptors.destroy_pools(_init._device);

        _frames[i].sceneDescriptorSet = VK_NULL_HANDLE;
    }

    if (_sceneDescriptorPool != VK_NULL_HANDLE) {
        vkDestroyDescriptorPool(_init._device, _sceneDescriptorPool, nullptr);
        _sceneDescriptorPool = VK_NULL_HANDLE;
    }
    if (_gpuSceneDataDescriptorLayout != VK_NULL_HANDLE) {
        vkDestroyDescriptorSetLayout(_init._device, _gpuSceneDataDescriptorLayout, nullptr);
        _gpuSceneDataDescriptorLayout = VK_NULL_HANDLE;
    }

    for (int i = 0; i < FRAME_OVERLAP; i++) {
        if (_frames[i]._commandPool != VK_NULL_HANDLE) {
            vkDestroyCommandPool(_init._device, _frames[i]._commandPool, nullptr);
            _frames[i]._commandPool = VK_NULL_HANDLE;
            _frames[i]._mainCommandBuffer = VK_NULL_HANDLE;
        }
    }
}

void VK_APPLICATION::VulkanApplication::run(){
    init_commands();
    init_descriptors();
    _maxSamples = vkinit::max_samples(_init);
    init_scene();
    init_pipeline_manager();

    SDL_Event e;
    bool bQuit = false;

    _delta.lastFrameTime = std::chrono::high_resolution_clock::now();
    _delta.startTime = std::chrono::high_resolution_clock::now();

    while (!bQuit) {
        while (SDL_PollEvent(&e)) {
            ImGui_ImplSDL3_ProcessEvent(&e);

            if (e.type == SDL_EVENT_QUIT) {
                bQuit = true;
            }
            if (e.type == SDL_EVENT_WINDOW_RESIZED) {
                resize_requested = true;
            }
            if (e.type == SDL_EVENT_WINDOW_MINIMIZED) {
                stop_rendering = true;
            }
            if (e.type == SDL_EVENT_WINDOW_RESTORED) {
                stop_rendering = false;
            }
            if (e.type == SDL_EVENT_MOUSE_MOTION) {
                if (_camera.isCameraActive){
                    float xoffset = e.motion.xrel;
                    float yoffset = -e.motion.yrel; // Инвертируем Y

                    float sensitivity = 0.05f;
                    xoffset *= sensitivity;
                    yoffset *= sensitivity;

                    _camera.yaw   -= xoffset;
                    _camera.pitch += yoffset;

                    if (_camera.pitch > 89.0f)  _camera.pitch = 89.0f;
                    if (_camera.pitch < -89.0f) _camera.pitch = -89.0f;
                }
            }


            if (e.type == SDL_EVENT_KEY_DOWN) {
                if (e.key.key == SDLK_ESCAPE) {
                    bool isRealative = SDL_GetWindowRelativeMouseMode(_init._window);

                    if (isRealative){
                        SDL_SetWindowRelativeMouseMode(_init._window, false);
                        _camera.isCameraActive = false;

                        ImGui::GetIO().ConfigFlags &= ~ImGuiConfigFlags_NoMouse;
                    }
                    else{
                        SDL_SetWindowRelativeMouseMode(_init._window, true);
                        _camera.isCameraActive = true;

                        ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_NoMouse;
                    }
                }
            }
        }
        if (stop_rendering) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            continue;
        }

        if (resize_requested) {
            resize_swapchain();
        }
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        _gui.update_imgui(_init, _delta, _camera, _modelManager, _activeScene,  sceneData, *_pipelineManager, _renderSystem);
        CONTROLLER::update_time(_movement, _delta);
        renderLoop();
        CONTROLLER::made_move(_movement, _camera, _delta);
    }
}

void VK_APPLICATION::VulkanApplication::renderLoop(){
    if (resize_requested) return;
    const uint32_t frameId = _frameNumber % FRAME_OVERLAP;
    FrameData& currentFrame = _frames[frameId];

    // Ждём когда GPU прекратит рендерить прошлую картинку в течении 1 сек.
    VK_CHECK(vkWaitForFences(_init._device, 1, &_init._renderFence[frameId], true, 1000000000));
    VK_CHECK(vkResetFences(_init._device, 1, &_init._renderFence[frameId]));

    currentFrame._deletionQueue.flush();
    currentFrame._frameDescriptors.clear_pools(_init._device);

    VkDescriptorSet globalDescriptor = update_scene_data(currentFrame);

    // О май гад это же ImageIndex из Vk-tutorial
    uint32_t swapchainImageIndex;
    // Запрашиваем картинку из
    VkResult e = vkAcquireNextImageKHR(_init._device, _init._swapchain, 1000000000, _init._swapchainSemaphores[frameId], nullptr, &swapchainImageIndex);
    if (e == VK_ERROR_OUT_OF_DATE_KHR || e == VK_TIMEOUT) {
        resize_requested = true;
        return;
    }
    else if (e == VK_SUBOPTIMAL_KHR) {
        resize_requested = true;
    }
    else if (e != VK_SUCCESS) {
        VK_CHECK(e);
    }

    VkCommandBuffer cmd = currentFrame._mainCommandBuffer;

    // Мы теперь можем очищать командный буфер, чтобы опять его перезаписать
    VK_CHECK(vkResetCommandBuffer(cmd, 0));

    // Разрешение нашей картинки с которой мы будем работать
    _drawExtent.height = std::min(_init._swapchainExtent.height, _init._drawImage.imageExtent.height) * renderScale;
    _drawExtent.width= std::min(_init._swapchainExtent.width, _init._drawImage.imageExtent.width) * renderScale;

    VkCommandBufferBeginInfo cmdBeginInfo = vkinit::command_buffer_begin_info(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
    VK_CHECK(vkBeginCommandBuffer(cmd, &cmdBeginInfo));

    vkutil::transition_image(cmd, _init._msaaColorImage.image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
    vkutil::transition_image(cmd, _init._msaaDepthImage.image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL); // Наша 4х глубина
    vkutil::transition_image(cmd, _init._drawImage.image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL); // Наш 1х resolve-таргет

    // ПОДГОТОВКА ОЧЕРЕДИ
    _renderSystem.ClearQueue();

    // Сборка сцены
    glm::vec3 cameraPos = { _movement.valueX, _movement.valueY, _movement.valueZ };
    _activeScene->CullingAndSubmit(_renderSystem, *_pipelineManager, cameraPos);

    // Отрисовка RenderObject
    _renderSystem.PrepareFrame();
    VkDescriptorSet bindlessSet = _textureManager.GetTextureSet();
    _renderSystem.Draw(cmd, _drawExtent, globalDescriptor, bindlessSet, *_pipelineManager, *_lightManager);

    // Захардкоженный интерефейс
    _gui.draw_imgui(_init, cmd, _drawExtent);

    vkutil::transition_image(cmd, _init._drawImage.image, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
    // Переводим текущую картинку Swapchain в режим приемника копирования
    vkutil::transition_image(cmd, _init._swapchainImages[swapchainImageIndex], VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

    // Копируем (и масштабируем при необходимости) наш холст прямо в Swapchain
    vkutil::copy_image_to_image(cmd, _init._drawImage.image, _init._swapchainImages[swapchainImageIndex], _drawExtent, _init._swapchainExtent);

    // Переводим картинку Swapchain в финальное состояние для отображения на мониторе
    vkutil::transition_image(cmd, _init._swapchainImages[swapchainImageIndex], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

    // Закрываем командный буфер, запись завершена
    VK_CHECK(vkEndCommandBuffer(cmd));

    // ОТПРАВКА НА GPU (SUBMIT)
    VkCommandBufferSubmitInfo cmdSubmitInfo = vkinit::command_buffer_submit_info(cmd);

    // Синхронизируем семафоры: GPU ждет сигнала от Swapchain перед выгрузкой цвета
    VkSemaphoreSubmitInfo waitInfo = vkinit::semaphore_submit_info(VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT_KHR, _init._swapchainSemaphores[frameId]);
    // GPU сигналит в _renderSemaphores, когда полностью закончит блайтить пиксели
    VkSemaphoreSubmitInfo signalInfo = vkinit::semaphore_submit_info(VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT_KHR, _init._renderSemaphores[swapchainImageIndex]);

    VkSubmitInfo2 submit = vkinit::submit_info(&cmdSubmitInfo, &signalInfo, &waitInfo);

    // Отправляем буфер в очередь и передаем Fence из нашего ядра.
    // Когда GPU закончит этот кадр, Fence автоматически откроется
    VK_CHECK(vkQueueSubmit2(_init._graphicsQueue, 1, &submit, _init._renderFence[frameId]));

    // ВЫВОД НА ЭКРАН (PRESENT)
    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.pNext = nullptr;

    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = &_init._swapchain;

    // Монитор ждет семафор окончания рендеринга текущего кадра
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = &_init._renderSemaphores[swapchainImageIndex];
    presentInfo.pImageIndices = &swapchainImageIndex;

    VkResult presentResult = vkQueuePresentKHR(_init._graphicsQueue, &presentInfo);
    if (presentResult == VK_ERROR_OUT_OF_DATE_KHR) {
        resize_requested = true;
    }

    // Переходим к следующему кадру
    _frameNumber++;
}

void VK_APPLICATION::VulkanApplication::resize_swapchain(){
    vkDeviceWaitIdle(_init._device);

    int w, h;
    SDL_GetWindowSizeInPixels(_init._window, &w, &h);
    while (w == 0 || h == 0) {
        SDL_GetWindowSizeInPixels(_init._window, &w, &h);
        SDL_WaitEvent(nullptr);
    }

    _init._windowExtent.width = w;
    _init._windowExtent.height = h;

    destroy_swapchain();

    vkb::SwapchainBuilder swapchainBuilder{_init._chosenGPU, _init._device, _init._surface};
    auto vkbSwapchain = swapchainBuilder
        .set_desired_extent(_init._windowExtent.width, _init._windowExtent.height)
        .set_desired_format({_init._swapchainImageFormat, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR})
        .add_image_usage_flags(VK_IMAGE_USAGE_TRANSFER_DST_BIT)
        .build()
        .value();

    VkExtent3D drawImageExtent = { _init._windowExtent.width, _init._windowExtent.height, 1 };

    _init._swapchain = vkbSwapchain.swapchain;
    _init._swapchainImages = vkbSwapchain.get_images().value();
    _init._swapchainImageViews = vkbSwapchain.get_image_views().value();
    _init._swapchainExtent = vkbSwapchain.extent;

    VmaAllocationCreateInfo rimg_allocinfo = {};
    rimg_allocinfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
    rimg_allocinfo.requiredFlags = VkMemoryPropertyFlags(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    // Обычный плоский цвет
    _init._drawImage.imageFormat = VK_FORMAT_R16G16B16A16_SFLOAT;
    _init._drawImage.imageExtent = drawImageExtent;
    VkImageUsageFlags drawImageUsages = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    VkImageCreateInfo rimg_info = vkinit::image_create_info(_init._drawImage.imageFormat, drawImageUsages, drawImageExtent);
    rimg_info.samples = VK_SAMPLE_COUNT_1_BIT;
    vmaCreateImage(_init._allocator, &rimg_info, &rimg_allocinfo, &_init._drawImage.image, &_init._drawImage.allocation, nullptr);
    VkImageViewCreateInfo rview_info = vkinit::imageview_create_info(_init._drawImage.imageFormat, _init._drawImage.image, VK_IMAGE_ASPECT_COLOR_BIT);
    VK_CHECK(vkCreateImageView(_init._device, &rview_info, nullptr, &_init._drawImage.imageView));
    // Холст MSAA
    _init._msaaColorImage.imageFormat = _init._drawImage.imageFormat;
    _init._msaaColorImage.imageExtent = drawImageExtent;
    VkImageUsageFlags msaaColorUsages = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT;
    VkImageCreateInfo msaa_img_info = vkinit::image_create_info(_init._msaaColorImage.imageFormat, msaaColorUsages, drawImageExtent);
    msaa_img_info.samples = _maxSamples;
    vmaCreateImage(_init._allocator, &msaa_img_info, &rimg_allocinfo, &_init._msaaColorImage.image, &_init._msaaColorImage.allocation, nullptr);
    VkImageViewCreateInfo msaa_view_info = vkinit::imageview_create_info(_init._msaaColorImage.imageFormat, _init._msaaColorImage.image, VK_IMAGE_ASPECT_COLOR_BIT);
    VK_CHECK(vkCreateImageView(_init._device, &msaa_view_info, nullptr, &_init._msaaColorImage.imageView));
    // Буфер глубины MSAA
    _init._msaaDepthImage.imageFormat = VK_FORMAT_D32_SFLOAT;
    _init._msaaDepthImage.imageExtent = drawImageExtent;
    VkImageUsageFlags depthImageUsages = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT;
    VkImageCreateInfo dimg_info = vkinit::image_create_info(_init._msaaDepthImage.imageFormat, depthImageUsages, drawImageExtent);
    dimg_info.samples = _maxSamples;

    // Аллокация строго в переменные _msaaDepthImage
    vmaCreateImage(_init._allocator, &dimg_info, &rimg_allocinfo, &_init._msaaDepthImage.image, &_init._msaaDepthImage.allocation, nullptr);
    VkImageViewCreateInfo dview_info = vkinit::imageview_create_info(_init._msaaDepthImage.imageFormat, _init._msaaDepthImage.image, VK_IMAGE_ASPECT_DEPTH_BIT);
    VK_CHECK(vkCreateImageView(_init._device, &dview_info, nullptr, &_init._msaaDepthImage.imageView));

    ImGui_ImplVulkan_SetMinImageCount(_init._swapchainImageViews.size());

    resize_requested = false;
}


void VK_APPLICATION::VulkanApplication::destroy_swapchain(){
    fmt::print("Destroy swapchain\n");

    if (_init._allocator != VK_NULL_HANDLE) {
        if (_init._drawImage.imageView != VK_NULL_HANDLE) {
            vkDestroyImageView(_init._device, _init._drawImage.imageView, nullptr);
            _init._drawImage.imageView = VK_NULL_HANDLE;
        }
        /*
        if (_init._depthImage.imageView != VK_NULL_HANDLE) {
            vkDestroyImageView(_init._device, _init._depthImage.imageView, nullptr);
            _init._depthImage.imageView = VK_NULL_HANDLE;
        }
        */
        if (_init._msaaColorImage.imageView != VK_NULL_HANDLE) {
            vkDestroyImageView(_init._device, _init._msaaColorImage.imageView, nullptr);
            _init._msaaColorImage.imageView = VK_NULL_HANDLE;
        }
        if (_init._msaaDepthImage.imageView != VK_NULL_HANDLE) {
            vkDestroyImageView(_init._device, _init._msaaDepthImage.imageView, nullptr);
            _init._msaaDepthImage.imageView = VK_NULL_HANDLE;
        }
        if (_init._drawImage.image != VK_NULL_HANDLE) {
            vmaDestroyImage(_init._allocator, _init._drawImage.image, _init._drawImage.allocation);
            _init._drawImage.image = VK_NULL_HANDLE;
            _init._drawImage.allocation = VK_NULL_HANDLE;
        }
        /*
        if (_init._depthImage.image != VK_NULL_HANDLE) {
            vmaDestroyImage(_init._allocator, _init._depthImage.image, _init._depthImage.allocation);
            _init._depthImage.image = VK_NULL_HANDLE;
            _init._depthImage.allocation = VK_NULL_HANDLE;
        }
        */
        if (_init._msaaColorImage.image != VK_NULL_HANDLE) {
            vmaDestroyImage(_init._allocator, _init._msaaColorImage.image, _init._msaaColorImage.allocation);
            _init._msaaColorImage.image = VK_NULL_HANDLE;
            _init._msaaColorImage.allocation = VK_NULL_HANDLE;
        }
        if (_init._msaaDepthImage.image != VK_NULL_HANDLE) {
            vmaDestroyImage(_init._allocator, _init._msaaDepthImage.image, _init._msaaDepthImage.allocation);
            _init._msaaDepthImage.image = VK_NULL_HANDLE;
            _init._msaaDepthImage.allocation = VK_NULL_HANDLE;
        }
    }

    for (auto imageView : _init._swapchainImageViews) {
        if (imageView != VK_NULL_HANDLE) {
            vkDestroyImageView(_init._device, imageView, nullptr);
        }
    }
    _init._swapchainImageViews.clear();
    _init._swapchainImages.clear();

    if (_init._swapchain != VK_NULL_HANDLE) {
        vkDestroySwapchainKHR(_init._device, _init._swapchain, nullptr);
        _init._swapchain = VK_NULL_HANDLE;
    }
}

void VK_APPLICATION::VulkanApplication::init_descriptors(){

    // Для данных о сцене
    {
        DescriptorLayoutBuilder builder;
        builder.add_binding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
        _gpuSceneDataDescriptorLayout = builder.build(_init._device, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT);
    }

    VkDescriptorPoolSize poolSize{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, (uint32_t)FRAME_OVERLAP };
    VkDescriptorPoolCreateInfo poolInfo{ VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO };
    poolInfo.maxSets = FRAME_OVERLAP;
    poolInfo.poolSizeCount = 1;
    poolInfo.pPoolSizes = &poolSize;
    // Создаем _sceneDescriptorPool (добавь это поле типа VkDescriptorPool в VulkanApplication.h)
    vkCreateDescriptorPool(_init._device, &poolInfo, nullptr, &_sceneDescriptorPool);

    for (int i = 0; i < FRAME_OVERLAP; i++) {
        // Выделяем память под UBO (твой код)
        _frames[i].gpuSceneDataBuffer = vkinit::create_buffer(
            sizeof(GPUSceneData), _init._allocator,
            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU
        );

        // Аллоцируем постоянный сет для кадра напрямую из нашего пула сцены!
        VkDescriptorSetAllocateInfo allocInfo{ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
        allocInfo.descriptorPool = _sceneDescriptorPool;
        allocInfo.descriptorSetCount = 1;
        allocInfo.pSetLayouts = &_gpuSceneDataDescriptorLayout;
        vkAllocateDescriptorSets(_init._device, &allocInfo, &_frames[i].sceneDescriptorSet);

        // Привязываем буфер к этому дескриптору намертво прямо на старте
        DescriptorWriter writer;
        writer.write_buffer(0, _frames[i].gpuSceneDataBuffer.buffer, sizeof(GPUSceneData), 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
        writer.update_set(_init._device, _frames[i].sceneDescriptorSet);
    }
}

void VK_APPLICATION::VulkanApplication::init_pipeline_manager(){
    _pipelineManager = std::make_unique<PipelineManager>(_init._device);

    VkDescriptorSetLayout textureLayout = _textureManager.GetTextureLayout();

    if (textureLayout == VK_NULL_HANDLE) {
        fmt::print(stderr, "[ERROR] textureLayout is STILL VK_NULL_HANDLE before InitCommonLayout!\n");
        std::abort();
    }

    _pipelineManager->InitCommonLayout(_gpuSceneDataDescriptorLayout, textureLayout);
    // Форматы для Dynamic Rendering берем из вашей MSAA картинки, как в старом коде
    VkFormat colorFormat = _init._msaaColorImage.imageFormat;
    VkFormat depthFormat = _init._msaaDepthImage.imageFormat;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Конвеер для базовых моделей (непрозрачных)
    PipelineCreateInfo baseMeshInfo{};
    baseMeshInfo.name = "BaseMesh";
    baseMeshInfo.passType = RenderPassType::Forward;
    baseMeshInfo.opacity = PipelineOpacity::Opaque;
    baseMeshInfo.useMSAA = true; // Так как в старом коде было _maxSamples
    baseMeshInfo.vertexShaderPath = "../Shaders/BaseMesh/Source/mesh.vert";
    baseMeshInfo.fragmentShaderPath = "../Shaders/BaseMesh/Source/mesh.frag";

    RealPipeline* basePipeline = _pipelineManager->CreatePipeline(baseMeshInfo, colorFormat, depthFormat, _maxSamples);
    if (basePipeline) {
        fmt::print("[PipelineManager] Pipeline 'BaseMesh' successfully loaded and built.\n");
    }

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Конвеер для базовых моделей (прозрачных)
    PipelineCreateInfo transparentMeshInfo{};
    transparentMeshInfo.name = "TransparentMesh";
    transparentMeshInfo.passType = RenderPassType::Forward;
    transparentMeshInfo.opacity = PipelineOpacity::Transparent;
    transparentMeshInfo.useMSAA = true;
    transparentMeshInfo.vertexShaderPath = "../Shaders/BaseMesh/Source/mesh.vert";
    transparentMeshInfo.fragmentShaderPath = "../Shaders/BaseMesh/Source/mesh.frag";

    RealPipeline* transPipeline = _pipelineManager->CreatePipeline(transparentMeshInfo, colorFormat, depthFormat, _maxSamples);
    if (transPipeline) {
        fmt::print("[PipelineManager] Pipeline 'TransparentMesh' successfully loaded and built.\n");
    }
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Конвеер для базовых моделей (AlphaTested)
    PipelineCreateInfo alphaTestedMeshInfo{};
    alphaTestedMeshInfo.name = "AlphaTestedMesh";
    alphaTestedMeshInfo.passType = RenderPassType::Forward;
    alphaTestedMeshInfo.opacity = PipelineOpacity::AlphaTested;
    alphaTestedMeshInfo.useMSAA = true;
    alphaTestedMeshInfo.vertexShaderPath = "../Shaders/BaseMesh/Source/mesh.vert";
    alphaTestedMeshInfo.fragmentShaderPath = "../Shaders/BaseMesh/Source/mesh.frag";

    RealPipeline* alphaPipeline = _pipelineManager->CreatePipeline(alphaTestedMeshInfo, colorFormat, depthFormat, _maxSamples);
    if (alphaPipeline) {
        fmt::print("[PipelineManager] Pipeline 'AlphaTestedMesh' successfully loaded and built.\n");
    }
 ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Конвеер для сетки
    PipelineCreateInfo gridInfo{};
    gridInfo.name = "Grid";
    gridInfo.passType = RenderPassType::Forward;
    gridInfo.opacity = PipelineOpacity::Transparent; // Включает AlphaBlend, отключает запись в глубину
    gridInfo.useMSAA = true; // Использовал set_multisampling_alpha(_maxSamples)
    gridInfo.vertexShaderPath = "../Shaders/InfGrid/Source/grid.vert";
    gridInfo.fragmentShaderPath = "../Shaders/InfGrid/Source/grid.frag";

    RealPipeline* gridPipeline = _pipelineManager->CreatePipeline(gridInfo, colorFormat, depthFormat, _maxSamples);
    if (gridPipeline) {
        fmt::print("[PipelineManager] Pipeline 'Grid' successfully loaded and built.\n");
    }

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Конвеер для каскадных теней
    PipelineCreateInfo shadowInfo{};
    shadowInfo.name = "Shadow";
    shadowInfo.passType = RenderPassType::ShadowCSM;
    shadowInfo.opacity = PipelineOpacity::Opaque;
    shadowInfo.useMSAA = false;
    shadowInfo.vertexShaderPath = "../Shaders/SCM/Source/shadow.vert";
    shadowInfo.fragmentShaderPath = "";

    VkFormat shadowDepthFormat = VK_FORMAT_D32_SFLOAT;

    RealPipeline* shadowPipeline = _pipelineManager->CreatePipeline(shadowInfo, VK_FORMAT_UNDEFINED, shadowDepthFormat, VK_SAMPLE_COUNT_1_BIT);
    if (shadowPipeline) {
        fmt::print("[PipelineManager] Pipeline 'ShadowCSM' successfully loaded and built for Layered Rendering.\n");
    }
    // Проходы рендера
    _renderSystem.AddPass(std::make_unique<ShadowCSMRenderPass>(_init), *_pipelineManager);
    _renderSystem.AddPass(std::make_unique<ForwardRenderPass>(_init), *_pipelineManager);
    _renderSystem.AddPass(std::make_unique<GridRenderPass>(_init), *_pipelineManager);
}

void VK_APPLICATION::VulkanApplication::init_commands(){
    VkCommandPoolCreateInfo commandPoolInfo = vkinit::command_pool_create_info(_init._graphicsQueueFamily, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);

    for (int i = 0; i < FRAME_OVERLAP; i++) {

        VK_CHECK(vkCreateCommandPool(_init._device, &commandPoolInfo, nullptr, &_frames[i]._commandPool));
        VkCommandBufferAllocateInfo cmdAllocInfo = vkinit::command_buffer_allocate_info(_frames[i]._commandPool, 1);

        VK_CHECK(vkAllocateCommandBuffers(_init._device, &cmdAllocInfo, &_frames[i]._mainCommandBuffer));
    }
}

void VK_APPLICATION::VulkanApplication::init_scene(){
    _meshManager.init(_init);
    _textureManager.init(_init);
    _activeScene = std::make_unique<Scene>(_modelManager);
    CSMConfig csmConfig{};
    _lightManager = std::make_unique<LightManager>(_init._device, _textureManager, csmConfig);
    _lightManager->init();
}

VkDescriptorSet VK_APPLICATION::VulkanApplication::update_scene_data(FrameData& currentFrame){
    // Z-up
    glm::vec3 up = {0.0f, 0.0f, 1.0f};

    // For camera-movement
    _camera.front.x = cos(glm::radians(_camera.yaw)) * cos(glm::radians(_camera.pitch));
    _camera.front.y = sin(glm::radians(_camera.yaw)) * cos(glm::radians(_camera.pitch));
    _camera.front.z = sin(glm::radians(_camera.pitch));
    _camera.front = glm::normalize(_camera.front);

    _camera.Wfront.x = cos(glm::radians(_camera.yaw));
    _camera.Wfront.y = sin(glm::radians(_camera.yaw));
    _camera.Wfront.z = 0;

    _camera.right = glm::normalize(glm::cross(_camera.Wfront, up));

    //For camera
    glm::vec3 eye = { _movement.valueX, _movement.valueY, _movement.valueZ };
    glm::vec3 target = eye + _camera.front;
    sceneData.view = glm::lookAt(eye, target, up);

    glm::vec3 lightDir = glm::normalize(glm::vec3(-0.5f, -0.6f, -0.8f));
    float sunPower = 7.5f; // Интенсивность для PBR

    sceneData.sunlightDirection = glm::vec4(lightDir, sunPower);
    
    sceneData.sunlightColor = glm::vec4(1.0f, 0.95f, 0.85f, 1.0f);

    sceneData.ambientColor = glm::vec4(0.2f, 0.25f, 0.35f, 1.0f);

    float aspect = (float)_init._windowExtent.width / (float)_init._windowExtent.height;
    float fov = glm::radians(70.0f);
    float cNear = 0.1f;
    float cFar = 100.0f;

    sceneData.proj = glm::tweakedInfinitePerspective(fov, aspect, cNear);
    sceneData.proj[1][1] *= -1.0f;

    // proj * view
    sceneData.viewproj = sceneData.proj * sceneData.view;
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // КАСКАДЫ ТЕНЕЙ
    _lightManager->UpdateCascades(sceneData.view, fov, aspect, cNear, cFar, lightDir);

    // Переносим матрицы каскадов в sceneData
    const glm::mat4* matrices = _lightManager->GetCascadeMatrices();
    for(int i = 0; i < 4; ++i) {
        sceneData.cascadeMatrices[i] = matrices[i];
    }

    // Переносим дистанции отсечения каскадов (упаковываем 4 float в один vec4)
    const float* splits = _lightManager->GetCascadeSplits();
    sceneData.cascadeSplits = glm::vec4(splits[0], splits[1], splits[2], splits[3]);

    // Передаем Bindless ID текстуры теней из менеджера
    sceneData.shadowMapTextureID = _lightManager->GetShadowTextureIndex();

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VmaAllocationInfo allocInfo;
    vmaGetAllocationInfo(_init._allocator, currentFrame.gpuSceneDataBuffer.allocation, &allocInfo);

    GPUSceneData* sceneUniformData = (GPUSceneData*)allocInfo.pMappedData;
    *sceneUniformData = sceneData;

    return currentFrame.sceneDescriptorSet;
}

