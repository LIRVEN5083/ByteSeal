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

    VkSamplerCreateInfo samplerInfo{ VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO };
    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.maxAnisotropy = 1.0f;
    vkCreateSampler(_init._device, &samplerInfo, nullptr, &_defaultSampler);

    create_default_white_texture(_init);
}

GPUTexture TextureManager::AllocateTexture(VkImageCreateInfo imageInfo, VkImageViewCreateInfo viewInfo){
    GPUTexture texture{};

    // Получить bindless индекс
    if (!_freeIndices.empty()) {
        texture.globalIndex = _freeIndices.back();
        _freeIndices.pop_back();
    } else {
        texture.globalIndex = _nextIndex++;
        if (texture.globalIndex >= MAX_BINDLESS_TEXTURES) {
        }
    }

    VmaAllocationCreateInfo poolAllocInfo{};
    poolAllocInfo.pool = _textureArena;
    poolAllocInfo.usage = VMA_MEMORY_USAGE_AUTO;
    vmaCreateImage(_allocator, &imageInfo, &poolAllocInfo, &texture.image.image, &texture.image.allocation, nullptr);

    viewInfo.image = texture.image.image;
    vkCreateImageView(_device, &viewInfo, nullptr, &texture.image.imageView);

    VkDescriptorImageInfo descriptorImgInfo{};
    descriptorImgInfo.imageView = texture.image.imageView;
    descriptorImgInfo.sampler = _defaultSampler;
    descriptorImgInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    VkWriteDescriptorSet write{ VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
    write.dstSet = _textureSet;
    write.dstBinding = 0;
    write.dstArrayElement = texture.globalIndex;
    write.descriptorCount = 1;
    write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    write.pImageInfo = &descriptorImgInfo;

    vkUpdateDescriptorSets(_device, 1, &write, 0, nullptr);

    return texture;
}

void TextureManager::FreeTexture(const GPUTexture& texture){
    if (texture.image.imageView != VK_NULL_HANDLE) {
        vkDestroyImageView(_device, texture.image.imageView, nullptr);
    }

    if (texture.image.image != VK_NULL_HANDLE) {
        vmaDestroyImage(_allocator, texture.image.image, texture.image.allocation);
    }

    // Возвращаем индекс в пул свободных для переиспользования
    _freeIndices.push_back(texture.globalIndex);
}

void TextureManager::DestroyAllocationData(){
    if (_defaultSampler) vkDestroySampler(_device, _defaultSampler, nullptr);
    if (defaultTexture.image.imageView != VK_NULL_HANDLE) {
        FreeTexture(defaultTexture);
    }

    // Теперь, когда ВСЕ картинки моделей и сама заглушка честно удалены через vmaDestroyImage,
    // этот пул гарантированно пустой, и ассерт m_pMetadata->IsEmpty() закроется молча!
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

    // 2. Создаем временный Staging-буфер
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

    // 3. Инфо для текстуры 1х1 на GPU
    VkImageCreateInfo imgInfo{ VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO };
    imgInfo.imageType = VK_IMAGE_TYPE_2D;
    imgInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
    imgInfo.extent = { 1, 1, 1 }; // Размер 1х1 пиксель
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

    // AllocateTexture гарантированно выдаст этой текстуре globalIndex = 0,
    // так как это самая первая аллокация при старте менеджера!
    defaultTexture = AllocateTexture(imgInfo, viewInfo);

    // 4. Заливаем пиксель на GPU через твой submit_immediate
    vkinit::submit_immediate([&](VkCommandBuffer cmd) {

        VkImageMemoryBarrier barrier{ VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
        barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.image = defaultTexture.image.image; // Используем твою структуру с AllocatedImage
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange.levelCount = 1;
        barrier.subresourceRange.layerCount = 1;
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

        vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);

        VkBufferImageCopy copyRegion{};
        copyRegion.bufferOffset = 0;
        copyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        copyRegion.imageSubresource.layerCount = 1;
        copyRegion.imageExtent = imgInfo.extent;

        vkCmdCopyBufferToImage(cmd, stagingBuffer.buffer, defaultTexture.image.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copyRegion);

        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);

    }, _init);

    // Очищаем временный буфер
    vmaDestroyBuffer(_init._allocator, stagingBuffer.buffer, stagingBuffer.allocation);

    std::cout << "TextureManager: Default white texture generated under Bindless ID = 0\n";
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

std::optional<AllocatedImage> load_image(VK_INIT_ENGINE::_inited_engine& _init, fastgltf::Asset& asset,
                                         fastgltf::Image& image){
    AllocatedImage newImage {};

    int width, height, nrChannels;

    std::visit(
        fastgltf::visitor {
            [](auto& arg) {},
            [&](fastgltf::sources::URI& filePath) {
                assert(filePath.fileByteOffset == 0); // We don't support offsets with stbi.
                assert(filePath.uri.isLocalPath()); // We're only capable of loading
                                                    // local files.

                const std::string path(filePath.uri.path().begin(),
                    filePath.uri.path().end()); // Thanks C++.
                unsigned char* data = stbi_load(path.c_str(), &width, &height, &nrChannels, 4);
                if (data) {
                    VkExtent3D imagesize;
                    imagesize.width = width;
                    imagesize.height = height;
                    imagesize.depth = 1;

                    newImage = vkinit::create_image(data, imagesize, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_SAMPLED_BIT, _init,false);

                    stbi_image_free(data);
                }
            },
            [&](fastgltf::sources::Vector& vector) {
                unsigned char* data = stbi_load_from_memory(
                    reinterpret_cast<const stbi_uc*>(vector.bytes.data()),
                    static_cast<int>(vector.bytes.size()),
                    &width, &height, &nrChannels, 4);
                if (data) {
                    VkExtent3D imagesize;
                    imagesize.width = width;
                    imagesize.height = height;
                    imagesize.depth = 1;

                    newImage = vkinit::create_image(data, imagesize, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_SAMPLED_BIT, _init,false);

                    stbi_image_free(data);
                }
            },
            [&](fastgltf::sources::BufferView& view) {
                auto& bufferView = asset.bufferViews[view.bufferViewIndex];
                auto& buffer = asset.buffers[bufferView.bufferIndex];

                std::visit(fastgltf::visitor { // We only care about VectorWithMime here, because we
                                               // specify LoadExternalBuffers, meaning all buffers
                                               // are already loaded into a vector.
                               [](auto& arg) {},
                               [&](fastgltf::sources::Vector& vector) {
                                   unsigned char* data = stbi_load_from_memory(
                                       reinterpret_cast<const stbi_uc*>(vector.bytes.data() + bufferView.byteOffset),
                                       static_cast<int>(bufferView.byteLength),
                                       &width, &height, &nrChannels, 4);
                                   if (data) {
                                       VkExtent3D imagesize;
                                       imagesize.width = width;
                                       imagesize.height = height;
                                       imagesize.depth = 1;

                                       newImage = vkinit::create_image(data, imagesize, VK_FORMAT_R8G8B8A8_UNORM,
                                           VK_IMAGE_USAGE_SAMPLED_BIT, _init,false);

                                       stbi_image_free(data);
                                   }
                               } },
                    buffer.data);
            },
        },
        image.data);

    // if any of the attempts to load the data failed, we havent written the image
    // so handle is null
    if (newImage.image == VK_NULL_HANDLE) {
        return {};
    } else {
        return newImage;
    }
}

void Model::destroy(VK_INIT_ENGINE::_inited_engine& _init, TextureManager& textureManager){
    for (auto& tex : loadedTextures) {
        // Пропускаем заглушки с ID = 0, их удалять нельзя, ими владеет менеджер!
        if (tex.globalIndex != 0) {
            textureManager.FreeTexture(tex);
        }
    }
    loadedTextures.clear();
    materials.clear();

    // 2. Освобождаем буферы геометрии мешей
    for (auto& mesh : Meshes) {
        vmaDestroyBuffer(_init._allocator, mesh->meshBuffers.indexBuffer.buffer, mesh->meshBuffers.indexBuffer.allocation);
        vmaDestroyBuffer(_init._allocator, mesh->meshBuffers.vertexBuffer.buffer, mesh->meshBuffers.vertexBuffer.allocation);
    }
    Meshes.clear();
    meshNodes.clear();
    rootNode.reset();
}

GPUMeshBuffers upload_meshes(VK_INIT_ENGINE::_inited_engine& _init, std::span<uint32_t> indices,
                             std::span<Vertex> vertices){
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

std::optional<GPUTexture> load_image(VK_INIT_ENGINE::_inited_engine& _init, TextureManager& textureManager,
    const unsigned char* pixelData, uint32_t width, uint32_t height, VkFormat format){
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

    // 2. Настраиваем инфо для оптимальной текстуры на GPU
    VkImageCreateInfo imgInfo{ VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO };
    imgInfo.imageType = VK_IMAGE_TYPE_2D;
    imgInfo.format = format;
    imgInfo.extent = { width, height, 1 };
    imgInfo.mipLevels = 1;
    imgInfo.arrayLayers = 1;
    imgInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imgInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imgInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;

    VkImageViewCreateInfo viewInfo{ VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = format;
    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.layerCount = 1;

    // Выделяем память из VMA Арены и регистрируем в Bindless-сет
    GPUTexture outTexture = textureManager.AllocateTexture(imgInfo, viewInfo);

    // Отправляем команды копирования на GPU через submit_immediate
    vkinit::submit_immediate([&](VkCommandBuffer cmd) {
        VkImageMemoryBarrier barrier{ VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
        barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.image = outTexture.image.image;
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange.levelCount = 1;
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
        copyRegion.imageSubresource.layerCount = 1;
        copyRegion.imageExtent = imgInfo.extent;

        vkCmdCopyBufferToImage(cmd, stagingBuffer.buffer, outTexture.image.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copyRegion);

        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        vkCmdPipelineBarrier(cmd,
            VK_PIPELINE_STAGE_TRANSFER_BIT,
            VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
            0, 0, nullptr, 0, nullptr, 1, &barrier);

    }, _init);

    // Очищаем временный staging-буфер
    vmaDestroyBuffer(_init._allocator, stagingBuffer.buffer, stagingBuffer.allocation);

    return outTexture;
}

std::optional<std::vector<std::shared_ptr<MeshAsset>>> load_Meshes(VK_INIT_ENGINE::_inited_engine& _init,
                            fastgltf::Asset& asset, const std::vector<std::shared_ptr<MaterialAsset>>& materials){
    std::vector<std::shared_ptr<MeshAsset>> meshes;
    std::vector<uint32_t> indices;
    std::vector<Vertex> vertices;

    for (size_t meshIdx = 0; meshIdx < asset.meshes.size(); ++meshIdx) {
        fastgltf::Mesh& gltfMesh = asset.meshes[meshIdx];

        auto newMeshAsset = std::make_shared<MeshAsset>();
        newMeshAsset->name = gltfMesh.name.c_str();

        indices.clear();
        vertices.clear();

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

            // 2. Загрузка позиций вершин
            {
                auto* positionAttribute = p.findAttribute("POSITION");
                if (positionAttribute == nullptr) {
                    fmt::print("Failed to find position attribute\n");
                } else {
                    fastgltf::Accessor& posAccessor = asset.accessors[positionAttribute->accessorIndex];
                    vertices.resize(vertices.size() + posAccessor.count);

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

            newMeshAsset->surfaces.push_back(newSurface);
        }

        // Визуализация нормалей через цвета (Блендер-стайл дебаг)
        constexpr bool OverrideColors = false;
        if (OverrideColors) {
            for (Vertex& vtx : vertices) {
                vtx.color = glm::vec4(vtx.normal, 1.f);
            }
        }

        // Загружаем буферы в Vulkan для конкретно этого меша
        newMeshAsset->meshBuffers = upload_meshes(_init, indices, vertices);
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
    currentEngineNode->localTransform = glm::transpose(glm::make_mat4(gltfMatrixData.data()));

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

Model load_glTF(VK_INIT_ENGINE::_inited_engine& _init, VK_APPLICATION::VulkanApplication* engine,
                TextureManager& textureManager, std::filesystem::path filePath){
    // Возвращаемая модель
    Model loadedModel;
    std::cout << "Loading GLTF: " << filePath << std::endl;
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
    for (auto& gltfImage : gltf.images) {
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
            auto gpuTex = load_image(_init, textureManager, pixelData, width, height, VK_FORMAT_R8G8B8A8_SRGB);

            stbi_image_free(pixelData);

            if (gpuTex.has_value()){
                loadedModel.loadedTextures.push_back(gpuTex.value());
                continue;
            }
        }
        else {
            std::cout << "CRITICAL: fastgltf/stbi failed to load image data for one of the textures!\n";
        }
        GPUTexture dummyTex{};
        dummyTex.globalIndex = 0;
        loadedModel.loadedTextures.push_back(dummyTex);
    }

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Загрузка материалов
    for (auto& gltfMaterial : gltf.materials) {
        auto newMaterial = std::make_shared<MaterialAsset>();
        newMaterial->name = gltfMaterial.name.c_str();
        newMaterial->baseColorFactor = glm::make_vec4(gltfMaterial.pbrData.baseColorFactor.data());

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

        loadedModel.materials.push_back(newMaterial);
    }
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Загрузка мешей
    auto meshesOpt = load_Meshes(_init, gltf, loadedModel.materials);
    if (meshesOpt.has_value()) {
        loadedModel.Meshes = std::move(meshesOpt.value());
    } else {
        fmt::print("Failed to process meshes for file: {}\n", filePath.string());
        return loadedModel;
    }

    auto baseRoot = std::make_shared<Node>();

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Загрузка Nodes
    fastgltf::Scene& defaultScene = gltf.scenes[gltf.defaultScene.value_or(0)];

    for (size_t rootNodeIdx : defaultScene.nodeIndices) {
        // Запускаем рекурсивный сбор дерева нод.
        // Функция load_Node сама свяжет меши и наполнит плоский список loadedModel.meshNodes
        auto nodeOpt = load_Node(gltf, gltf.nodes[rootNodeIdx], loadedModel);
        if (nodeOpt.has_value()) {
            baseRoot->AddChild(nodeOpt.value());
        }
    }

    // Сохраняем корень дерева в модель
    loadedModel.rootNode = baseRoot;

    return loadedModel;

}