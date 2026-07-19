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

struct GeoSurface {
    uint32_t startIndex;
    uint32_t count;
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
};

//forward declaration
namespace VK_APPLICATION{
    class VulkanApplication;
}

// Аллокация буферов под вершины
GPUMeshBuffers upload_meshes(VK_INIT_ENGINE::_inited_engine& _init, std::span<uint32_t> indices, std::span<Vertex> vertices);

// В разработке
//std::optional<AllocatedImage> load_image(VK_INIT_ENGINE::_inited_engine& _init, fastgltf::Asset& asset, fastgltf::Image& image);

std::optional<std::vector<std::shared_ptr<MeshAsset>>> load_Meshes(VK_INIT_ENGINE::_inited_engine& _init, fastgltf::Asset& asset);

std::optional<std::shared_ptr<Node>> load_Node(fastgltf::Asset& asset, fastgltf::Node& gltfNode, Model& outModel);

Model load_glTF(VK_INIT_ENGINE::_inited_engine& _init, VK_APPLICATION::VulkanApplication* engine, std::filesystem::path filePath);