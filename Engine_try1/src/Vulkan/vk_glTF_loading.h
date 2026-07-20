#pragma once

#include "vk_types.h"
#include "vk_init_engine.h"
#include <filesystem>
#include <unordered_map>

#ifdef None
#undef None
#endif
#ifdef Success
#undef Success
#endif

#include <fastgltf/glm_element_traits.hpp>
#include <fastgltf/core.hpp>
#include <fastgltf/tools.hpp>
#include <iostream>

struct GPUTexture {
    AllocatedImage image;
    uint32_t globalIndex{ 0 };
};

struct MaterialAsset {
    std::string name;
    uint32_t colorTextureID{ 0 };
    uint32_t metallicRoughnessTextureID{ 0 };

    glm::vec4 baseColorFactor{ 1.0f };
};

class TextureManager{
public:
    const uint32_t MAX_BINDLESS_TEXTURES = 1000;

    void init(VK_INIT_ENGINE::_inited_engine& _init);

    GPUTexture AllocateTexture(VkImageCreateInfo imageInfo, VkImageViewCreateInfo viewInfo);

    void FreeTexture(const GPUTexture& texture);
    void DestroyAllocationData();

    void create_default_white_texture(VK_INIT_ENGINE::_inited_engine& _init);

    VkDescriptorSet GetTextureSet() const { return _textureSet; }
    VkDescriptorSetLayout GetTextureLayout() const { return _textureLayout; }

private:
    GPUTexture defaultTexture;

    VkDevice _device{ VK_NULL_HANDLE };
    VmaAllocator _allocator{ VK_NULL_HANDLE };

    VkDescriptorSetLayout _textureLayout;
    VkDescriptorPool _texturePool;
    VkDescriptorSet _textureSet;

    VmaPool _textureArena{ VK_NULL_HANDLE };
    VkSampler _defaultSampler{ VK_NULL_HANDLE };

    uint32_t _nextIndex{ 0 };
    std::vector<uint32_t> _freeIndices;
};

struct GeoSurface {
    uint32_t startIndex;
    uint32_t count;
    std::shared_ptr<MaterialAsset> material;
};

struct MeshAsset {
    std::string name;

    std::vector<GeoSurface> surfaces;
    GPUMeshBuffers meshBuffers;
};

class Node {
public:
    // Родитель ноды, тот кто выше её в иерархическом древе
    Node* parent{ nullptr };
    // Тот кто ниже этой ноды, потомок
    std::vector<std::shared_ptr<Node>> children;

    // Матрицы проебразования
    // Смещение относительно родителя
    glm::mat4 localTransform{ 1.0f };
    // Смещение в мировом пространстве
    glm::mat4 worldTransform{ 1.0f };

    virtual ~Node() = default;

    // Создание потомка, мы становимся родителем
    void AddChild(std::shared_ptr<Node> child);

    // Идём сверху вниз по иерархии и собираем модель целиком
    // Рекурсивно обнавляем всем матрицы смещения
    void UpdateMatrices(const glm::mat4& parentMatrix);
};

// Нода привязанная к саб-мешу
class MeshNode : public Node {
public:
    std::string meshID;
    std::shared_ptr<MeshAsset> mesh;
};

struct Model{
    std::vector<std::shared_ptr<MeshAsset>> Meshes;

    std::shared_ptr<Node> rootNode;
    std::vector<std::shared_ptr<MeshNode>> meshNodes;

    std::vector<GPUTexture> loadedTextures;
    std::vector<std::shared_ptr<MaterialAsset>> materials;

    void destroy(VK_INIT_ENGINE::_inited_engine& _init ,TextureManager& textureManager);
};

//forward declaration
namespace VK_APPLICATION{
    class VulkanApplication;
}

// Аллокация буферов под вершины
GPUMeshBuffers upload_meshes(VK_INIT_ENGINE::_inited_engine& _init, std::span<uint32_t> indices, std::span<Vertex> vertices);

std::optional<GPUTexture> load_image(VK_INIT_ENGINE::_inited_engine& _init, TextureManager& textureManager,
                                            const unsigned char* pixelData, uint32_t width, uint32_t height, VkFormat format);

std::optional<std::vector<std::shared_ptr<MeshAsset>>> load_Meshes(VK_INIT_ENGINE::_inited_engine& _init,
                    fastgltf::Asset& asset, const std::vector<std::shared_ptr<MaterialAsset>>& materials);

std::optional<std::shared_ptr<Node>> load_Node(fastgltf::Asset& asset, fastgltf::Node& gltfNode, Model& outModel);

Model load_glTF(VK_INIT_ENGINE::_inited_engine& _init, VK_APPLICATION::VulkanApplication* engine,
                TextureManager& textureManager, std::filesystem::path filePath);