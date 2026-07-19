#include "vk_glTF_loading.h"
#include "vk_application.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

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

std::optional<std::vector<std::shared_ptr<MeshAsset>>> load_Meshes(VK_INIT_ENGINE::_inited_engine& _init, fastgltf::Asset& asset){
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
        constexpr bool OverrideColors = true;
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
    currentEngineNode->localTransform = std::bit_cast<glm::mat4>(fastgltf::getTransformMatrix(gltfNode));

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
    std::filesystem::path filePath){
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

    // Загрузка мешей
    auto meshesOpt = load_Meshes(_init, gltf);
    if (meshesOpt.has_value()) {
        loadedModel.Meshes = std::move(meshesOpt.value());
    } else {
        fmt::print("Failed to process meshes for file: {}\n", filePath.string());
        return loadedModel;
    }

    auto baseRoot = std::make_shared<Node>();

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
