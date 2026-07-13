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
    SDL_Event e;
    bool bQuit = false;

    // Основной цикл вообще всего
    while (!bQuit) {
        // Основной цикл событий
        while (SDL_PollEvent(&e)) {

            if (e.type == SDL_EVENT_QUIT) {
                bQuit = true;
            }

            if (e.type == SDL_EVENT_WINDOW_MINIMIZED) {
                stop_rendering = true;
            }
            if (e.type == SDL_EVENT_WINDOW_RESTORED) {
                stop_rendering = false;
            }
        }
        // do not draw if we are minimized
        if (stop_rendering) {
            // throttle the speed to avoid the endless spinning
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            continue;
        }
        renderLoop();
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

    VmaAllocationInfo allocInfo;
    vmaGetAllocationInfo(_init._allocator, currentFrame.gpuSceneDataBuffer.allocation, &allocInfo);

    // Мгновенно перезаписываем матрицы камеры в памяти GPU поверх старых
    GPUSceneData* sceneUniformData = (GPUSceneData*)allocInfo.pMappedData;
    *sceneUniformData = sceneData; // Простое копирование структур в ОЗУ/ВРЕМЯ

    VkDescriptorSet globalDescriptor = currentFrame._frameDescriptors.allocate(_init._device, _gpuSceneDataDescriptorLayout);

    DescriptorWriter writer;
    writer.write_buffer(0, currentFrame.gpuSceneDataBuffer.buffer, sizeof(GPUSceneData), 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
    writer.update_set(_init._device, globalDescriptor);

    // О май гад это же ImageIndex из Vk-tutorial
    uint32_t swapchainImageIndex;
    // Запрашиваем картинку из
    VkResult e = vkAcquireNextImageKHR(_init._device, _init._swapchain, 1000000000, _init._swapchainSemaphores[frameId], nullptr, &swapchainImageIndex);

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

    vkQueuePresentKHR(_init._graphicsQueue, &presentInfo);

    // Переходим к следующему кадру
    _frameNumber++;
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
    pipelineBuilder.enable_depthtest(VK_TRUE, VK_COMPARE_OP_LESS_OR_EQUAL);

    //connect the image format we will draw into, from draw image
    pipelineBuilder.set_color_attachment_format(_init._drawImage.imageFormat);
    pipelineBuilder.set_depth_format(_init._depthImage.imageFormat);

    //finally build the pipeline
    _gridPipeline = pipelineBuilder.build_pipeline(_init._device);

    //clean structures
    vkDestroyShaderModule(_init._device, triangleFragShader, nullptr);
    vkDestroyShaderModule(_init._device, triangleVertexShader, nullptr);
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
    // Камера летает по кругу, смотрим строго в центр (0,0,0)
    sceneData.view = glm::lookAt(
        glm::vec3(0.0f, -2.0f, 2.0f),
        glm::vec3(0.0f, 0.0f, 2.0f),
        glm::vec3(0.0f, 0.0f, 1.0f)
    );

    float aspect = (float)_init._windowExtent.width / (float)_init._windowExtent.height;
    sceneData.proj = glm::perspective(glm::radians(70.0f), aspect, 0.1f, 1000.0f);

    sceneData.proj[1][1] *= -1.0f;

    sceneData.viewproj = sceneData.proj * sceneData.view;


    VkClearValue clearColor;
    clearColor.color = { { 1.0f, 1.0f, 1.0f, 1.0f } };

    VkRenderingAttachmentInfo colorAttachment = vkinit::attachment_info(_init._drawImage.imageView, &clearColor, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
    VkRenderingAttachmentInfo depthAttachment = vkinit::depth_attachment_info(_init._depthImage.imageView, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL);

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

