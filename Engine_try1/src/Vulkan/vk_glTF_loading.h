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

// Делим ебанные обьекты по типу аллокации
// И времени их существования на сцене

// Static - arena-allocator
// Dynamic - динамическое выделение
enum class ModelLifetime : uint8_t{
    Static,
    Dynamic
};

// push constants для работы
struct GPUDrawPushConstants {
    glm::mat4 render_matrix;          // Обычная матрица преобразований
    VkDeviceAddress vertexBuffer;   // Вершинный буфер который мы алоцировали и получили адресс для передачи

    uint32_t colorTextureID;
    uint32_t metallicRoughnessTextureID;
};

struct GPUSceneData {
    glm::mat4 view;
    glm::mat4 proj;
    glm::mat4 viewproj;
    glm::vec4 ambientColor;
    glm::vec4 sunlightDirection; // w for sun power
    glm::vec4 sunlightColor;
};

struct Vertex {

    glm::vec3 position;
    float uv_x;
    glm::vec3 normal;
    float uv_y;
    glm::vec4 color;
};

struct GPUMeshBuffers {

    AllocatedBuffer indexBuffer;
    AllocatedBuffer vertexBuffer;
    VkDeviceAddress vertexBufferAddress;

    ModelLifetime lifetime{ ModelLifetime::Dynamic };
};

struct GPUTexture {
    AllocatedImage image;
    uint32_t globalIndex{ 0 };
    uint32_t mipLevels;

    VkDescriptorSet imguiDescriptorSet{VK_NULL_HANDLE};
    ModelLifetime lifetime{ ModelLifetime::Dynamic };
};

struct MaterialAsset {
    std::string name;
    uint32_t colorTextureID{ 0 };
    uint32_t metallicRoughnessTextureID{ 0 };

    glm::vec4 baseColorFactor{ 1.0f };
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

class TextureManager{
public:
    const uint32_t MAX_BINDLESS_TEXTURES = 1000;

    void init(VK_INIT_ENGINE::_inited_engine& _init);

    GPUTexture AllocateTexture(VkImageCreateInfo imageInfo, VkImageViewCreateInfo viewInfo, ModelLifetime lifetime = ModelLifetime::Dynamic);

    void FreeTexture(GPUTexture& texture);
    void DestroyAllocationData();

    void create_default_white_texture(VK_INIT_ENGINE::_inited_engine& _init);

    VkDescriptorSet GetTextureSet() const { return _textureSet; }
    VkDescriptorSetLayout GetTextureLayout() const { return _textureLayout; }
    VkSampler GetDefaultSampler() const {return _defaultSampler;}
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

    void destroy(VK_INIT_ENGINE::_inited_engine& _init, MeshManager& meshManager, TextureManager& textureManager);
};

//forward declaration
namespace VK_APPLICATION{
    class VulkanApplication;
}





// Парсинг текстур и их аллокация
std::optional<GPUTexture> load_image(VK_INIT_ENGINE::_inited_engine& _init, TextureManager& textureManager,
                                            const unsigned char* pixelData, uint32_t width, uint32_t height,
                                            VkFormat format, ModelLifetime lifetime = ModelLifetime::Dynamic);

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
private:
    VK_INIT_ENGINE::_inited_engine& _init;
    MeshManager& _meshManager;
    TextureManager& _textureManager;

    std::vector<Model> _models;
    std::unordered_map<std::string, uint32_t> _path_to_id;
};


struct RenderObject{
    VkBuffer indexBuffer;
    VkDeviceAddress vertexBufferAddress;
    uint32_t indexCount;
    uint32_t firstIndex;

    VkPipeline pipeline;
    VkPipelineLayout pipelineLayout;
    uint32_t colorTextureID;
    uint32_t metallicRoughnessTextureID;

    glm::mat4 render_matrix;

    // Ключ для сортировки
    uint64_t sortKey{0};

    AABB worldAABB;
};

class RenderSystem{
public:
    RenderSystem(VK_INIT_ENGINE::_inited_engine& init) : _init(init){}

    void Allocate(size_t count);

    // Создание ключа для RenderObject
    void Submit (RenderObject ro);

    // Сортировка по ключу
    void PrepareFrame();

    // TODO: Временная затычка с DESCRIPTOR SET, потом буду нормально передовать
    void DrawForward(VkCommandBuffer cmd, VkExtent2D drawExtent,
        VkDescriptorSet globalDescriptor, VkDescriptorSet bindlessTextureSet);

    // Очистка очереди
    void ClearQueue() { _mainDrawQueue.clear(); }
private:
    VK_INIT_ENGINE::_inited_engine& _init;
    std::vector<RenderObject> _mainDrawQueue;
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
