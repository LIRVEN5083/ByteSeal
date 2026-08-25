#include "vk_pipelines.h"

bool vkutil::load_shader_module(const char* filePath,
    VkDevice device,
    VkShaderModule* outShaderModule){
    // open the file. With cursor at the end
    std::ifstream file(filePath, std::ios::ate | std::ios::binary);

    if (!file.is_open()) {
        return false;
    }

    // find what the size of the file is by looking up the location of the cursor
    // because the cursor is at the end, it gives the size directly in bytes
    size_t fileSize = (size_t)file.tellg();

    // spirv expects the buffer to be on uint32, so make sure to reserve a int
    // vector big enough for the entire file
    std::vector<uint32_t> buffer(fileSize / sizeof(uint32_t));

    // put file cursor at beginning
    file.seekg(0);

    // load the entire file into the buffer
    file.read((char*)buffer.data(), fileSize);

    // now that the file is loaded into the buffer, we can close it
    file.close();

    // create a new shader module, using the buffer we loaded
    VkShaderModuleCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.pNext = nullptr;

    // codeSize has to be in bytes, so multply the ints in the buffer by size of
    // int to know the real size of the buffer
    createInfo.codeSize = buffer.size() * sizeof(uint32_t);
    createInfo.pCode = buffer.data();

    // check that the creation goes well.
    VkShaderModule shaderModule;
    if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
        return false;
    }
    *outShaderModule = shaderModule;
    return true;
}

void vkutil::generate_mipmaps(VkCommandBuffer cmd, VkImage image, VkExtent2D imageSize){
    int mipLevels = int(std::floor(std::log2(std::max(imageSize.width, imageSize.height)))) + 1;
    for (int mip = 0; mip < mipLevels; mip++) {

        VkExtent2D halfSize = imageSize;
        halfSize.width /= 2;
        halfSize.height /= 2;

        VkImageMemoryBarrier2 imageBarrier { .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2, .pNext = nullptr };

        imageBarrier.srcStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
        imageBarrier.srcAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT;
        imageBarrier.dstStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
        imageBarrier.dstAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT | VK_ACCESS_2_MEMORY_READ_BIT;

        imageBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        imageBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;

        VkImageAspectFlags aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        imageBarrier.subresourceRange = vkinit::image_subresource_range(aspectMask);
        imageBarrier.subresourceRange.levelCount = 1;
        imageBarrier.subresourceRange.baseMipLevel = mip;
        imageBarrier.image = image;

        VkDependencyInfo depInfo { .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO, .pNext = nullptr };
        depInfo.imageMemoryBarrierCount = 1;
        depInfo.pImageMemoryBarriers = &imageBarrier;

        vkCmdPipelineBarrier2(cmd, &depInfo);

        if (mip < mipLevels - 1) {
            VkImageBlit2 blitRegion { .sType = VK_STRUCTURE_TYPE_IMAGE_BLIT_2, .pNext = nullptr };

            blitRegion.srcOffsets[1].x = imageSize.width;
            blitRegion.srcOffsets[1].y = imageSize.height;
            blitRegion.srcOffsets[1].z = 1;

            blitRegion.dstOffsets[1].x = halfSize.width;
            blitRegion.dstOffsets[1].y = halfSize.height;
            blitRegion.dstOffsets[1].z = 1;

            blitRegion.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            blitRegion.srcSubresource.baseArrayLayer = 0;
            blitRegion.srcSubresource.layerCount = 1;
            blitRegion.srcSubresource.mipLevel = mip;

            blitRegion.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            blitRegion.dstSubresource.baseArrayLayer = 0;
            blitRegion.dstSubresource.layerCount = 1;
            blitRegion.dstSubresource.mipLevel = mip + 1;

            VkBlitImageInfo2 blitInfo {.sType = VK_STRUCTURE_TYPE_BLIT_IMAGE_INFO_2, .pNext = nullptr};
            blitInfo.dstImage = image;
            blitInfo.dstImageLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            blitInfo.srcImage = image;
            blitInfo.srcImageLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
            blitInfo.filter = VK_FILTER_LINEAR;
            blitInfo.regionCount = 1;
            blitInfo.pRegions = &blitRegion;

            vkCmdBlitImage2(cmd, &blitInfo);

            imageSize = halfSize;
        }
    }

    // transition all mip levels into the final read_only layout
    transition_image(cmd, image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
}

void PipelineBuilder::clear(){
    // clear all of the structs we need back to 0 with their correct stype
    _inputAssembly = { .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO };

    _rasterizer = { .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO };

    _colorBlendAttachment = {};

    _multisampling = { .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO };

    _pipelineLayout = {};

    _depthStencil = { .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO };

    _renderInfo = { .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO };

    _shaderStages.clear();
}

VkPipeline PipelineBuilder::build_pipeline(VkDevice device){
    // make viewport state from our stored viewport and scissor.
    // at the moment we wont support multiple viewports or scissors
    VkPipelineViewportStateCreateInfo viewportState = {};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.pNext = nullptr;

    viewportState.viewportCount = 1;
    viewportState.scissorCount = 1;

    // setup dummy color blending. We arent using transparent objects yet
    // the blending is just "no blend", but we do write to the color attachment
    VkPipelineColorBlendStateCreateInfo colorBlending = {};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.pNext = nullptr;

    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.logicOp = VK_LOGIC_OP_COPY;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &_colorBlendAttachment;

    // completely clear VertexInputStateCreateInfo, as we have no need for it
    VkPipelineVertexInputStateCreateInfo _vertexInputInfo = { .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO };

    // build the actual pipeline
    // we now use all of the info structs we have been writing into into this one
    // to create the pipeline
    VkGraphicsPipelineCreateInfo pipelineInfo = { .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO };
    // connect the renderInfo to the pNext extension mechanism
    pipelineInfo.pNext = &_renderInfo;

    if (_shaderStages.size() == 1) {
        colorBlending.attachmentCount = 0;
        colorBlending.pAttachments = nullptr;
    } else {
        colorBlending.attachmentCount = 1;
        colorBlending.pAttachments = &_colorBlendAttachment;
    }

    pipelineInfo.stageCount = (uint32_t)_shaderStages.size();
    pipelineInfo.pStages = _shaderStages.data();
    pipelineInfo.pVertexInputState = &_vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &_inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &_rasterizer;
    pipelineInfo.pMultisampleState = &_multisampling;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.pDepthStencilState = &_depthStencil;
    pipelineInfo.layout = _pipelineLayout;
    VkDynamicState state[] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };

    VkPipelineDynamicStateCreateInfo dynamicInfo = { .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO };
    dynamicInfo.pDynamicStates = &state[0];
    dynamicInfo.dynamicStateCount = 2;

    pipelineInfo.pDynamicState = &dynamicInfo;
    VkPipeline newPipeline;
    if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo,
            nullptr, &newPipeline)
        != VK_SUCCESS) {
        fmt::println("failed to create pipeline");
        return VK_NULL_HANDLE; // failed to create graphics pipeline
        } else {
            return newPipeline;
        }
}

void PipelineBuilder::set_shaders(VkShaderModule vertexShader, VkShaderModule fragmentShader){
    _shaderStages.clear();

    _shaderStages.push_back(
        vkinit::pipeline_shader_stage_create_info(VK_SHADER_STAGE_VERTEX_BIT, vertexShader));

    if (fragmentShader != VK_NULL_HANDLE) {
        _shaderStages.push_back(
            vkinit::pipeline_shader_stage_create_info(VK_SHADER_STAGE_FRAGMENT_BIT, fragmentShader));
    }
}

void PipelineBuilder::set_input_topology(VkPrimitiveTopology topology){
    _inputAssembly.topology = topology;
    // we are not going to use primitive restart on the entire tutorial so leave
    // it on false
    _inputAssembly.primitiveRestartEnable = VK_FALSE;
}

void PipelineBuilder::set_polygon_mode(VkPolygonMode mode){
    _rasterizer.polygonMode = mode;
    _rasterizer.lineWidth = 1.f;
}

void PipelineBuilder::set_cull_mode(VkCullModeFlags cullMode, VkFrontFace frontFace){
    _rasterizer.cullMode = cullMode;
    _rasterizer.frontFace = frontFace;
}

void PipelineBuilder::set_multisampling_none(){
    _multisampling.sampleShadingEnable = VK_FALSE;
    // multisampling defaulted to no multisampling (1 sample per pixel)
    _multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    _multisampling.minSampleShading = 1.0f;
    _multisampling.pSampleMask = nullptr;
    // no alpha to coverage either
    _multisampling.alphaToCoverageEnable = VK_FALSE;
    _multisampling.alphaToOneEnable = VK_FALSE;
}

void PipelineBuilder::set_multisampling(VkSampleCountFlagBits samples){
    _multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    _multisampling.pNext = nullptr;

    // Количество сэмплов
    _multisampling.rasterizationSamples = samples;

    // Базовые настройки аппаратного MSAA
    _multisampling.sampleShadingEnable = VK_FALSE; // Альфа-аппроксимация (False для скорости)
    _multisampling.minSampleShading = 1.0f;
    _multisampling.pSampleMask = nullptr;

    _multisampling.alphaToCoverageEnable = VK_FALSE;
    _multisampling.alphaToOneEnable = VK_FALSE;
}

void PipelineBuilder::set_multisampling_alpha(VkSampleCountFlagBits samples){
    _multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    _multisampling.pNext = nullptr;
    _multisampling.rasterizationSamples = samples;

    // магия Alpha-to-Coverage
    _multisampling.sampleShadingEnable = VK_FALSE;
    _multisampling.alphaToCoverageEnable = VK_TRUE;
    _multisampling.alphaToOneEnable = VK_FALSE;
    _multisampling.pSampleMask = nullptr;
}

void PipelineBuilder::disable_blending(){
    // default write mask
    _colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    // no blending
    _colorBlendAttachment.blendEnable = VK_FALSE;
}

void PipelineBuilder::set_color_attachment_format(VkFormat format){
    _colorAttachmentformat = format;
    // connect the format to the renderInfo  structure

    if (format == VK_FORMAT_UNDEFINED){
        _renderInfo.colorAttachmentCount = 0;
        _renderInfo.pColorAttachmentFormats = nullptr;
    }
    else{
        _renderInfo.colorAttachmentCount = 1;
        _renderInfo.pColorAttachmentFormats = &_colorAttachmentformat;
    }
}

void PipelineBuilder::set_depth_format(VkFormat format){
    _renderInfo.depthAttachmentFormat = format;
}

void PipelineBuilder::disable_depthtest(){
    _depthStencil.depthTestEnable = VK_FALSE;
    _depthStencil.depthWriteEnable = VK_FALSE;
    _depthStencil.depthCompareOp = VK_COMPARE_OP_NEVER;
    _depthStencil.depthBoundsTestEnable = VK_FALSE;
    _depthStencil.stencilTestEnable = VK_FALSE;
    _depthStencil.front = {};
    _depthStencil.back = {};
    _depthStencil.minDepthBounds = 0.f;
    _depthStencil.maxDepthBounds = 1.f;
}

void PipelineBuilder::enable_depthtest(bool depthWriteEnable, VkCompareOp op)
{
    _depthStencil.depthTestEnable = VK_TRUE;
    _depthStencil.depthWriteEnable = depthWriteEnable ? VK_TRUE : VK_FALSE;
    _depthStencil.depthCompareOp = op;
    _depthStencil.depthBoundsTestEnable = VK_FALSE;
    _depthStencil.stencilTestEnable = VK_FALSE;
    _depthStencil.front = {};
    _depthStencil.back = {};
    _depthStencil.minDepthBounds = 0.0f;
    _depthStencil.maxDepthBounds = 1.0f;
}

void PipelineBuilder::enable_blending_additive()
{
    _colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    _colorBlendAttachment.blendEnable = VK_TRUE;
    _colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    _colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE;
    _colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
    _colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    _colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    _colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
}

void PipelineBuilder::enable_blending_alphablend()
{
    _colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    _colorBlendAttachment.blendEnable = VK_TRUE;
    _colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    _colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    _colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
    _colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    _colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    _colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
}

void PipelineManager::InitCommonLayout(VkDescriptorSetLayout globalSetLayout, VkDescriptorSetLayout bindlessSetLayout){
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // СОЗДАНИЕ ОСНОВНОГО LAYOUT
    VkPushConstantRange pushConstantRange{};
    pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
    pushConstantRange.offset = 0;
    pushConstantRange.size = sizeof(GPUDrawPushConstants);

    std::vector<VkDescriptorSetLayout> layouts = { globalSetLayout, bindlessSetLayout };

    VkPipelineLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    layoutInfo.setLayoutCount = static_cast<uint32_t>(layouts.size());
    layoutInfo.pSetLayouts = layouts.data();
    layoutInfo.pushConstantRangeCount = 1;
    layoutInfo.pPushConstantRanges = &pushConstantRange;

    if (vkCreatePipelineLayout(_device, &layoutInfo, nullptr, &_commonLayout) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create common pipeline layout!");
    }

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // СОЗДАНИЕ SCM LAYOUT
    VkPushConstantRange shadowPushConstantRange{};
    shadowPushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    shadowPushConstantRange.offset = 0;
    shadowPushConstantRange.size = sizeof(GPUShadowPushConstants); // 72 байта

    std::vector<VkDescriptorSetLayout> shadowLayouts = { globalSetLayout };

    VkPipelineLayoutCreateInfo shadowLayoutInfo{};
    shadowLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    shadowLayoutInfo.setLayoutCount = static_cast<uint32_t>(shadowLayouts.size());
    shadowLayoutInfo.pSetLayouts = shadowLayouts.data();
    shadowLayoutInfo.pushConstantRangeCount = 1;
    shadowLayoutInfo.pPushConstantRanges = &shadowPushConstantRange;

    if (vkCreatePipelineLayout(_device, &shadowLayoutInfo, nullptr, &_shadowLayout) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create shadow pipeline layout!");
    }
}

RealPipeline* PipelineManager::CreateComputePipeline(const PipelineCreateInfo& info){
    if (info.computeShaderPath.empty()) {
        fmt::print(stderr, "[PipelineManager ERROR] Compute shader path is empty for {}\n", info.name);
        return nullptr;
    }

    std::vector<uint32_t> compCode = UTILS::CompileGLSLToSPIRV(info.computeShaderPath);
    if (compCode.empty()) {
        fmt::print(stderr, "[PipelineManager ERROR] Compute shader compilation failed for {}\n", info.name);
        return nullptr;
    }

    return CreatePipelineFromMemory(info, compCode);
}

RealPipeline* PipelineManager::CreatePipelineFromMemory(const PipelineCreateInfo& info,
    const std::vector<uint32_t>& compCode){
    if (_pipelinesByName.find(info.name) != _pipelinesByName.end()) {
        return &_pipelinesByName[info.name];
    }

    VkShaderModule compModule{ VK_NULL_HANDLE };
    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = compCode.size() * sizeof(uint32_t);
    createInfo.pCode = compCode.data();

    if (vkCreateShaderModule(_device, &createInfo, nullptr, &compModule) != VK_SUCCESS) {
        fmt::print(stderr, "[PipelineManager ERROR] Failed to create VkShaderModule for compute {}\n", info.name);
        return nullptr;
    }

    VkPipelineLayout activeLayout = _commonLayout;

    VkComputePipelineCreateInfo computePipelineInfo{};
    computePipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    computePipelineInfo.layout = activeLayout;

    computePipelineInfo.stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    computePipelineInfo.stage.stage = VK_SHADER_STAGE_COMPUTE_BIT;
    computePipelineInfo.stage.module = compModule;
    computePipelineInfo.stage.pName = "main"; // Точка входа в GLSL

    VkPipeline newPipeline{ VK_NULL_HANDLE };
    if (vkCreateComputePipelines(_device, VK_NULL_HANDLE, 1, &computePipelineInfo, nullptr, &newPipeline) != VK_SUCCESS) {
        fmt::print(stderr, "[PipelineManager ERROR] vkCreateComputePipelines returned error for {}\n", info.name);
        vkDestroyShaderModule(_device, compModule, nullptr);
        return nullptr;
    }

    vkDestroyShaderModule(_device, compModule, nullptr);

    // Генерируем ID конвейера
    uint16_t newId = static_cast<uint16_t>(_pipelinesByName.size());

    // Сохраняем в твою общую структуру RealPipeline
    RealPipeline pipelineData{};
    pipelineData.name              = info.name;
    pipelineData.pipeline          = newPipeline;
    pipelineData.layout            = activeLayout;
    pipelineData.passType          = RenderPassType::Compute;
    pipelineData.opacity           = PipelineOpacity::Opaque;
    pipelineData.id                = newId;
    pipelineData.isCompute         = true; //Вычеслюшкииии
    pipelineData.computeShaderPath = info.computeShaderPath;

    _pipelinesByName[info.name] = pipelineData;

    return &_pipelinesByName[info.name];
}

RealPipeline* PipelineManager::CreatePipeline(const PipelineCreateInfo& info, VkFormat colorFormat,
                                              VkFormat depthFormat, VkSampleCountFlagBits maxSamples){

    std::vector<uint32_t> vertCode = UTILS::CompileGLSLToSPIRV(info.vertexShaderPath);
    if (vertCode.empty()) {
        fmt::print(stderr, "[PipelineManager ERROR] Vertex shader compilation failed for {}\n", info.name);
        return nullptr;
    }

    // Компилируем фрагментный шейдер ТОЛЬКО если путь к нему не пустой
    std::vector<uint32_t> fragCode;
    if (!info.fragmentShaderPath.empty()) {
        fragCode = UTILS::CompileGLSLToSPIRV(info.fragmentShaderPath);
        if (fragCode.empty()) {
            fmt::print(stderr, "[PipelineManager ERROR] Fragment shader compilation failed for {}\n", info.name);
            return nullptr;
        }
    }

    return CreatePipelineFromMemory(info, vertCode, fragCode, colorFormat, depthFormat, maxSamples);
}

RealPipeline* PipelineManager::CreatePipelineFromMemory(const PipelineCreateInfo& info,
    const std::vector<uint32_t>& vertCode, const std::vector<uint32_t>& fragCode, VkFormat colorFormat,
    VkFormat depthFormat, VkSampleCountFlagBits maxSamples){

    if (_pipelinesByName.find(info.name) != _pipelinesByName.end()) {
        return &_pipelinesByName[info.name];
    }

    // НАПРЯМУЮ СОЗДАЁМ БИНАРНИКИ И НАПРЯМУЮ СОЗДАЁМ SHADER MODULE
    VkShaderModule vertModule{ VK_NULL_HANDLE };
    VkShaderModule fragModule{ VK_NULL_HANDLE };

    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;

    createInfo.codeSize = vertCode.size() * sizeof(uint32_t);
    createInfo.pCode = vertCode.data();
    if (vkCreateShaderModule(_device, &createInfo, nullptr, &vertModule) != VK_SUCCESS) return nullptr;

    if (!info.fragmentShaderPath.empty()){
        createInfo.codeSize = fragCode.size() * sizeof(uint32_t);
        createInfo.pCode = fragCode.data();
        if (vkCreateShaderModule(_device, &createInfo, nullptr, &fragModule) != VK_SUCCESS) {
            vkDestroyShaderModule(_device, vertModule, nullptr);
            return nullptr;
        }
    }

    PipelineBuilder pipelineBuilder;
    pipelineBuilder.clear();

    // Отдаем билдеру Layout взависимости тени это или нет
    VkPipelineLayout activeLayout = VK_NULL_HANDLE;
    if (info.passType == RenderPassType::ShadowCSM) {
        activeLayout = _shadowLayout;
    }
    else{
        activeLayout = _commonLayout;
    }
    pipelineBuilder._pipelineLayout = activeLayout;

    // Настраиваем шейдеры и базовые геометрические параметры
    pipelineBuilder.set_shaders(vertModule, fragModule);
    pipelineBuilder.set_input_topology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);

    // Определяем сэмплы для MSAA
    VkSampleCountFlagBits samplesToUse = info.useMSAA ? maxSamples : VK_SAMPLE_COUNT_1_BIT;

    if (info.passType == RenderPassType::ShadowCSM){
        pipelineBuilder.set_polygon_mode(VK_POLYGON_MODE_FILL);
        pipelineBuilder.set_cull_mode(VK_CULL_MODE_NONE, VK_FRONT_FACE_CLOCKWISE);
        pipelineBuilder.set_multisampling(VK_SAMPLE_COUNT_1_BIT); // Тени всегда 1 сепмл
        pipelineBuilder.enable_depthtest(VK_TRUE, VK_COMPARE_OP_GREATER_OR_EQUAL);
    }
    else if (info.passType == RenderPassType::Skybox){
        pipelineBuilder.set_polygon_mode(VK_POLYGON_MODE_FILL);

        pipelineBuilder.set_cull_mode(VK_CULL_MODE_NONE, VK_FRONT_FACE_CLOCKWISE);

        pipelineBuilder.set_multisampling(samplesToUse);
        pipelineBuilder.disable_blending();

        pipelineBuilder.enable_depthtest(VK_FALSE, VK_COMPARE_OP_GREATER_OR_EQUAL);
    }
    else{
        pipelineBuilder.set_polygon_mode(VK_POLYGON_MODE_FILL);
        pipelineBuilder.set_cull_mode(VK_CULL_MODE_NONE, VK_FRONT_FACE_CLOCKWISE);

        if (info.opacity == PipelineOpacity::Transparent) {
            // Конфигурация для сетки (Grid)
            pipelineBuilder.set_multisampling_alpha(samplesToUse);
            pipelineBuilder.enable_blending_alphablend();
            pipelineBuilder.enable_depthtest(VK_TRUE, VK_COMPARE_OP_GREATER_OR_EQUAL);
        }
        else if (info.opacity == PipelineOpacity::AlphaTested) {
            // Конфигурация для листвы/масок (Alpha-test)
            pipelineBuilder.set_multisampling(samplesToUse);
            pipelineBuilder.disable_blending();
            pipelineBuilder.enable_depthtest(VK_TRUE, VK_COMPARE_OP_GREATER_OR_EQUAL);  // Reversed-Z, запись вкл
        }
        else {
            // Конфигурация для обычных моделей (BaseMesh)
            pipelineBuilder.set_multisampling(samplesToUse);
            pipelineBuilder.disable_blending();
            pipelineBuilder.enable_depthtest(VK_TRUE, VK_COMPARE_OP_GREATER_OR_EQUAL);   // Reversed-Z, запись вкл
        }
    }

    // Прокидываем форматы динамического рендеринга
    pipelineBuilder.set_color_attachment_format(colorFormat);
    pipelineBuilder.set_depth_format(depthFormat);

    VkPipeline newPipeline = pipelineBuilder.build_pipeline(_device);

    vkDestroyShaderModule(_device, vertModule, nullptr);
    if (fragModule != VK_NULL_HANDLE){
        vkDestroyShaderModule(_device, fragModule, nullptr);
    }

    if (newPipeline == VK_NULL_HANDLE) {
        fmt::print(stderr, "[PipelineManager ERROR] pipelineBuilder.build_pipeline returned VK_NULL_HANDLE for {}\n", info.name);
        return nullptr;
    }

    // Генерируем компактный числовой ID конвейера для нашего sortKey
    uint16_t newId = static_cast<uint16_t>(_pipelinesByName.size());

    // Сохраняем в карту менеджера конвейеров
    RealPipeline pipelineData{};
    pipelineData.name               = info.name;
    pipelineData.pipeline           = newPipeline;
    pipelineData.layout             = activeLayout;
    pipelineData.passType           = info.passType;
    pipelineData.opacity            = info.opacity;
    pipelineData.id                 = newId;

    pipelineData.vertexShaderPath   = info.vertexShaderPath;
    pipelineData.fragmentShaderPath = info.fragmentShaderPath;
    pipelineData.colorFormat        = colorFormat;
    pipelineData.depthFormat        = depthFormat;
    pipelineData.maxSamples         = maxSamples;
    _pipelinesByName[info.name] = pipelineData;

    RealPipeline* insertedPtr = &_pipelinesByName[info.name];
    PipelineKey key{ info.passType, info.opacity };
    _pipelinesByKey[key] = insertedPtr;

    return insertedPtr;
}

RealPipeline* PipelineManager::GetPipeline(RenderPassType passType, PipelineOpacity opacity) {
    auto it = _pipelinesByKey.find({ passType, opacity });
    if (it != _pipelinesByKey.end()) {
        return it->second;
    }
    return nullptr;
}

RealPipeline* PipelineManager::GetPipelineByName(const std::string& name) {
    auto it = _pipelinesByName.find(name);
    if (it != _pipelinesByName.end()) {
        return &it->second;
    }
    return nullptr;
}

bool PipelineManager::DestroyPipeline(const std::string& name){
    auto it = _pipelinesByName.find(name);

    // Если конвейер с таким именем не найден — возвращаем false
    if (it == _pipelinesByName.end()) {
        return false;
    }

    // Удаляем сам объект Vulkan
    if (it->second.pipeline != VK_NULL_HANDLE) {
        vkDestroyPipeline(_device, it->second.pipeline, nullptr);
    }

    if (!it->second.isCompute) {
        PipelineKey key{ it->second.passType, it->second.opacity };
        _pipelinesByKey.erase(key);
    }


    // Удаляем запись из хэш-карты менеджера
    _pipelinesByName.erase(it);
    return true;
}

void PipelineManager::DestroyAllPipelines(){
    for (auto& [name, pipe] : _pipelinesByName) {
        if (pipe.pipeline != VK_NULL_HANDLE) {
            vkDestroyPipeline(_device, pipe.pipeline, nullptr);
            pipe.pipeline = VK_NULL_HANDLE;
        }
    }

    _pipelinesByKey.clear();
    _pipelinesByName.clear();
}

bool PipelineManager:: ReloadAllPipelines(){
    std::cout << "[PipelineManager] Initiating full runtime pipeline rebuild\n";

    std::unordered_map<std::string, std::vector<uint32_t>> newVertCodes;
    std::unordered_map<std::string, std::vector<uint32_t>> newFragCodes;
    std::unordered_map<std::string, std::vector<uint32_t>> newCompCodes;

    for (const auto& [name, realPipeline] : _pipelinesByName) {
        // Если конвеер вычсилительный то хуячим вот сюда
        if (realPipeline.isCompute) {
            auto compCode = UTILS::CompileGLSLToSPIRV(realPipeline.computeShaderPath);
            if (compCode.empty()) {
                std::cerr << "[PipelineManager] Hot-reload aborted due to Compute shader compiler errors in " << name << ".\n";
                return false;
            }
            newCompCodes[name] = compCode;
            continue;
        }
        // Вершинный шейдер компилируем всегда
        auto vertCode = UTILS::CompileGLSLToSPIRV(realPipeline.vertexShaderPath);
        if (vertCode.empty()) {
            std::cerr << "[PipelineManager] Hot-reload aborted due to Vertex shader compiler errors in " << name << ".\n";
            return false;
        }
        newVertCodes[name] = vertCode;

        // Фрагментный шейдер компилируем ТОЛЬКО если путь к нему существует
        std::vector<uint32_t> fragCode;
        if (!realPipeline.fragmentShaderPath.empty()) {
            fragCode = UTILS::CompileGLSLToSPIRV(realPipeline.fragmentShaderPath);
            if (fragCode.empty()) {
                std::cerr << "[PipelineManager] Hot-reload aborted due to Fragment shader compiler errors in " << name << ".\n";
                return false;
            }
        }
        newFragCodes[name] = fragCode;
    }

    vkDeviceWaitIdle(_device);

    struct PipelineBackup {
        std::string name;
        RenderPassType passType;
        PipelineOpacity opacity;
        std::string vertPath;
        std::string fragPath;
        std::string compPath;
        VkFormat colorFormat;
        VkFormat depthFormat;
        VkSampleCountFlagBits maxSamples;

        // Прикол такой что в бинарное дерево при перезагрузке конвееры загружаются случайно, а не в старом порядке
        // из-за этого случайные айди конвееров ломают отрисовку в CullingAndSubmit
        uint16_t oldID;
        bool isCompute;

        // Бинарные шейдеры
        std::vector<uint32_t> vertCode;
        std::vector<uint32_t> fragCode;
        std::vector<uint32_t> compCode;
    };
    std::vector<PipelineBackup> backupQueue;

    for (const auto& [name, realPipeline] : _pipelinesByName) {
        backupQueue.push_back({
            name,
            realPipeline.passType,
            realPipeline.opacity,
            realPipeline.vertexShaderPath,
            realPipeline.fragmentShaderPath,
            realPipeline.computeShaderPath,
            realPipeline.colorFormat,
            realPipeline.depthFormat,
            realPipeline.maxSamples,
            realPipeline.id,
            realPipeline.isCompute,
            newVertCodes[name],
            newFragCodes[name],
            newCompCodes[name]
        });

        // Сразу уничтожаем старый запеченный конвейер на GPU
        if (realPipeline.pipeline != VK_NULL_HANDLE) {
            vkDestroyPipeline(_device, realPipeline.pipeline, nullptr);
        }
    }

    _pipelinesByKey.clear();
    _pipelinesByName.clear();

    std::sort(backupQueue.begin(), backupQueue.end(), [](const PipelineBackup& a, const PipelineBackup& b) {
        return a.oldID < b.oldID;
    });

    for (const auto& pipeline : backupQueue)
    {
        PipelineCreateInfo info{};
        info.name = pipeline.name;
        info.passType = pipeline.passType;
        info.opacity = pipeline.opacity;
        info.useMSAA = (pipeline.maxSamples > VK_SAMPLE_COUNT_1_BIT);
        info.vertexShaderPath = pipeline.vertPath;
        info.fragmentShaderPath = pipeline.fragPath;
        info.computeShaderPath = pipeline.compPath;

        RealPipeline* rebuilt = nullptr;

        if (pipeline.isCompute) {
            // Сам запишет только в _pipelinesByName
            rebuilt = CreatePipelineFromMemory(info, pipeline.compCode);
        }
        else {
            // Сам внутри себя запишет и в _pipelinesByName, и в _pipelinesByKey
            rebuilt = CreatePipelineFromMemory(
                info,
                pipeline.vertCode,
                pipeline.fragCode,
                pipeline.colorFormat,
                pipeline.depthFormat,
                pipeline.maxSamples);
        }

        if (rebuilt) {
            rebuilt->id = pipeline.oldID; // Просто возвращаем старый ID на место
        }
        else {
            std::cerr << "[CRITICAL ENGINE ERROR]: Failed to recreate pipeline '" << pipeline.name << "' during crude rebuild!\n";
            std::abort();
            return false;
        }
    }
    std::cout << "[PipelineManager] Success!\n";
    return true;
}

void PipelineManager::cleanup(){
    DestroyAllPipelines();

    // Уничтожаем общий Layout
    if (_commonLayout != VK_NULL_HANDLE) {
        vkDestroyPipelineLayout(_device, _commonLayout, nullptr);
        _commonLayout = VK_NULL_HANDLE;
    }

    // Уничтожаем SCM Layout
    if (_shadowLayout != VK_NULL_HANDLE) {
        vkDestroyPipelineLayout(_device, _shadowLayout, nullptr);
        _shadowLayout = VK_NULL_HANDLE;
    }
}

VkPipelineLayout PipelineManager::GetCommonLayout() const {return _commonLayout;}

VkPipelineLayout PipelineManager::GetShadowLayout() const { return _shadowLayout; }
