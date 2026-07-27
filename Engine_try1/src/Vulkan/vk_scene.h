#pragma once
#include "vk_glTF_loading.h"

struct GameEntity {
    uint32_t id{ 0 };
    std::string name;

    // Трансформация объекта на сцене
    // Эта данные нам понадобятся в IMGUI
    glm::vec3 position{ 0.0f };
    glm::vec3 rotation{ 0.0f };
    glm::vec3 scale{ 1.0f };

    // Связь с ресурсом
    uint32_t modelAssetId{ 0 };
    bool bIsVisible{ true };

    glm::mat4 GetLocalMatrix() const;
};

class Scene{
public:
    Scene(ModelManager& modelManager) : _modelManager(modelManager){}

    GameEntity* CreateEntity(const std::string& name, uint32_t modelAssetId);

    GameEntity* GetEntity(uint32_t id);

    void DestroyEntity(uint32_t id);

    void CullingAndSubmit(RenderSystem& renderSystem, VkPipeline defaultPipeline, VkPipelineLayout defaultLayout);

private:
    ModelManager& _modelManager;

    std::vector<GameEntity> _entities;
    std::unordered_map<uint32_t, size_t> _idToIndex;

    uint32_t _nextEntityId{ 1 };
};

struct EntityConfig {
    std::string modelPath;
    std::string entityName;
    glm::vec3 initialPosition;
};