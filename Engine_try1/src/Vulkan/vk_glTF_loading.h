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

// TODO: from vk_images.h
namespace vkutil{
    VkFilter GetVkFilter(int gltfFilter);
    VkSamplerMipmapMode GetVkMipmapMode(int gltfFilter);
    VkSamplerAddressMode GetVkAddressMode(int gltfWrap);
    struct SamplerCreateInfoHash;
    struct SamplerCreateInfoEqual;
}

// push constants для работы
// Сука выравнивание на GPU по 16 байт
struct GPUDrawPushConstants {
    glm::mat4 render_matrix;          // Обычная матрица преобразований
    VkDeviceAddress vertexBuffer;   // Вершинный буфер который мы алоцировали и получили адресс для передачи

    uint32_t colorTextureID;
    uint32_t metallicRoughnessTextureID;
    uint32_t normalTextureID;
    uint32_t occlusionTextureID;

    glm::vec2 padding{0.0f};

    glm::vec4 baseColorFactor;

    // roughness, metallic, emissive
    glm::vec4 materialFactors{0.5f, 0.0f, 0.0f, 0.0f};
};

struct GPUSceneData {
    glm::mat4 view;
    glm::mat4 proj;
    glm::mat4 viewproj;
    glm::vec4 ambientColor;
    glm::vec4 sunlightDirection; // w for sun power
    glm::vec4 sunlightColor;

    glm::mat4 cascadeMatrices[4];

    glm::vec4 cascadeSplits; // 4 каскада по 4 байта мы упоковываем в вектор из 4 компонентов

    uint32_t shadowMapTextureID;
    uint32_t padding[3]; // Выравнивание по 16 ByteSeal
};

struct Vertex {

    glm::vec3 position;
    float uv_x;
    glm::vec3 normal;
    float uv_y;
    glm::vec4 color;
    glm::vec4 tangent;
};

struct MaterialAsset {
    std::string name;
    std::string pipelineName;

    uint32_t colorTextureID{ 0 };
    uint32_t metallicRoughnessTextureID{ 0 };
    uint32_t normalTextureID{ 0 };
    uint32_t occlusionTextureID{ 0 };

    glm::vec4 baseColorFactor{ 1.0f };
    float roughnessFactor{ 0.5f };
    float metallicFactor{0.0f};
};

class MeshManager{
public:

    const uint64_t MAX_ARENA_SIZE = 512 * 1024 * 1024;

    // Создание арены
    void init(VK_INIT_ENGINE::_inited_engine& _init);

    // Удаление ВСЕХ мешей
    void DestroyAllocationData();

    // Удаление выбранного меша
    void FreeMesh(const GPUMeshBuffers& buffers);

    // Аллокация буферов под вершины
    GPUMeshBuffers upload_meshes(VK_INIT_ENGINE::_inited_engine& _init, std::span<uint32_t> indices, std::span<Vertex> vertices,
        ModelLifetime lifetime = ModelLifetime::Dynamic);
private:
    VkDevice _device{ VK_NULL_HANDLE };
    VmaAllocator _allocator{ VK_NULL_HANDLE };

    VmaPool _meshArena;
};

struct SamplerOptions{
    int minFilter{ 9729 }; // LINEAR по умолчанию
    int magFilter{ 9729 }; // LINEAR по умолчанию
    int wrapS{ 10497 };    // REPEAT по умолчанию
    int wrapT{ 10497 };    // REPEAT по умолчанию
};

class TextureManager{
public:
    const uint32_t MAX_BINDLESS_TEXTURES = 1000;

    void init(VK_INIT_ENGINE::_inited_engine& _init);

    GPUTexture AllocateTexture(
        VkImageCreateInfo imageInfo,
        VkImageViewCreateInfo viewInfo,
        const SamplerOptions& params = {},
        ModelLifetime lifetime = ModelLifetime::Dynamic);

    void FreeTexture(GPUTexture& texture);
    void DestroyAllocationData();

    void create_default_white_texture(VK_INIT_ENGINE::_inited_engine& _init);

    VkDescriptorSet GetTextureSet() const { return _textureSet; }
    VkDescriptorSetLayout GetTextureLayout() const { return _textureLayout; }
    VkSampler GetDefaultSampler() const {return _defaultSampler;}
private:
    VkSampler CreateSampler(const SamplerOptions& params);

    GPUTexture defaultTexture;

    VkDevice _device{ VK_NULL_HANDLE };
    VmaAllocator _allocator{ VK_NULL_HANDLE };

    VkDescriptorSetLayout _textureLayout;
    VkDescriptorPool _texturePool;
    VkDescriptorSet _textureSet;

    VmaPool _textureArena{ VK_NULL_HANDLE };

    // Базовый сэмлпер пустышка
    VkSampler _defaultSampler{ VK_NULL_HANDLE };

    // Хэш ддя сэмплеров
    std::unordered_map<VkSamplerCreateInfo, VkSampler, vkutil::SamplerCreateInfoHash, vkutil::SamplerCreateInfoEqual> _samplerCache;

    uint32_t _nextIndex{ 0 };
    std::vector<uint32_t> _freeIndices;
};

struct AABB {
    glm::vec3 min{ std::numeric_limits<float>::infinity() };
    glm::vec3 max{ -std::numeric_limits<float>::infinity() };
};

struct GeoSurface {
    uint32_t startIndex;
    uint32_t count;
    std::shared_ptr<MaterialAsset> material;
    AABB localAABB;
};

struct MeshAsset {
    std::string name;

    std::vector<GeoSurface> surfaces;
    GPUMeshBuffers meshBuffers;

    AABB localAABB;
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

    ModelLifetime lifetime{ ModelLifetime::Dynamic };
    bool bIsValid{ false };

    AABB localAABB;

    void destroy(VK_INIT_ENGINE::_inited_engine& _init, MeshManager& meshManager, TextureManager& textureManager);
};

//forward declaration
namespace VK_APPLICATION{
    class VulkanApplication;
}





// Парсинг текстур и их аллокация
std::optional<GPUTexture> load_image(VK_INIT_ENGINE::_inited_engine& _init, TextureManager& textureManager,
                                            const unsigned char* pixelData, uint32_t width, uint32_t height,
                                            VkFormat format, SamplerOptions samplerParams = {},
                                            ModelLifetime lifetime = ModelLifetime::Dynamic);

// Парсинг мешей и их аллокация
std::optional<std::vector<std::shared_ptr<MeshAsset>>> load_Meshes(VK_INIT_ENGINE::_inited_engine& _init,
                    MeshManager& meshManager, fastgltf::Asset& asset,
                    const std::vector<std::shared_ptr<MaterialAsset>>& materials,
                    ModelLifetime lifetime = ModelLifetime::Dynamic);

// Парсинг Node
std::optional<std::shared_ptr<Node>> load_Node(fastgltf::Asset& asset, fastgltf::Node& gltfNode, Model& outModel);

AABB transformAABB(const AABB& localBox, const glm::mat4& M);

void calculate_model_bounds(Node* node, const glm::mat4& parentTransform, AABB& outTotalAABB);

// Обьединение всего парсинга модели
Model load_glTF(VK_INIT_ENGINE::_inited_engine& _init,
                MeshManager& meshManager, TextureManager& textureManager, std::filesystem::path filePath,
                ModelLifetime lifetime = ModelLifetime::Dynamic, bool useArena = false);





class ModelManager {
public:
    ModelManager(VK_INIT_ENGINE::_inited_engine& init, MeshManager& meshManager, TextureManager& texManager)
        : _init(init), _meshManager(meshManager), _textureManager(texManager) {}

    uint32_t LoadModel(const std::filesystem::path& filePath, ModelLifetime lifetime, bool useArena);

    // Геттер на модель
    Model& GetModel(uint32_t id);

    std::vector<Model>& GetModels();

    bool empty();

    uint32_t CountOfModels() { return _models.size(); }

    bool has_model(uint32_t id);

    // Метод очистки моделей
    void destroy_model(uint32_t id);
    void destroy_dynamic_models();
    void destroy_all();

    VkSampler GetDefaultSampler() const{ return _textureManager.GetDefaultSampler(); }

    AABB GetModelAABB(uint32_t id);

private:
    VK_INIT_ENGINE::_inited_engine& _init;
    MeshManager& _meshManager;
    TextureManager& _textureManager;

    std::vector<Model> _models;
    std::unordered_map<std::string, uint32_t> _path_to_id;
};

struct StaticModelConf{
    bool useArena{true};
    ModelLifetime lifetime{ ModelLifetime::Static };
};

struct DynamicModelConf{
    bool useArena{false};
    ModelLifetime lifetime{ ModelLifetime::Dynamic };
};

inline StaticModelConf _confStatic;
inline DynamicModelConf _confDynamic;
