#include "vk_glTF_loading.h"
#include "vk_application.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

void TextureManager::init(VK_INIT_ENGINE::_inited_engine& _init){
    VmaPoolCreateInfo poolCreateInfo {};

    _device = _init._device;
    _allocator = _init._allocator;

    VkDescriptorSetLayoutBinding binding{};
    binding.binding = 0;
    binding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    binding.descriptorCount = MAX_BINDLESS_TEXTURES;
    binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    VkDescriptorBindingFlags flags = VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT | VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT;
    VkDescriptorSetLayoutBindingFlagsCreateInfo extInfo{ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO };
    extInfo.bindingCount = 1;
    extInfo.pBindingFlags = &flags;

    VkDescriptorSetLayoutCreateInfo layoutInfo{ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
    layoutInfo.pNext = &extInfo;
    layoutInfo.flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT;
    layoutInfo.bindingCount = 1;
    layoutInfo.pBindings = &binding;
    vkCreateDescriptorSetLayout(_device, &layoutInfo, nullptr, &_textureLayout);

    VkDescriptorPoolSize poolSize{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, MAX_BINDLESS_TEXTURES };
    VkDescriptorPoolCreateInfo poolInfo{ VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO };
    poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT;
    poolInfo.maxSets = 1; // Только один сет!
    poolInfo.poolSizeCount = 1;
    poolInfo.pPoolSizes = &poolSize;
    vkCreateDescriptorPool(_device, &poolInfo, nullptr, &_texturePool);

    VkDescriptorSetAllocateInfo allocInfo{ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
    allocInfo.descriptorPool = _texturePool;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = &_textureLayout;
    vkAllocateDescriptorSets(_device, &allocInfo, &_textureSet);

    // Находим нужный тип памяти
    VkImageCreateInfo dummyImageInfo{ VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO };
    dummyImageInfo.imageType = VK_IMAGE_TYPE_2D;
    dummyImageInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
    dummyImageInfo.extent = { 1024, 1024, 1 }; // Обычный размер
    dummyImageInfo.mipLevels = 1;
    dummyImageInfo.arrayLayers = 1;
    dummyImageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    dummyImageInfo.tiling = VK_IMAGE_TILING_OPTIMAL; // Важно: Optimal Tiling!
    dummyImageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;

    VmaAllocationCreateInfo alloc_create_info{};
    alloc_create_info.flags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;
    alloc_create_info.usage = VMA_MEMORY_USAGE_AUTO;

    uint32_t memoryTypeIndex;
    vmaFindMemoryTypeIndexForImageInfo(_allocator, &dummyImageInfo, &alloc_create_info, &memoryTypeIndex);

    poolCreateInfo.memoryTypeIndex = memoryTypeIndex;
    poolCreateInfo.blockSize = 128 * 1024 * 1024;
    poolCreateInfo.minBlockCount = 1;

    // СОЗДАНИЕ VMA-ARENA
    vmaCreatePool(_init._allocator, &poolCreateInfo, &_textureArena);

    VkPhysicalDeviceProperties properties{};
    vkGetPhysicalDeviceProperties(_init._chosenGPU, &properties);

    VkSamplerCreateInfo samplerInfo{ VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO };
    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;

    // MIPMAP_MODE_LINEAR активирует трилинейную фильтрацию
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;

    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;

    // Границы переключения уровней детализации (Level of Detail)
    samplerInfo.minLod = 0.0f;

    // Константа VK_LOD_CLAMP_NONE указывает Vulkan автоматически адаптироваться под любой размер текстуры.
    samplerInfo.maxLod = VK_LOD_CLAMP_NONE;

    samplerInfo.mipLodBias = 0.0f;

    if (properties.limits.maxSamplerAnisotropy > 1.0f) {
        samplerInfo.anisotropyEnable = VK_TRUE;
        samplerInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy;
    } else {
        samplerInfo.anisotropyEnable = VK_FALSE;
        samplerInfo.maxAnisotropy = 1.0f;
    }

    samplerInfo.compareEnable = VK_FALSE;
    samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
    samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    samplerInfo.unnormalizedCoordinates = VK_FALSE;

    vkCreateSampler(_device, &samplerInfo, nullptr, &_defaultSampler);

    create_default_white_texture(_init);
}

GPUTexture TextureManager::AllocateTexture(VkImageCreateInfo imageInfo,
        VkImageViewCreateInfo viewInfo,
        const SamplerOptions& params,
        ModelLifetime lifetime){
    GPUTexture texture{};
    texture.lifetime = lifetime;

    texture.mipLevels = imageInfo.mipLevels;
    imageInfo.mipLevels = texture.mipLevels;
    imageInfo.usage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;

    texture.sampler = CreateSampler(params);

    // Получить bindless индекс
    if (!_freeIndices.empty()) {
        texture.globalIndex = _freeIndices.back();
        _freeIndices.pop_back();
    } else {
        texture.globalIndex = _nextIndex++;
        if (texture.globalIndex >= MAX_BINDLESS_TEXTURES) {
            fmt::print("[ENGINE CRITICAL ERROR]:  Texture index out of range!");
        }
    }

    VmaAllocationCreateInfo poolAllocInfo{};
    poolAllocInfo.usage = VMA_MEMORY_USAGE_AUTO;
    if (lifetime == ModelLifetime::Static){
        poolAllocInfo.pool = _textureArena;
    }
    else if (lifetime == ModelLifetime::Dynamic)
    {
        poolAllocInfo.pool = VK_NULL_HANDLE;
    }

    vmaCreateImage(_allocator, &imageInfo, &poolAllocInfo, &texture.image.image, &texture.image.allocation, nullptr);

    viewInfo.image = texture.image.image;
    viewInfo.subresourceRange.levelCount = texture.mipLevels;
    vkCreateImageView(_device, &viewInfo, nullptr, &texture.image.imageView);

    VkDescriptorImageInfo descriptorImgInfo{};
    descriptorImgInfo.imageView = texture.image.imageView;
    descriptorImgInfo.sampler = texture.sampler;
    descriptorImgInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    VkWriteDescriptorSet write{ VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
    write.dstSet = _textureSet;
    write.dstBinding = 0;
    write.dstArrayElement = texture.globalIndex;
    write.descriptorCount = 1;
    write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    write.pImageInfo = &descriptorImgInfo;

    vkUpdateDescriptorSets(_device, 1, &write, 0, nullptr);

    texture.imguiDescriptorSet = ImGui_ImplVulkan_AddTexture(
        texture.sampler,
        texture.image.imageView,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
    );

    return texture;
}

void TextureManager::FreeTexture(GPUTexture& texture){
    if (texture.globalIndex == 0) {return;}

    if (texture.image.imageView != VK_NULL_HANDLE) {
        vkDestroyImageView(_device, texture.image.imageView, nullptr);
    }

    if (texture.image.image != VK_NULL_HANDLE) {
        vmaDestroyImage(_allocator, texture.image.image, texture.image.allocation);
    }

    if (texture.imguiDescriptorSet != VK_NULL_HANDLE) {
        ImGui_ImplVulkan_RemoveTexture(texture.imguiDescriptorSet);
        texture.imguiDescriptorSet = VK_NULL_HANDLE;
    }

    // Возвращаем индекс в пул свободных для переиспользования
    _freeIndices.push_back(texture.globalIndex);
}

void TextureManager::DestroyAllocationData(){
    if (defaultTexture.imguiDescriptorSet != VK_NULL_HANDLE) {
        ImGui_ImplVulkan_RemoveTexture(defaultTexture.imguiDescriptorSet);
        defaultTexture.imguiDescriptorSet = VK_NULL_HANDLE;
    }

    // Уничтожаем хэш сэмплеров и сами сэмплеры
    for (auto& [info, sampler] : _samplerCache) {
        if (sampler != VK_NULL_HANDLE) {
            vkDestroySampler(_device, sampler, nullptr);
        }
    }
    _samplerCache.clear();
    if (_defaultSampler) vkDestroySampler(_device, _defaultSampler, nullptr);
    if (defaultTexture.image.imageView != VK_NULL_HANDLE) {
        vkDestroyImageView(_device, defaultTexture.image.imageView, nullptr);
    }
    if (defaultTexture.image.image != VK_NULL_HANDLE) {
        vmaDestroyImage(_allocator, defaultTexture.image.image, defaultTexture.image.allocation);
        defaultTexture.image.image = VK_NULL_HANDLE;
    }


    if (_textureArena) {
        vmaDestroyPool(_allocator, _textureArena);
        _textureArena = VK_NULL_HANDLE;
    }

    if (_texturePool) vkDestroyDescriptorPool(_device, _texturePool, nullptr);
    if (_textureLayout) vkDestroyDescriptorSetLayout(_device, _textureLayout, nullptr);

    _nextIndex = 0;
    _freeIndices.clear();
}

void TextureManager::create_default_white_texture(VK_INIT_ENGINE::_inited_engine& _init){
    int32_t whitePixel = 0xFFFFFFFF;
    size_t dataSize = sizeof(whitePixel);

    AllocatedBuffer stagingBuffer = vkinit::create_buffer(
        dataSize,
        _init._allocator,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VMA_MEMORY_USAGE_CPU_TO_GPU
    );

    // Копируем пиксель в staging-буфер
    void* data;
    vmaMapMemory(_init._allocator, stagingBuffer.allocation, &data);
    memcpy(data, &whitePixel, dataSize);
    vmaUnmapMemory(_init._allocator, stagingBuffer.allocation);

    VkImageCreateInfo imgInfo{ VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO };
    imgInfo.imageType = VK_IMAGE_TYPE_2D;
    imgInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
    imgInfo.extent = { 1, 1, 1 };
    imgInfo.mipLevels = 1;
    imgInfo.arrayLayers = 1;
    imgInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imgInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imgInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;

    VkImageViewCreateInfo viewInfo{ VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.layerCount = 1;

    SamplerOptions params = {};
    defaultTexture = AllocateTexture(imgInfo, viewInfo, params, ModelLifetime::Static);

    vkinit::submit_immediate([&](VkCommandBuffer cmd) {
        VkImageMemoryBarrier barrier{ VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
        barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.image = defaultTexture.image.image;
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange.baseMipLevel = 0;
        barrier.subresourceRange.levelCount = 1;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = 1;
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

        vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);

        VkBufferImageCopy copyRegion{};
        copyRegion.bufferOffset = 0;
        copyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        copyRegion.imageSubresource.mipLevel = 0;
        copyRegion.imageSubresource.baseArrayLayer = 0;
        copyRegion.imageSubresource.layerCount = 1;
        copyRegion.imageExtent = imgInfo.extent;

        vkCmdCopyBufferToImage(cmd, stagingBuffer.buffer, defaultTexture.image.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copyRegion);

        VkImageMemoryBarrier shaderBarrier{ VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
        shaderBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        shaderBarrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        shaderBarrier.image = defaultTexture.image.image;
        shaderBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        shaderBarrier.subresourceRange.baseMipLevel = 0;
        shaderBarrier.subresourceRange.levelCount = 1;
        shaderBarrier.subresourceRange.baseArrayLayer = 0;
        shaderBarrier.subresourceRange.layerCount = 1;
        shaderBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        shaderBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        vkCmdPipelineBarrier(cmd,
            VK_PIPELINE_STAGE_TRANSFER_BIT,
            VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
            0, 0, nullptr, 0, nullptr, 1, &shaderBarrier
        );



    }, _init);

    // Очищаем временный буфер
    vmaDestroyBuffer(_init._allocator, stagingBuffer.buffer, stagingBuffer.allocation);

    std::cout << "TextureManager: Default white texture generated under Bindless ID = 0\n";
}

VkSampler TextureManager::CreateSampler(const SamplerOptions& params){
    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = vkutil::GetVkFilter(params.magFilter);
    samplerInfo.minFilter = vkutil::GetVkFilter(params.minFilter);
    samplerInfo.mipmapMode = vkutil::GetVkMipmapMode(params.minFilter);
    samplerInfo.addressModeU = vkutil::GetVkAddressMode(params.wrapS);
    samplerInfo.addressModeV = vkutil::GetVkAddressMode(params.wrapT);
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;

    // включаем анизотропию для всех 3D текстур
    samplerInfo.anisotropyEnable = VK_TRUE;
    samplerInfo.maxAnisotropy = 16.0f;

    // Поиск по кешу
    auto it = _samplerCache.find(samplerInfo);
    if (it != _samplerCache.end()) {
        return it->second;
    }

    // Нет в кеше - создаём новый
    VkSampler newSampler;
    if (vkCreateSampler(_device, &samplerInfo, nullptr, &newSampler) != VK_SUCCESS) {
        throw std::runtime_error("[ENGINE CRITICAL ERROR]: Can't create sampler");
    }

    _samplerCache[samplerInfo] = newSampler;
    return newSampler;
}

void Node::AddChild(std::shared_ptr<Node> child){
    child->parent = this;
    children.push_back(child);
}

void Node::UpdateMatrices(const glm::mat4& parentMatrix){
    worldTransform = parentMatrix * localTransform;

    for (auto& child : children) {
        child->UpdateMatrices(worldTransform);
    }
}

void Model::destroy(VK_INIT_ENGINE::_inited_engine& _init, MeshManager& meshManager, TextureManager& textureManager){
    for (auto& tex : loadedTextures) {
        if (tex.globalIndex == 0) continue;
        textureManager.FreeTexture(tex);
    }
    loadedTextures.clear();
    materials.clear();

    for (auto& mesh : Meshes) {
        meshManager.FreeMesh(mesh->meshBuffers);
    }

    Meshes.clear();
    meshNodes.clear();
    rootNode.reset();
}

void MeshManager::init(VK_INIT_ENGINE::_inited_engine& _init){
    _device = _init._device;
    _allocator = _init._allocator;

    VkBufferCreateInfo dummyInfo{ VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
    dummyInfo.size = 1024;
    dummyInfo.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
                      VK_BUFFER_USAGE_INDEX_BUFFER_BIT |
                      VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;

    VmaAllocationCreateInfo dummyAllocInfo{};
    dummyAllocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;

    uint32_t memoryTypeIndex;
    vmaFindMemoryTypeIndexForBufferInfo(_allocator, &dummyInfo, &dummyAllocInfo, &memoryTypeIndex);

    VmaPoolCreateInfo poolCreateInfo{};
    poolCreateInfo.memoryTypeIndex = memoryTypeIndex;
    poolCreateInfo.blockSize = MAX_ARENA_SIZE; // Выделяем память
    poolCreateInfo.minBlockCount = 1;
    poolCreateInfo.maxBlockCount = 1;

    vmaCreatePool(_allocator, &poolCreateInfo, &_meshArena);
}

void MeshManager::DestroyAllocationData(){

    if (_meshArena != VK_NULL_HANDLE) {
        vmaDestroyPool(_allocator, _meshArena);
        _meshArena = VK_NULL_HANDLE;
    }
}

void MeshManager::FreeMesh(const GPUMeshBuffers& buffers){
    if (buffers.vertexBuffer.buffer == VK_NULL_HANDLE || buffers.indexBuffer.buffer == VK_NULL_HANDLE) {
        return;
    }

    if (buffers.lifetime == ModelLifetime::Static){
        vmaDestroyBuffer(_allocator, buffers.vertexBuffer.buffer, buffers.vertexBuffer.allocation);
        vmaDestroyBuffer(_allocator, buffers.indexBuffer.buffer, buffers.indexBuffer.allocation);
    }
    else if (buffers.lifetime == ModelLifetime::Dynamic){
        vkinit::destroy_buffer(buffers.vertexBuffer, _allocator);
        vkinit::destroy_buffer(buffers.indexBuffer, _allocator);
    }
}

GPUMeshBuffers MeshManager::upload_meshes(VK_INIT_ENGINE::_inited_engine& _init, std::span<uint32_t> indices,
                                          std::span<Vertex> vertices, ModelLifetime lifetime){
    // Размер масивов с данными, чтобы программа знала точное количество
    const size_t vertexBufferSize = vertices.size() * sizeof(Vertex);
    const size_t indexBufferSize = indices.size() * sizeof(uint32_t);

    // Тут мы храним:
    // 1. Буфер индексов
    // 2. Буфер вершин
    // 3. Адресс на всю эту бурмалду
    GPUMeshBuffers newSurface;
    newSurface.lifetime = lifetime;

    // Создание информации кто читает вершинный буфер
    VkBufferCreateInfo vertexBufferInfo{ VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
    vertexBufferInfo.size = vertexBufferSize;
    vertexBufferInfo.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;

    // Создание информации кто читает индескный буфер
    VkBufferCreateInfo indexBufferInfo{ VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
    indexBufferInfo.size = indexBufferSize;
    indexBufferInfo.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;

    // Чтение только GPU
    VmaAllocationCreateInfo allocCreateInfo{};
    allocCreateInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;

    // Статическая память
    if (lifetime == ModelLifetime::Static && _meshArena != VK_NULL_HANDLE) {

        // Нарезаем память из выделенного при старте пула
        allocCreateInfo.pool = _meshArena;

        // Создаем под-буферы внутри Арены мешей
        vmaCreateBuffer(_init._allocator, &vertexBufferInfo, &allocCreateInfo, &newSurface.vertexBuffer.buffer, &newSurface.vertexBuffer.allocation, nullptr);
        vmaCreateBuffer(_init._allocator, &indexBufferInfo, &allocCreateInfo, &newSurface.indexBuffer.buffer, &newSurface.indexBuffer.allocation, nullptr);
    }
    // Динамическая память
    else {
        // Аллоцируем абсолютно новую память
        allocCreateInfo.pool = VK_NULL_HANDLE; // Используем уже обычный VMA

        // Используем старую реализацию из vk-guide для создания буфера через VMA
        newSurface.vertexBuffer = vkinit::create_buffer(vertexBufferSize, _init._allocator, vertexBufferInfo.usage, VMA_MEMORY_USAGE_GPU_ONLY);
        newSurface.indexBuffer = vkinit::create_buffer(indexBufferSize, _init._allocator, indexBufferInfo.usage, VMA_MEMORY_USAGE_GPU_ONLY);
    }

    // Находим адресс к вершиному буферу
    VkBufferDeviceAddressInfo deviceAdressInfo{ .sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,.buffer = newSurface.vertexBuffer.buffer };
    // Заносим этот адресс в структуру
    newSurface.vertexBufferAddress = vkGetBufferDeviceAddress(_init._device, &deviceAdressInfo);


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

    newSurface.lifetime = lifetime;

    // Возвращаем
    return newSurface;
}

std::optional<GPUTexture> load_image(VK_INIT_ENGINE::_inited_engine& _init, TextureManager& textureManager,
    const unsigned char* pixelData, uint32_t width, uint32_t height, VkFormat format,
    SamplerOptions samplerParams, ModelLifetime lifetime){

    size_t dataSize = static_cast<size_t>(width) * height * 4; // Предпокаем RGBA8

    AllocatedBuffer stagingBuffer = vkinit::create_buffer(
        dataSize,
        _init._allocator,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VMA_MEMORY_USAGE_CPU_TO_GPU
    );

    // Копируем пиксели в staging-буфер
    void* data;
    vmaMapMemory(_init._allocator, stagingBuffer.allocation, &data);
    memcpy(data, pixelData, dataSize);
    vmaUnmapMemory(_init._allocator, stagingBuffer.allocation);

    uint32_t mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(width, height)))) + 1;

    // Настраиваем инфо для оптимальной текстуры на GPU
    VkImageCreateInfo imgInfo{ VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO };
    imgInfo.imageType = VK_IMAGE_TYPE_2D;
    imgInfo.format = format;
    imgInfo.extent = { width, height, 1 };
    imgInfo.mipLevels = mipLevels;
    imgInfo.arrayLayers = 1;
    imgInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imgInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imgInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;

    VkImageViewCreateInfo viewInfo{ VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = format;
    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    viewInfo.subresourceRange.levelCount = mipLevels;
    viewInfo.subresourceRange.layerCount = 1;

    // Выделяем память из VMA Арены и регистрируем в Bindless-сет
    GPUTexture outTexture = textureManager.AllocateTexture(imgInfo, viewInfo, samplerParams, lifetime);

    // Отправляем команды копирования на GPU через submit_immediate
    vkinit::submit_immediate([&](VkCommandBuffer cmd) {
        VkImageMemoryBarrier barrier{ VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
        barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.image = outTexture.image.image;
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange.levelCount = outTexture.mipLevels;
        barrier.subresourceRange.layerCount = 1;
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

        vkCmdPipelineBarrier(cmd,
            VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
            VK_PIPELINE_STAGE_TRANSFER_BIT,
            0, 0, nullptr, 0, nullptr, 1, &barrier);

        VkBufferImageCopy copyRegion{};
        copyRegion.bufferOffset = 0;
        copyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        copyRegion.imageSubresource.mipLevel = 0;
        copyRegion.imageSubresource.layerCount = 1;
        copyRegion.imageExtent = imgInfo.extent;

        vkCmdCopyBufferToImage(cmd, stagingBuffer.buffer, outTexture.image.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copyRegion);

        vkinit::generate_mipmaps(cmd, outTexture.image.image, width, height, outTexture.mipLevels);

    }, _init);

    // Очищаем временный staging-буфер
    vmaDestroyBuffer(_init._allocator, stagingBuffer.buffer, stagingBuffer.allocation);

    return outTexture;
}

std::optional<std::vector<std::shared_ptr<MeshAsset>>> load_Meshes(VK_INIT_ENGINE::_inited_engine& _init,
                            MeshManager& meshManager, fastgltf::Asset& asset,
                            const std::vector<std::shared_ptr<MaterialAsset>>& materials,
                            ModelLifetime lifetime){
    std::vector<std::shared_ptr<MeshAsset>> meshes;
    std::vector<uint32_t> indices;
    std::vector<Vertex> vertices;

    for (size_t meshIdx = 0; meshIdx < asset.meshes.size(); ++meshIdx) {
        fastgltf::Mesh& gltfMesh = asset.meshes[meshIdx];

        auto newMeshAsset = std::make_shared<MeshAsset>();
        newMeshAsset->name = gltfMesh.name.c_str();

        indices.clear();
        vertices.clear();

        AABB totalMeshAABB;

        for (auto&& p : gltfMesh.primitives) {
            GeoSurface newSurface;
            newSurface.startIndex = static_cast<uint32_t>(indices.size());
            newSurface.count = static_cast<uint32_t>(asset.accessors[p.indicesAccessor.value()].count);

            if (p.materialIndex.has_value()) {
                size_t matIdx = p.materialIndex.value();
                // Забираем материал из вектора, который мы наполнили в load_glTF
                newSurface.material = materials[matIdx];
            } else {
                // Если у меша в glTF нет материала, создаем дефолтную пустышку с текстурой ID = 0
                newSurface.material = std::make_shared<MaterialAsset>();
                newSurface.material->name = "Default_Fallback_Material";
                newSurface.material->baseColorFactor = glm::vec4(1.0f);
                newSurface.material->colorTextureID = 0;
                newSurface.material->metallicRoughnessTextureID = 0;
            }

            size_t initial_vtx = vertices.size();

            // 1. Загрузка индексов
            {
                fastgltf::Accessor& indexaccessor = asset.accessors[p.indicesAccessor.value()];
                indices.reserve(indices.size() + indexaccessor.count);

                fastgltf::iterateAccessor<std::uint32_t>(asset, indexaccessor,
                    [&](std::uint32_t idx) {
                        indices.push_back(idx + static_cast<uint32_t>(initial_vtx));
                    });
            }

            // 2. Загрузка позиций вершин + Извлечение AABB саб-меша
            {
                auto* positionAttribute = p.findAttribute("POSITION");
                if (positionAttribute == nullptr) {
                    fmt::print("Failed to find position attribute\n");
                } else {
                    fastgltf::Accessor& posAccessor = asset.accessors[positionAttribute->accessorIndex];
                    vertices.resize(vertices.size() + posAccessor.count);

                    if (posAccessor.min.has_value() && posAccessor.max.has_value()) {
                        auto& minVals = posAccessor.min.value();
                        auto& maxVals = posAccessor.max.value();

                        newSurface.localAABB.min = glm::vec3(static_cast<float>(minVals.get<double>(0)),
                                                             static_cast<float>(minVals.get<double>(1)),
                                                             static_cast<float>(minVals.get<double>(2)));

                        newSurface.localAABB.max = glm::vec3(static_cast<float>(maxVals.get<double>(0)),
                                                             static_cast<float>(maxVals.get<double>(1)),
                                                             static_cast<float>(maxVals.get<double>(2)));
                    } else {
                        // если в glTF нет min/max границ
                        newSurface.localAABB.min = glm::vec3(std::numeric_limits<float>::infinity());
                        newSurface.localAABB.max = glm::vec3(-std::numeric_limits<float>::infinity());
                    }

                    fastgltf::iterateAccessorWithIndex<glm::vec3>(asset, posAccessor,
                        [&](glm::vec3 v, size_t index) {
                            Vertex newvtx;
                            newvtx.position = v;
                            newvtx.normal = { 1.0f, 0.0f, 0.0f };
                            newvtx.color = glm::vec4 { 1.f };
                            newvtx.uv_x = 0.0f;
                            newvtx.uv_y = 0.0f;
                            vertices[initial_vtx + index] = newvtx;
                        });
                }
            }

            // 3. Загрузка нормалей
            auto normals = p.findAttribute("NORMAL");
            if (normals != p.attributes.end()) {
                fastgltf::iterateAccessorWithIndex<glm::vec3>(asset, asset.accessors[normals->accessorIndex],
                    [&](glm::vec3 v, size_t index) {
                        vertices[initial_vtx + index].normal = v;
                    });
            }

            // 4. Загрузка UV-координат
            auto uv = p.findAttribute("TEXCOORD_0");
            if (uv != p.attributes.end()) {
                fastgltf::iterateAccessorWithIndex<glm::vec2>(asset, asset.accessors[uv->accessorIndex],
                    [&](glm::vec2 v, size_t index) {
                        vertices[initial_vtx + index].uv_x = v.x;
                        vertices[initial_vtx + index].uv_y = v.y;
                    });
            }

            // 5. Загрузка цветов вершин
            auto colors = p.findAttribute("COLOR_0");
            if (colors != p.attributes.end()) {
                fastgltf::iterateAccessorWithIndex<glm::vec4>(asset, asset.accessors[colors->accessorIndex],
                    [&](glm::vec4 v, size_t index) {
                        vertices[initial_vtx + index].color = v;
                    });
            }

            // Обьединяем AABB текущего саб-меша в один большой меш-ассет AABB
            totalMeshAABB.min = glm::min(totalMeshAABB.min, newSurface.localAABB.min);
            totalMeshAABB.max = glm::max(totalMeshAABB.max, newSurface.localAABB.max);

            newMeshAsset->surfaces.push_back(newSurface);
        }

        newMeshAsset->localAABB = totalMeshAABB;

        // Визуализация нормалей через цвета
        constexpr bool OverrideColors = false;
        if (OverrideColors) {
            for (Vertex& vtx : vertices) {
                vtx.color = glm::vec4(vtx.normal, 1.f);
            }
        }

        // Загружаем буферы в Vulkan для конкретно этого меша
        newMeshAsset->meshBuffers = meshManager.upload_meshes(_init, indices, vertices, lifetime);
        meshes.push_back(newMeshAsset);
    }

    return meshes;
}

std::optional<std::shared_ptr<Node>> load_Node(fastgltf::Asset& asset, fastgltf::Node& gltfNode, Model& outModel){
    std::shared_ptr<Node> currentEngineNode;

    // 1. Проверяем, привязан ли к этой ноде меш в glTF
    if (gltfNode.meshIndex.has_value()) {
        auto meshNode = std::make_shared<MeshNode>();

        // Достаем индекс меша из glTF
        size_t meshIdx = gltfNode.meshIndex.value();

        // Связываем ноду с уже загруженным MeshAsset по этому индексу
        meshNode->mesh = outModel.Meshes[meshIdx];

        // Запоминаем её в плоском списке модели для моментального рендера без обхода дерева
        outModel.meshNodes.push_back(meshNode);

        currentEngineNode = meshNode;
    } else {
        // Если это техническая нода для группировки или пустой Pivot-объект
        currentEngineNode = std::make_shared<Node>();
    }

    // 2. Достаем локальную матрицу трансформации из glTF
    // fastgltf::getTransformMatrix автоматически собирает полноценную glm::mat4
    // вне зависимости от того, как записаны данные в файле (сразу матрицей или через Translation/Rotation/Scale)
    auto gltfMatrixData = fastgltf::getTransformMatrix(gltfNode);
    currentEngineNode->localTransform = glm::make_mat4(gltfMatrixData.data());

    // 3. Рекурсивно спускаемся ко всем дочерним узлам (Children)
    for (size_t childIndex : gltfNode.children) {
        auto childNodeOpt = load_Node(asset, asset.nodes[childIndex], outModel);
        if (childNodeOpt.has_value()) {
            // Добавляем ребенка в иерархию (метод AddChild пропишет parent указатель)
            currentEngineNode->AddChild(childNodeOpt.value());
        }
    }

    return currentEngineNode;
}

AABB transformAABB(const AABB& localBox, const glm::mat4& M){
    AABB worldBox;

    // Позиция смещения (translation) берется из 4-го столбца матрицы
    glm::vec3 translation = glm::vec3(M[3]);
    worldBox.min = translation;
    worldBox.max = translation;

    // Обходим оси координат и пересчитываем новые границы
    for (int i = 0; i < 3; ++i) {
        for (int j = 0; j < 3; ++j) {
            float a = M[j][i] * localBox.min[j];
            float b = M[j][i] * localBox.max[j];

            worldBox.min[i] += std::min(a, b);
            worldBox.max[i] += std::max(a, b);
        }
    }
    return worldBox;
}

void calculate_model_bounds(Node* node, const glm::mat4& parentTransform, AABB& outTotalAABB){
    if (!node) return;

    // Считаем трансформацию текущей ноды относительно корня модели
    glm::mat4 currentTransform = parentTransform * node->localTransform;

    // Проверяем, является ли текущая нода мешем (MeshNode)
    if (auto* meshNode = dynamic_cast<MeshNode*>(node)) {
        // Проверяем, что к ноде действительно привязан ассет меша
        if (meshNode->mesh) {
            // Трансформируем локальный AABB меша
            // с учетом накопленной матрицы трансформации этой ноды
            AABB transformedBox = transformAABB(meshNode->mesh->localAABB, currentTransform);

            // Расширяем глобальный AABB модели, беря минимальные и максимальные
            // координаты среди всех уже обработанных саб-мешей
            outTotalAABB.min = glm::min(outTotalAABB.min, transformedBox.min);
            outTotalAABB.max = glm::max(outTotalAABB.max, transformedBox.max);
        }
    }

    // Рекурсивно спускаемся вниз к детям, передавая им текущую матрицу в качестве родительской
    for (auto& child : node->children) {
        calculate_model_bounds(child.get(), currentTransform, outTotalAABB);
    }
}

Model load_glTF(VK_INIT_ENGINE::_inited_engine& _init,
                MeshManager& meshManager, TextureManager& textureManager, std::filesystem::path filePath,
                ModelLifetime lifetime, bool useArena){
    // Возвращаемая модель
    Model loadedModel;
    loadedModel.lifetime = lifetime;
    loadedModel.bIsValid = true;
    std::cout << "Loading GLTF: " << filePath << std::endl;
    std::cout<< "Model type: ";
    if (lifetime == ModelLifetime::Dynamic){
        std::cout<<"Dynamic\n";
    }
    else if (lifetime == ModelLifetime::Static){
        std::cout<<"Static\n";
    }
    // Чтение данных
    auto data = fastgltf::GltfDataBuffer::FromPath(filePath.string());
    if (!data) {
        fmt::print("Failed to open glTF file path: {}\n", filePath.string());
        return loadedModel; // Возвращаем пустую модель
    }

    constexpr auto gltfOptions = fastgltf::Options::LoadGLBBuffers | fastgltf::Options::LoadExternalBuffers;
    fastgltf::Parser parser {};

    auto load = parser.loadGltf(data.get(), filePath.parent_path(), gltfOptions);
    if (!load) {
        fmt::print("Failed to parse glTF JSON: {} \n", fastgltf::to_underlying(load.error()));
        return loadedModel;
    }

    fastgltf::Asset& gltf = load.get();

    std::cout << "GLTF Asset Parsed Info:" << std::endl;
    std::cout << "  - Images: " << gltf.images.size() << std::endl;
    std::cout << "  - Textures: " << gltf.textures.size() << std::endl;
    std::cout << "  - Materials: " << gltf.materials.size() << std::endl;
    std::cout << "  - Meshes: " << gltf.meshes.size() << std::endl;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Загрузка изображений

    for (size_t imgIdx = 0; imgIdx < gltf.images.size(); ++imgIdx) {
        auto& gltfImage = gltf.images[imgIdx];
        int width = 0, height = 0, channels = 0;
        unsigned char* pixelData = nullptr;

        // Вариант А: Картинка зашита внутри BufferView (самый частый случай для .glb файлов)
        if (auto* viewSource = std::get_if<fastgltf::sources::BufferView>(&gltfImage.data)) {
            auto& bufferView = gltf.bufferViews[viewSource->bufferViewIndex];
            auto& buffer = gltf.buffers[bufferView.bufferIndex];

            // Проверяем, загружен ли буфер как сырой массив байт в память
            if (auto* arraySource = std::get_if<fastgltf::sources::Array>(&buffer.data)) {
                const unsigned char* rawData = reinterpret_cast<const unsigned char*>(arraySource->bytes.data()) + bufferView.byteOffset;
                pixelData = stbi_load_from_memory(rawData, static_cast<int>(bufferView.byteLength), &width, &height, &channels, 4);
            }
        }
        // Вариант Б: Картинка лежит как отдельный массив байт (например, base64 или внешний файл, уже считанный fastgltf)
        else if (auto* arraySource = std::get_if<fastgltf::sources::Array>(&gltfImage.data)) {
            pixelData = stbi_load_from_memory(
                reinterpret_cast<const unsigned char*>(arraySource->bytes.data()),
                static_cast<int>(arraySource->bytes.size()),
                &width, &height, &channels, 4
            );
        }

        // Если пиксели успешно раскодированы — отправляем их в нашу изолированную load_image
        if (pixelData) {
            SamplerOptions samplerParams{};

            for (const auto& gltfTex : gltf.textures) {
                if (gltfTex.imageIndex.has_value() && gltfTex.imageIndex.value() == imgIdx) {
                    if (gltfTex.samplerIndex.has_value()) {
                        auto& gltfSampler = gltf.samplers[gltfTex.samplerIndex.value()];

                        // Используем fastgltf::to_underlying для безопасного извлечения ID из enum class
                        if (gltfSampler.minFilter.has_value()) {
                            samplerParams.minFilter = static_cast<int>(fastgltf::to_underlying(gltfSampler.minFilter.value()));
                        }
                        if (gltfSampler.magFilter.has_value()) {
                            samplerParams.magFilter = static_cast<int>(fastgltf::to_underlying(gltfSampler.magFilter.value()));
                        }

                        samplerParams.wrapS = static_cast<int>(fastgltf::to_underlying(gltfSampler.wrapS));
                        samplerParams.wrapT = static_cast<int>(fastgltf::to_underlying(gltfSampler.wrapT));
                    }
                    break;
                }
            }

            VkFormat imageFormat = VK_FORMAT_R8G8B8A8_SRGB;

            // Короче для материалов нужен формат UNORM
            for (const auto& mat : gltf.materials) {
                if (mat.normalTexture.has_value() && gltf.textures[mat.normalTexture->textureIndex].imageIndex == imgIdx) {
                    imageFormat = VK_FORMAT_R8G8B8A8_UNORM;
                }
                if (mat.pbrData.metallicRoughnessTexture.has_value() && gltf.textures[mat.pbrData.metallicRoughnessTexture->textureIndex].imageIndex == imgIdx) {
                    imageFormat = VK_FORMAT_R8G8B8A8_UNORM;
                }
            }

            auto gpuTex = load_image(_init, textureManager, pixelData, width, height, imageFormat, samplerParams, lifetime);

            stbi_image_free(pixelData);

            if (gpuTex.has_value()){
                gpuTex.value().lifetime = lifetime;
                loadedModel.loadedTextures.push_back(gpuTex.value());
                continue;
            }
        }
        else {
            std::cout << "[ENGINE CRITICAL ERROR]: fastgltf/stbi failed to load image data for one of the textures!\n";
        }
        GPUTexture dummyTex{};
        dummyTex.globalIndex = 0;
        dummyTex.lifetime = ModelLifetime::Static;
        loadedModel.loadedTextures.push_back(dummyTex);
    }


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
   // Загрузка материалов
    for (auto& gltfMaterial : gltf.materials) {
        auto newMaterial = std::make_shared<MaterialAsset>();
        newMaterial->name = gltfMaterial.name.c_str();
        newMaterial->baseColorFactor = glm::make_vec4(gltfMaterial.pbrData.baseColorFactor.data());

        newMaterial->roughnessFactor = gltfMaterial.pbrData.roughnessFactor;
        newMaterial->metallicFactor  = gltfMaterial.pbrData.metallicFactor;

        // Базовый цвет (Albedo)
        if (gltfMaterial.pbrData.baseColorTexture.has_value()) {
            size_t gltfTextureIdx = gltfMaterial.pbrData.baseColorTexture->textureIndex;
            auto imageIndexOpt = gltf.textures[gltfTextureIdx].imageIndex;
            if (imageIndexOpt.has_value()) {
                size_t imgIdx = imageIndexOpt.value();
                newMaterial->colorTextureID = loadedModel.loadedTextures[imgIdx].globalIndex;
            }
        } else {
            newMaterial->colorTextureID = 0; // Заглушка
        }

        // Metallic / Roughness карту
        if (gltfMaterial.pbrData.metallicRoughnessTexture.has_value()) {
            size_t gltfTextureIdx = gltfMaterial.pbrData.metallicRoughnessTexture->textureIndex;
            auto imageIndexOpt = gltf.textures[gltfTextureIdx].imageIndex;
            if (imageIndexOpt.has_value()) {
                size_t imgIdx = imageIndexOpt.value();
                newMaterial->metallicRoughnessTextureID = loadedModel.loadedTextures[imgIdx].globalIndex;
            }
        } else {
            newMaterial->metallicRoughnessTextureID = 0; // Заглушка
        }

        // Карта Нормалей (Normal Map) — ДОБАВЛЕНО
        if (gltfMaterial.normalTexture.has_value()) {
            size_t gltfTextureIdx = gltfMaterial.normalTexture->textureIndex;
            auto imageIndexOpt = gltf.textures[gltfTextureIdx].imageIndex;
            if (imageIndexOpt.has_value()) {
                size_t imgIdx = imageIndexOpt.value();
                newMaterial->normalTextureID = loadedModel.loadedTextures[imgIdx].globalIndex;
            }
        } else {
            // Идеально вернуть ID нежно-голубой заглушки (например, 1), если она создана в TextureManager
            newMaterial->normalTextureID = 0;
        }

        // Карта Окклюзии
        if (gltfMaterial.occlusionTexture.has_value()) {
            size_t gltfTextureIdx = gltfMaterial.occlusionTexture->textureIndex;
            auto imageIndexOpt = gltf.textures[gltfTextureIdx].imageIndex;
            if (imageIndexOpt.has_value()) {
                size_t imgIdx = imageIndexOpt.value();
                newMaterial->occlusionTextureID = loadedModel.loadedTextures[imgIdx].globalIndex;
            }
        } else {
            newMaterial->occlusionTextureID = 0; // Заглушка
        }

        if (gltfMaterial.alphaMode == fastgltf::AlphaMode::Blend) {
            newMaterial->pipelineName = "TransparentMesh";
        }
        else if (gltfMaterial.alphaMode == fastgltf::AlphaMode::Mask) {
            newMaterial->pipelineName = "AlphaTestedMesh";
        }
        else {
            newMaterial->pipelineName = "BaseMesh";
        }

        loadedModel.materials.push_back(newMaterial);
    }
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Загрузка мешей
    auto meshesOpt = load_Meshes(_init, meshManager, gltf, loadedModel.materials, lifetime);
    if (meshesOpt.has_value()) {
        loadedModel.Meshes = std::move(meshesOpt.value());
    } else {
        fmt::print("Failed to process meshes for file: {}\n", filePath.string());
        return loadedModel;
    }

    auto baseRoot = std::make_shared<Node>();

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Загрузка Nodes, БЕЗ ЕБАННЫХ НЕСКОЛЬКИХ ROOT-NODE
    fastgltf::Scene& defaultScene = gltf.scenes[gltf.defaultScene.value_or(0)];

    // Жесткий отстрел многокорневых файлов
    if (defaultScene.nodeIndices.size() > 1) {
        fmt::print("[ENGINE CRITICAL ERROR]: Fuck you, we don't use multiple root nodes in the same model!\n",
            filePath.string(), defaultScene.nodeIndices.size());
        loadedModel.destroy(_init, meshManager, textureManager);
        loadedModel.bIsValid = false;
        return loadedModel;
    }

    // Создаем ИСКУССТВЕННЫЙ временный корень для проведения операции запекания
    auto bakeRoot = std::make_shared<Node>();

    size_t singleRootIdx = defaultScene.nodeIndices[0];
    auto gltfRootNodeOpt = load_Node(gltf, gltf.nodes[singleRootIdx], loadedModel);

    if (gltfRootNodeOpt.has_value()) {
        // Навешиваем glTF-корень художника на наш временный bakeRoot
        bakeRoot->AddChild(gltfRootNodeOpt.value());
    } else {
        loadedModel.destroy(_init, meshManager, textureManager);
        loadedModel.bIsValid = false;
        return loadedModel;
    }

    AABB totalModelAABB;
    calculate_model_bounds(bakeRoot.get(), glm::mat4(1.0f), totalModelAABB);

    // Считаем масштаб (вписываем в стандартные 2.0 метра)
    glm::vec3 modelSize = totalModelAABB.max - totalModelAABB.min;
    float maxExtent = std::max({modelSize.x, modelSize.y, modelSize.z});
    constexpr float targetSize = 2.0f;
    float scaleFactor = targetSize / maxExtent;

    float gltfCenterX = (totalModelAABB.min.x + totalModelAABB.max.x) * 0.5f;
    float gltfCenterY = (totalModelAABB.min.y + totalModelAABB.max.y) * 0.5f;

    float gltfBottomY = totalModelAABB.min.y;

    // Собираем правильный вектор смещения в пространстве glTF
    glm::vec3 modelPivotOffset(-gltfCenterX, -gltfBottomY, -gltfCenterY);

    // Строим матрицу нормализации (сначала сдвигаем пивот в пол, затем масштабируем)
    glm::mat4 normalizationMatrix = glm::scale(glm::mat4(1.0f), glm::vec3(scaleFactor)) *
                                    glm::translate(glm::mat4(1.0f), modelPivotOffset);

    // Ебанное Y-up экспортируемые glTF формат
    glm::mat4 gltfToZUp = glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));

    // Накладываем исправления на наш временный корень bakeRoot
    bakeRoot->localTransform = gltfToZUp * normalizationMatrix;

    bakeRoot->UpdateMatrices(glm::mat4(1.0f));

    glm::vec3 finalMin(std::numeric_limits<float>::max());
    glm::vec3 finalMax(-std::numeric_limits<float>::max());
    bool hasMeshes = false;

    for (auto& meshNode : loadedModel.meshNodes) {
        if (meshNode->mesh) {
            // Переносим запеченную Z-Up матрицу в локальную матрицу ноды меша
            meshNode->localTransform = meshNode->worldTransform;
            meshNode->worldTransform = glm::mat4(1.0f);

            // ОДИН РАЗ трансформируем локальный AABB меша в пространство Z-Up
            meshNode->mesh->localAABB = transformAABB(meshNode->mesh->localAABB, meshNode->localTransform);

            // СРАЗУ добавляем этот трансформированный куб в общую копилку модели
            finalMin = glm::min(finalMin, meshNode->mesh->localAABB.min);
            finalMax = glm::max(finalMax, meshNode->mesh->localAABB.max);
            hasMeshes = true;

            // Изолируем Node - у нее больше нет родителей из glTF файла, она самодостаточна
            meshNode->parent = nullptr;
            meshNode->children.clear();
        }
    }

    // Запись AABB
    if (hasMeshes) {
        loadedModel.localAABB.min = finalMin;
        loadedModel.localAABB.max = finalMax;
    } else {
        loadedModel.localAABB.min = glm::vec3(0.0f);
        loadedModel.localAABB.max = glm::vec3(0.0f);
    }

    // Создаем чистый финальный rootNode для нашей модели, у которого localTransform = identity
    loadedModel.rootNode = std::make_shared<Node>();

    // Привязываем все наши готовые, повернутые meshNodes напрямую к новому чистому корню
    for (auto& meshNode : loadedModel.meshNodes) {
        loadedModel.rootNode->AddChild(meshNode);
    }

    // Финальный локальный апдейт модели в чистом Z-Up пространстве движка
    loadedModel.rootNode->UpdateMatrices(glm::mat4(1.0f));


    loadedModel.bIsValid = true;
    std::cout << "Model successfully isolated & baked into native Z-Up!\n";
    return loadedModel;

}

uint32_t ModelManager::LoadModel(const std::filesystem::path& filePath, ModelLifetime lifetime, bool useArena) {
    std::string key = filePath.lexically_normal().string();

    // Защита от дублирования загруженных моделей
    auto it = _path_to_id.find(key);
    if (it != _path_to_id.end()) {
        return it->second;
    }

    uint32_t targetIndex = uint32_t(-1);
    // Ищем свободное место
    for (size_t i = 0; i < _models.size(); ++i) {
        if (!_models[i].bIsValid) { // Если нашли пустую дыру в массиве
            targetIndex = static_cast<uint32_t>(i);
            break;
        }
    }

    // Загружаем модель
    Model newModel = load_glTF(_init, _meshManager, _textureManager, filePath, lifetime,useArena);
    newModel.lifetime = lifetime;
    newModel.bIsValid = true;

    uint32_t externalId = 0;
    if (targetIndex == uint32_t(-1)) {
        // Размер вектора и есть ID новой модели (от 1)
        externalId = static_cast<uint32_t>(_models.size());

        // Если дыр в массиве не было - пушим в конец
        _models.push_back(std::move(newModel));
    } else {
        // Если нашли дыру — сажаем модель на старое место
        _models[targetIndex] = std::move(newModel);

        externalId = targetIndex;
    }
    // Храним ключ по которому сверяем загружена модель или нет
    _path_to_id[key] = externalId;

    return externalId;
}

Model& ModelManager::GetModel(uint32_t id){
    return _models.at(id);
}

std::vector<Model>& ModelManager::GetModels(){
    return _models;
}

bool ModelManager::empty(){
    if (_models.empty()) return true;
    return false;
}

bool ModelManager::has_model(uint32_t id){
    if (id >= _models.size()){
        return false;
    }
    else{
        return _models.at(id).bIsValid;
    }
}

void ModelManager:: destroy_model(uint32_t id){
    if (id >= _models.size() || !_models[id].bIsValid) {
        return;
    }

    if (_models[id].lifetime == ModelLifetime::Static) {
        std::cerr << "CRITICAL ERROR: Attempt to remove a static scene model during gameplay!\n";
        return;
    }

    vkDeviceWaitIdle(_init._device);

    // Вызываем очистку динамических ресурсов на GPU
    _models.at(id).destroy(_init, _meshManager, _textureManager);
    _models.at(id).bIsValid = false;

    // БЕЗОПАСНОЕ УДАЛЕНИЕ ИЗ МАПЫ:
    std::string keyToRemove = "";
    for (const auto& [path, modelId] : _path_to_id) {
        if (modelId == id) {
            keyToRemove = path;
            break;
        }
    }

    if (!keyToRemove.empty()) {
        _path_to_id.erase(keyToRemove);
    }
}

void ModelManager::destroy_dynamic_models() {
    for (size_t i = 0; i < _models.size(); ++i) {
        if (_models[i].bIsValid && _models[i].lifetime == ModelLifetime::Dynamic) {

            _models[i].destroy(_init, _meshManager, _textureManager);
            _models[i].bIsValid = false; // Ячейка свободна

            uint32_t externalId = static_cast<uint32_t>(i + 1);

            // Удаляем из мапы
            for (auto it = _path_to_id.begin(); it != _path_to_id.end(); ++it) {
                if (it->second == externalId) {
                    _path_to_id.erase(it);
                    break;
                }
            }
        }
    }
}


void ModelManager::destroy_all(){
    vkDeviceWaitIdle(_init._device);
    for (auto& model : _models) {
        // Чистим модель ТОЛЬКО если этот слот сейчас занят живым объектом
        if (model.bIsValid){
            model.destroy(_init, _meshManager, _textureManager);
            model.bIsValid = false; // На всякий случай сбрасываем флаг
        }
    }
    _models.clear();
    _path_to_id.clear();
}

AABB ModelManager::GetModelAABB(uint32_t id){
    if (!has_model(id)) {
        return AABB{ glm::vec3(-1.0f), glm::vec3(1.0f) };
    }
    return GetModel(id).localAABB;
}


void RenderSystem::Allocate(size_t count){
    _mainDrawQueue.reserve(count);
}

void RenderSystem::Submit(RenderObject ro){
    uint64_t pipelineBits = reinterpret_cast<uint64_t>(ro.pipeline) & 0xFFFFFFFF;
    uint64_t bufferBits = reinterpret_cast<uint64_t>(ro.indexBuffer) & 0xFFFFFFFF;
    ro.sortKey = (pipelineBits << 32) | bufferBits;

    _mainDrawQueue.push_back(ro);
}

void RenderSystem::PrepareFrame(){
    std::sort(_mainDrawQueue.begin(), _mainDrawQueue.end(), [](const RenderObject& a, const RenderObject& b) {
        return a.sortKey < b.sortKey;
    });
}

void RenderSystem::DrawForward(VkCommandBuffer cmd, VkExtent2D drawExtent, VkDescriptorSet globalDescriptor,
    VkDescriptorSet bindlessTextureSet){

    if (_mainDrawQueue.empty()) return;

    VkClearValue clearColor;
    clearColor.color = { { 0.3f, 0.3f, 0.3f, 1.0f } };

    VkClearValue depthClear;
    depthClear.depthStencil.depth = 0.0f;

    VkRenderingAttachmentInfo colorAttachment = vkinit::attachment_info(
        _init._msaaColorImage.imageView,
        &clearColor,
        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
    );
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

    colorAttachment.resolveMode = VK_RESOLVE_MODE_AVERAGE_BIT;
    colorAttachment.resolveImageView = _init._drawImage.imageView;
    colorAttachment.resolveImageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkRenderingAttachmentInfo depthAttachment = vkinit::depth_attachment_info(
        _init._msaaDepthImage.imageView,
        VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL
    );
    depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

    depthAttachment.clearValue = depthClear;

    VkRenderingInfo renderInfo = vkinit::rendering_info(drawExtent, &colorAttachment, &depthAttachment);

    vkCmdBeginRendering(cmd, &renderInfo);

    VkViewport viewport = { 0.0f, 0.0f, (float)drawExtent.width, (float)drawExtent.height, 1.0f, 0.0f };
    vkCmdSetViewport(cmd, 0, 1, &viewport);

    VkRect2D scissor = { {0, 0}, drawExtent };
    vkCmdSetScissor(cmd, 0, 1, &scissor);

    VkDescriptorSet setsToBind[] = { globalDescriptor, bindlessTextureSet };

    VkPipeline currentPipeline = VK_NULL_HANDLE;
    VkBuffer currentIndexBuffer = VK_NULL_HANDLE;

    for (const RenderObject& object : _mainDrawQueue) {
        if (object.pipeline != currentPipeline) {
            vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, object.pipeline);
            vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, object.pipelineLayout, 0, 2, setsToBind, 0, nullptr);
            currentPipeline = object.pipeline;
        }

        if (object.indexBuffer != VK_NULL_HANDLE) {
            if (object.indexBuffer != currentIndexBuffer) {
                vkCmdBindIndexBuffer(cmd, object.indexBuffer, 0, VK_INDEX_TYPE_UINT32);
                currentIndexBuffer = object.indexBuffer;
            }
        }

        GPUDrawPushConstants push_constants;
        push_constants.render_matrix = object.render_matrix;
        push_constants.vertexBuffer = object.vertexBufferAddress;

        push_constants.colorTextureID = object.colorTextureID;
        push_constants.metallicRoughnessTextureID = object.metallicRoughnessTextureID;
        push_constants.normalTextureID = object.normalTextureID;
        push_constants.occlusionTextureID = object.occlusionTextureID;

        push_constants.baseColorFactor = object.baseColorFactor;
        push_constants.materialFactors = object.materialFactors;

        vkCmdPushConstants(cmd, object.pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(GPUDrawPushConstants), &push_constants);

        if (object.indexBuffer != VK_NULL_HANDLE) {
            vkCmdDrawIndexed(cmd, object.indexCount, 1, object.firstIndex, 0, 0);
        } else {
            vkCmdDraw(cmd, object.indexCount, 1, 0, 0);
        }
    }

    vkCmdEndRendering(cmd);
}
