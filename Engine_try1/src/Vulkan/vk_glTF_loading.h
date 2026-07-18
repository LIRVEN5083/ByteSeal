#pragma once

#include "vk_types.h"
#include "vk_init_engine.h"
#include <filesystem>
#include <unordered_map>

struct GeoSurface {
    uint32_t startIndex;
    uint32_t count;
    glm::mat4 transform = glm::mat4(1.0f);
};

struct MeshAsset {
    std::string name;

    std::vector<GeoSurface> surfaces;
    GPUMeshBuffers meshBuffers;
};

//forward declaration
namespace VK_APPLICATION{
    class VulkanApplication;
}

std::optional<std::vector<std::shared_ptr<MeshAsset>>> loadGltfMeshes(VK_APPLICATION::VulkanApplication* engine, std::filesystem::path filePath);