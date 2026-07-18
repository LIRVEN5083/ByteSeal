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

    for (auto& mesh : testMeshes) {
        vkinit::destroy_buffer(mesh->meshBuffers.indexBuffer, _init._allocator);
        vkinit::destroy_buffer(mesh->meshBuffers.vertexBuffer, _init._allocator);
    }

    vkDestroyPipelineLayout(_init._device, _BasePipelineLayout, nullptr);
    vkDestroyPipeline(_init._device, _BasePipeline, nullptr);

    vkDestroyPipelineLayout(_init._device, _gridPipelineLayout, nullptr);
    vkDestroyPipeline(_init._device, _gridPipeline, nullptr);

    for (int i = 0; i < FRAME_OVERLAP; i++) {
        vkinit::destroy_buffer(_frames[i].gpuSceneDataBuffer, _init._allocator);

        _frames[i]._frameDescriptors.destroy_pools(_init._device);
    }

    if (_gpuSceneDataDescriptorLayout != VK_NULL_HANDLE) {
        vkDestroyDescriptorSetLayout(_init._device, _gpuSceneDataDescriptorLayout, nullptr);
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
    init_grid_pipeline();
    init_base_pipeline();
    SDL_Event e;
    bool bQuit = false;

    _delta.lastFrameTime = std::chrono::high_resolution_clock::now();
    _delta.startTime = std::chrono::high_resolution_clock::now();
    testMeshes = loadGltfMeshes(this,"../Model/1965_ford_mustang_shelby_gt350.glb").value();

    while (!bQuit) {
        while (SDL_PollEvent(&e)) {

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


            if (e.type == SDL_EVENT_MOUSE_BUTTON_DOWN) {
                if (!SDL_GetWindowRelativeMouseMode(_init._window)) {
                    SDL_SetWindowRelativeMouseMode(_init._window, true);
                    _camera.isCameraActive = true;
                }
            }
            if (e.type == SDL_EVENT_KEY_DOWN) {
                if (e.key.key == SDLK_ESCAPE) {
                    SDL_SetWindowRelativeMouseMode(_init._window, false);
                    _camera.isCameraActive = false;
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
        renderLoop();
        made_move();
    }
}

void VK_APPLICATION::VulkanApplication::renderLoop(){
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

    vkutil::transition_image(cmd, _init._drawImage.image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
    vkutil::transition_image(cmd, _init._depthImage.image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL);

    draw_meshes(cmd, globalDescriptor);
    draw_grid(cmd, globalDescriptor);

    vkutil::transition_image(cmd, _init._drawImage.image, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
    // Переводим текущую картинку Swapchain в режим приемника копирования
    vkutil::transition_image(cmd, _init._swapchainImages[swapchainImageIndex], VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

    // Копируем (и масштабируем при необходимости) наш холст прямо в Swapchain
    vkutil::copy_image_to_image(cmd, _init._drawImage.image, _init._swapchainImages[swapchainImageIndex], _drawExtent, _init._swapchainExtent);

    // Переводим картинку Swapchain в финальное состояние для отображения на мониторе
    vkutil::transition_image(cmd, _init._swapchainImages[swapchainImageIndex], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

    // Закрываем командный буфер, запись завершена
    VK_CHECK(vkEndCommandBuffer(cmd));

    // 4. ОТПРАВКА НА GPU (SUBMIT)
    VkCommandBufferSubmitInfo cmdSubmitInfo = vkinit::command_buffer_submit_info(cmd);

    // Синхронизируем семафоры: GPU ждет сигнала от Swapchain перед выгрузкой цвета
    VkSemaphoreSubmitInfo waitInfo = vkinit::semaphore_submit_info(VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT_KHR, _init._swapchainSemaphores[frameId]);
    // GPU сигналит в _renderSemaphores, когда полностью закончит блайтить пиксели
    VkSemaphoreSubmitInfo signalInfo = vkinit::semaphore_submit_info(VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT_KHR, _init._renderSemaphores[swapchainImageIndex]);

    VkSubmitInfo2 submit = vkinit::submit_info(&cmdSubmitInfo, &signalInfo, &waitInfo);

    // Отправляем буфер в очередь и передаем Fence из нашего ядра.
    // Когда GPU закончит этот кадр, Fence автоматически откроется
    VK_CHECK(vkQueueSubmit2(_init._graphicsQueue, 1, &submit, _init._renderFence[frameId]));

    // 5. ВЫВОД НА ЭКРАН (PRESENT)
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
        // Флаг нужный для resize
        .add_image_usage_flags(VK_IMAGE_USAGE_TRANSFER_DST_BIT)
        .build()
        .value();

    VkExtent3D drawImageExtent = {
        _init._windowExtent.width,
        _init._windowExtent.height,
        1
    };

    _init._swapchain = vkbSwapchain.swapchain;
    _init._swapchainImages = vkbSwapchain.get_images().value();
    _init._swapchainImageViews = vkbSwapchain.get_image_views().value();
    _init._swapchainExtent = vkbSwapchain.extent;

    _init._drawImage.imageFormat = VK_FORMAT_R16G16B16A16_SFLOAT;
    _init._drawImage.imageExtent = drawImageExtent;

    VkImageUsageFlags drawImageUsages{};
    drawImageUsages |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    drawImageUsages |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    drawImageUsages |= VK_IMAGE_USAGE_STORAGE_BIT;
    drawImageUsages |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    VkImageCreateInfo rimg_info = vkinit::image_create_info(_init._drawImage.imageFormat, drawImageUsages, drawImageExtent);

    //for the draw image, we want to allocate it from gpu local memory
    VmaAllocationCreateInfo rimg_allocinfo = {};
    rimg_allocinfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
    rimg_allocinfo.requiredFlags = VkMemoryPropertyFlags(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    //allocate and create the image
    vmaCreateImage(_init._allocator, &rimg_info, &rimg_allocinfo, &_init._drawImage.image, &_init._drawImage.allocation, nullptr);

    //build a image-view for the draw image to use for rendering
    VkImageViewCreateInfo rview_info = vkinit::imageview_create_info(_init._drawImage.imageFormat, _init._drawImage.image, VK_IMAGE_ASPECT_COLOR_BIT);

    VK_CHECK(vkCreateImageView(_init._device, &rview_info, nullptr, &_init._drawImage.imageView));

    _init._depthImage.imageFormat = VK_FORMAT_D32_SFLOAT;
    _init._depthImage.imageExtent = drawImageExtent;
    VkImageUsageFlags depthImageUsages{};
    depthImageUsages |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;

    VkImageCreateInfo dimg_info = vkinit::image_create_info(_init._depthImage.imageFormat, depthImageUsages, drawImageExtent);

    //allocate and create the image
    vmaCreateImage(_init._allocator, &dimg_info, &rimg_allocinfo, &_init._depthImage.image, &_init._depthImage.allocation, nullptr);

    //build a image-view for the draw image to use for rendering
    VkImageViewCreateInfo dview_info = vkinit::imageview_create_info(_init._depthImage.imageFormat, _init._depthImage.image, VK_IMAGE_ASPECT_DEPTH_BIT);

    VK_CHECK(vkCreateImageView(_init._device, &dview_info, nullptr, &_init._depthImage.imageView));

    resize_requested = false;
}


void VK_APPLICATION::VulkanApplication::destroy_swapchain(){
    // Очистка swapchain
    if (_init._allocator != VK_NULL_HANDLE) {
        if (_init._drawImage.imageView != VK_NULL_HANDLE) {
            vkDestroyImageView(_init._device, _init._drawImage.imageView, nullptr);
            _init._drawImage.imageView = VK_NULL_HANDLE;
        }
        if (_init._depthImage.imageView != VK_NULL_HANDLE) {
            vkDestroyImageView(_init._device, _init._depthImage.imageView, nullptr);
            _init._depthImage.imageView = VK_NULL_HANDLE;
        }
        if (_init._drawImage.image != VK_NULL_HANDLE) {
            vmaDestroyImage(_init._allocator, _init._drawImage.image, _init._drawImage.allocation);
            _init._drawImage.image = VK_NULL_HANDLE;
            _init._drawImage.allocation = VK_NULL_HANDLE;
        }
        if (_init._depthImage.image != VK_NULL_HANDLE) {
            vmaDestroyImage(_init._allocator, _init._depthImage.image, _init._depthImage.allocation);
            _init._depthImage.image = VK_NULL_HANDLE;
            _init._depthImage.allocation = VK_NULL_HANDLE;
        }
    }

    for (auto imageView : _init._swapchainImageViews) {
        if (imageView != VK_NULL_HANDLE) {
            vkDestroyImageView(_init._device, imageView, nullptr);
        }
    }

    _init._swapchainImageViews.clear();
    _init._swapchainImages.clear();

    // Уничтожаем сам Swapchain
    if (_init._swapchain != VK_NULL_HANDLE) {
        vkDestroySwapchainKHR(_init._device, _init._swapchain, nullptr);
    }
}

GPUMeshBuffers VK_APPLICATION::VulkanApplication::upload_meshes(std::span<uint32_t> indices, std::span<Vertex> vertices){
    // Размер масивов с данными, чтобы программа знала точное количество
    const size_t vertexBufferSize = vertices.size() * sizeof(Vertex);
    const size_t indexBufferSize = indices.size() * sizeof(uint32_t);

    // Тут мы храним:
    // 1. Буфер индексов
    // 2. Буфер вершин
    // 3. Адресс на всю эту бурмалду
    GPUMeshBuffers newSurface;

    // Создание вершинного буфера
    newSurface.vertexBuffer = vkinit::create_buffer(vertexBufferSize, _init._allocator, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
        VMA_MEMORY_USAGE_GPU_ONLY);

    // Находим адресс к вершиному буферу
    VkBufferDeviceAddressInfo deviceAdressInfo{ .sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,.buffer = newSurface.vertexBuffer.buffer };
    // Заносим этот адресс в нашу структуру
    newSurface.vertexBufferAddress = vkGetBufferDeviceAddress(_init._device, &deviceAdressInfo);

    // Создаём буфер индексов
    newSurface.indexBuffer = vkinit::create_buffer(indexBufferSize, _init._allocator, VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        VMA_MEMORY_USAGE_GPU_ONLY);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Дальше типичная реализация Staging buffer, т.е системы копирования данных и передачи их в видеокарту напрямую
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    // staging buffer и его создание с флагом VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
    // который говорит что это источник для копирования
    AllocatedBuffer staging = vkinit::create_buffer(vertexBufferSize + indexBufferSize, _init._allocator, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY);
    // Получаем указатель
    VmaAllocationInfo allocInfo;
    vmaGetAllocationInfo(_init._allocator, staging.allocation, &allocInfo);
    void* data = allocInfo.pMappedData;

    // Копируем данные о вершинах в staging buffer
    memcpy(data, vertices.data(), vertexBufferSize);
    // Копируем данные о индексах в staging buffer,
    // но со смешением по памяти в массив вершин
    memcpy((char*)data + vertexBufferSize, indices.data(), indexBufferSize);

    // И используем нашу крутую функцию для быстрой записы в командный буфер
    // Тут мы копируем данные из Staging Buffer в уже красиво подготовленый
    // адресс памяти (технология BDA)
    vkinit::submit_immediate([&](VkCommandBuffer cmd) {
        VkBufferCopy vertexCopy{ 0 };
        vertexCopy.dstOffset = 0;
        vertexCopy.srcOffset = 0;
        vertexCopy.size = vertexBufferSize;

        vkCmdCopyBuffer(cmd, staging.buffer, newSurface.vertexBuffer.buffer, 1, &vertexCopy);

        VkBufferCopy indexCopy{ 0 };
        indexCopy.dstOffset = 0;
        indexCopy.srcOffset = vertexBufferSize; // Мы начинаем не с 0 адресса как для вершин, а с конца адресса вершин
        indexCopy.size = indexBufferSize;

        vkCmdCopyBuffer(cmd, staging.buffer, newSurface.indexBuffer.buffer, 1, &indexCopy);
    }, _init);

    // Уничтожаем Staging Buffer
    vkinit::destroy_buffer(staging, _init._allocator);

    // Возвращаем
    return newSurface;
}

void VK_APPLICATION::VulkanApplication::init_descriptors(){

    // Для данных о сцене
    {
        DescriptorLayoutBuilder builder;
        builder.add_binding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
        _gpuSceneDataDescriptorLayout = builder.build(_init._device, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT);
    }

    for (int i = 0; i < FRAME_OVERLAP; i++) {
        // Выделяем память ОДИН раз
        _frames[i].gpuSceneDataBuffer = vkinit::create_buffer(
            sizeof(GPUSceneData),
            _init._allocator,
            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            VMA_MEMORY_USAGE_CPU_TO_GPU
        );

        std::vector<DescriptorAllocatorGrowable::PoolSizeRatio> frame_sizes = {
            { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 3 },
        };

        _frames[i]._frameDescriptors = DescriptorAllocatorGrowable{};
        _frames[i]._frameDescriptors.init(_init._device, 1000, frame_sizes);
    }
}

void VK_APPLICATION::VulkanApplication::init_grid_pipeline(){
    VkShaderModule triangleFragShader;
    if (!vkutil::load_shader_module("../Shaders/InfGrid/Binary/grid.frag.spv", _init._device, &triangleFragShader)) {
        fmt::print("Error when building the grid fragment shader module\n");
    }
    else {
        fmt::print("Grid fragment shader succesfully loaded\n");
    }

    VkShaderModule triangleVertexShader;
    if (!vkutil::load_shader_module("../Shaders/InfGrid/Binary/grid.vert.spv", _init._device, &triangleVertexShader)) {
        fmt::print("Error when building the grid vertex shader module\n");
    }
    else {
        fmt::print("Grid vertex shader succesfully loaded\n");
    }

    //build the pipeline layout that controls the inputs/outputs of the shader
    //we are not using descriptor sets or other systems yet, so no need to use anything other than empty default
    VkPipelineLayoutCreateInfo pipeline_layout_info = vkinit::pipeline_layout_create_info();
    pipeline_layout_info.setLayoutCount = 1;
    pipeline_layout_info.pSetLayouts = &_gpuSceneDataDescriptorLayout;
    VK_CHECK(vkCreatePipelineLayout(_init._device, &pipeline_layout_info, nullptr, &_gridPipelineLayout));

    PipelineBuilder pipelineBuilder;

    //use the triangle layout we created
    pipelineBuilder._pipelineLayout = _gridPipelineLayout;
    //connecting the vertex and pixel shaders to the pipeline
    pipelineBuilder.set_shaders(triangleVertexShader, triangleFragShader);
    //it will draw triangles
    pipelineBuilder.set_input_topology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
    //filled triangles
    pipelineBuilder.set_polygon_mode(VK_POLYGON_MODE_FILL);
    //no backface culling
    pipelineBuilder.set_cull_mode(VK_CULL_MODE_NONE, VK_FRONT_FACE_CLOCKWISE);
    //no multisampling
    pipelineBuilder.set_multisampling_none();

    pipelineBuilder.enable_blending_alphablend();

    //pipelineBuilder.disable_blending();
    pipelineBuilder.enable_depthtest(VK_FALSE, VK_COMPARE_OP_LESS_OR_EQUAL);

    //connect the image format we will draw into, from draw image
    pipelineBuilder.set_color_attachment_format(_init._drawImage.imageFormat);
    pipelineBuilder.set_depth_format(_init._depthImage.imageFormat);

    //finally build the pipeline
    _gridPipeline = pipelineBuilder.build_pipeline(_init._device);

    //clean structures
    vkDestroyShaderModule(_init._device, triangleFragShader, nullptr);
    vkDestroyShaderModule(_init._device, triangleVertexShader, nullptr);
}

void VK_APPLICATION::VulkanApplication::init_base_pipeline(){
    VkShaderModule baseFragShader;
    if (!vkutil::load_shader_module("../Shaders/BaseMesh/Binary/mesh.frag.spv", _init._device, &baseFragShader)) {
        fmt::print("Error when building the triangle fragment shader module");
    }
    else {
        fmt::print("Triangle fragment shader succesfully loaded");
    }

    VkShaderModule baseVertexShader;
    if (!vkutil::load_shader_module("../Shaders/BaseMesh/Binary/mesh.vert.spv", _init._device, &baseVertexShader)) {
        fmt::print("Error when building the triangle vertex shader module");
    }
    else {
        fmt::print("Triangle vertex shader succesfully loaded");
    }

    VkPushConstantRange bufferRange{};
    bufferRange.offset = 0;
    bufferRange.size = sizeof(GPUDrawPushConstants);
    bufferRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

    VkPipelineLayoutCreateInfo pipeline_layout_info = vkinit::pipeline_layout_create_info();
    pipeline_layout_info.pPushConstantRanges = &bufferRange;
    pipeline_layout_info.pushConstantRangeCount = 1;
    pipeline_layout_info.pSetLayouts = &_gpuSceneDataDescriptorLayout;
    pipeline_layout_info.setLayoutCount = 1;
    VK_CHECK(vkCreatePipelineLayout(_init._device, &pipeline_layout_info, nullptr, &_BasePipelineLayout));

    PipelineBuilder pipelineBuilder;

    //use the triangle layout we created
    pipelineBuilder._pipelineLayout = _BasePipelineLayout;
    //connecting the vertex and pixel shaders to the pipeline
    pipelineBuilder.set_shaders(baseVertexShader, baseFragShader);
    //it will draw triangles
    pipelineBuilder.set_input_topology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
    //filled triangles
    pipelineBuilder.set_polygon_mode(VK_POLYGON_MODE_FILL);
    //no backface culling
    pipelineBuilder.set_cull_mode(VK_CULL_MODE_NONE, VK_FRONT_FACE_CLOCKWISE);
    //no multisampling
    pipelineBuilder.set_multisampling_none();

    pipelineBuilder.disable_blending();

    //pipelineBuilder.disable_blending();
    pipelineBuilder.enable_depthtest(VK_TRUE, VK_COMPARE_OP_LESS_OR_EQUAL);

    //connect the image format we will draw into, from draw image
    pipelineBuilder.set_color_attachment_format(_init._drawImage.imageFormat);
    pipelineBuilder.set_depth_format(_init._depthImage.imageFormat);

    //finally build the pipeline
    _BasePipeline = pipelineBuilder.build_pipeline(_init._device);

    //clean structures
    vkDestroyShaderModule(_init._device, baseFragShader, nullptr);
    vkDestroyShaderModule(_init._device, baseVertexShader, nullptr);
}

void VK_APPLICATION::VulkanApplication::init_commands(){
    VkCommandPoolCreateInfo commandPoolInfo = vkinit::command_pool_create_info(_init._graphicsQueueFamily, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);

    for (int i = 0; i < FRAME_OVERLAP; i++) {

        VK_CHECK(vkCreateCommandPool(_init._device, &commandPoolInfo, nullptr, &_frames[i]._commandPool));
        VkCommandBufferAllocateInfo cmdAllocInfo = vkinit::command_buffer_allocate_info(_frames[i]._commandPool, 1);

        VK_CHECK(vkAllocateCommandBuffers(_init._device, &cmdAllocInfo, &_frames[i]._mainCommandBuffer));
    }
}

void VK_APPLICATION::VulkanApplication::draw_grid(VkCommandBuffer cmd, VkDescriptorSet globalDescriptor){
    VkRenderingAttachmentInfo colorAttachment = vkinit::attachment_info(_init._drawImage.imageView, nullptr, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD; // <-- СОХРАНЯЕМ цвет машины!
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;

    VkRenderingAttachmentInfo depthAttachment = vkinit::depth_attachment_info(_init._depthImage.imageView, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL);
    depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD; // <-- СОХРАНЯЕМ глубину машины!
    depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;

    VkRenderingInfo renderInfo = vkinit::rendering_info(_drawExtent, &colorAttachment, &depthAttachment);
    vkCmdBeginRendering(cmd, &renderInfo);

    VkViewport viewport{};
    viewport.x = 0;
    viewport.y = 0;
    viewport.width = _drawExtent.width;
    viewport.height = _drawExtent.height;
    viewport.minDepth = 0.f;
    viewport.maxDepth = 1.f;
    vkCmdSetViewport(cmd, 0, 1, &viewport);

    VkRect2D scissor = {};
    scissor.offset.x = 0;
    scissor.offset.y = 0;
    scissor.extent.width = _drawExtent.width;
    scissor.extent.height = _drawExtent.height;
    vkCmdSetScissor(cmd, 0, 1, &scissor);


    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, _gridPipeline);

    vkCmdBindDescriptorSets(
        cmd,
        VK_PIPELINE_BIND_POINT_GRAPHICS,
        _gridPipelineLayout,
        0,
        1,
        &globalDescriptor,
        0, nullptr
    );

    vkCmdDraw(cmd, 6, 1, 0, 0);
    vkCmdEndRendering(cmd);
}

void VK_APPLICATION::VulkanApplication::draw_meshes(VkCommandBuffer cmd, VkDescriptorSet globalDescriptor){
    // 1. Проверяем, загрузились ли наши меши при старте, чтобы не поймать crash
    if (testMeshes.empty()) return;

    VkClearValue clearColor;
    clearColor.color = { { 0.3f, 0.3f, 0.3f, 1.0f } };

    VkRenderingAttachmentInfo colorAttachment = vkinit::attachment_info(_init._drawImage.imageView, &clearColor, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR; // <-- СТИРАЕМ старый цвет
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;

    VkRenderingAttachmentInfo depthAttachment = vkinit::depth_attachment_info(_init._depthImage.imageView, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL);
    depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR; // <-- СТИРАЕМ старую глубину
    depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;

    VkRenderingInfo renderInfo = vkinit::rendering_info(_drawExtent, &colorAttachment, &depthAttachment);
    vkCmdBeginRendering(cmd, &renderInfo);

    // 3. Выставляем динамические Viewport и Scissor
    VkViewport viewport = {};
    viewport.x = 0;
    viewport.y = 0;
    viewport.width = _drawExtent.width;
    viewport.height = _drawExtent.height;
    viewport.minDepth = 0.f;
    viewport.maxDepth = 1.f;
    vkCmdSetViewport(cmd, 0, 1, &viewport);

    VkRect2D scissor = {};
    scissor.offset.x = 0;
    scissor.offset.y = 0;
    scissor.extent.width = _drawExtent.width;
    scissor.extent.height = _drawExtent.height;
    vkCmdSetScissor(cmd, 0, 1, &scissor);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Биндим пайплайн для 3D мешей (созданный с поддержкой формата глубины D32)
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, _BasePipeline);

    vkCmdBindDescriptorSets(
        cmd,
        VK_PIPELINE_BIND_POINT_GRAPHICS,
        _BasePipelineLayout,
        0,
        1,
        &globalDescriptor,
        0, nullptr
    );

    for (const auto& mesh : testMeshes)
    {
        // Биндим индексный буфер КОНКРЕТНОЙ модели (он один на весь меш)
        vkCmdBindIndexBuffer(cmd, mesh->meshBuffers.indexBuffer.buffer, 0, VK_INDEX_TYPE_UINT32);

        // ВНУТРЕННИЙ ЦИКЛ: Рисуем каждую отдельную часть (surface) этой модели
        for (const auto& surface : mesh->surfaces)
        {
            GPUDrawPushConstants push_constants;

            push_constants.worldMatrix = surface.transform;

            push_constants.vertexBuffer = mesh->meshBuffers.vertexBufferAddress;

            vkCmdPushConstants(cmd, _BasePipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(GPUDrawPushConstants), &push_constants);

            vkCmdDrawIndexed(
                cmd,
                surface.count,
                1,
                surface.startIndex,
                0,
                0
            );
        }
    }
    vkCmdEndRendering(cmd);
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

    // Perspective projection
    float aspect = (float)_init._windowExtent.width / (float)_init._windowExtent.height;
    sceneData.proj = glm::perspective(glm::radians(70.0f), aspect, 0.1f, 1000.0f);
    sceneData.proj[1][1] *= -1.0f;

    // proj * view
    sceneData.viewproj = sceneData.proj * sceneData.view;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VmaAllocationInfo allocInfo;
    vmaGetAllocationInfo(_init._allocator, currentFrame.gpuSceneDataBuffer.allocation, &allocInfo);

    GPUSceneData* sceneUniformData = (GPUSceneData*)allocInfo.pMappedData;
    *sceneUniformData = sceneData;

    VkDescriptorSet globalDescriptor = currentFrame._frameDescriptors.allocate(_init._device, _gpuSceneDataDescriptorLayout);

    DescriptorWriter writer;
    writer.write_buffer(0, currentFrame.gpuSceneDataBuffer.buffer, sizeof(GPUSceneData), 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
    writer.update_set(_init._device, globalDescriptor);

    return globalDescriptor;
}

void VK_APPLICATION::VulkanApplication::made_move(){
    auto currentTime = std::chrono::high_resolution_clock::now();
    auto deltaTime = std::chrono::duration_cast<std::chrono::duration<float>>(currentTime - _delta.lastFrameTime).count();
    _delta.lastFrameTime = currentTime;
    _delta.moveStep = deltaTime * _movement.speed;

    if (_camera.isCameraActive) {
        int numkeys;
        const bool* keyboardState = SDL_GetKeyboardState(&numkeys);

        if (keyboardState[SDL_SCANCODE_W]) {
            _movement.valueY += _camera.Wfront.y * _delta.moveStep;
            _movement.valueX += _camera.Wfront.x * _delta.moveStep;
        }
        if (keyboardState[SDL_SCANCODE_S]) {
            _movement.valueY -= _camera.Wfront.y * _delta.moveStep;
            _movement.valueX -= _camera.Wfront.x * _delta.moveStep;
        }

        if (keyboardState[SDL_SCANCODE_A]) {
            _movement.valueY -= _camera.right.y * _delta.moveStep;
            _movement.valueX -= _camera.right.x * _delta.moveStep;
        }
        if (keyboardState[SDL_SCANCODE_D]) {
            _movement.valueY += _camera.right.y * _delta.moveStep;
            _movement.valueX += _camera.right.x * _delta.moveStep;
        }
        if (keyboardState[SDL_SCANCODE_SPACE]) {
            _movement.valueZ += 1.0f * _delta.moveStep;
        }
        if (keyboardState[SDL_SCANCODE_LSHIFT]) {
            _movement.valueZ -= 1.0f * _delta.moveStep;
        }
        if (keyboardState[SDL_SCANCODE_E]) {
            CONTROLLER::IncreaseSpeed(_movement.speed);
        }
        if (keyboardState[SDL_SCANCODE_Q]) {
            CONTROLLER::DecreaseSpeed(_movement.speed);
        }
    }
}

