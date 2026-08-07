#pragma once

class ModelManager;
class RenderSystem;
struct GPUSceneData;
struct AABB;

#include <stdint.h>
#include <unordered_map>
#include <glm/glm.hpp>
#include <string>
#include <vulkan/vulkan.h>

class PipelineManager;

struct GameEntity {
    uint32_t id{ 0 };
    std::string name;
    std::string type;

    // Трансформация объекта на сцене
    // Эта данные нам понадобятся в IMGUI
    glm::vec3 position{ 0.0f };
    glm::vec3 rotation{ 0.0f };
    glm::vec3 scale{ 1.0f };

    // Связь с ресурсом
    uint32_t modelAssetId{ 0 };
    bool bIsVisible{ true };

    glm::mat4 GetLocalMatrix() const;

    AABB GetWorldAABB(ModelManager& modelManager) const;
};

class Ray {
public:
    Ray() = default;
    Ray(const glm::vec3& origin, const glm::vec3& direction);

    // создает луч из экранных координат мыши
    static Ray FromScreen(float screenX, float screenY,
                          float screenWidth, float screenHeight,
                          const GPUSceneData& sceneData);

    // Алгоритм Смита (Smith's AABB/Ray intersection)
    bool IntersectsAABB(const AABB& box, float& outDist) const;

    const glm::vec3& GetOrigin() const { return _origin; }
    const glm::vec3& GetDirection() const { return _direction; }

    glm::vec3 GetPoint(float distance) const { return _origin + _direction * distance; }

private:
    glm::vec3 _origin{ 0.0f };
    glm::vec3 _direction{ 0.0f, 0.0f, 1.0f };
};

struct RaycastHit {
    bool hit{ false };
    float distance{ std::numeric_limits<float>::max() };
    GameEntity* entity{ nullptr };
};

class Scene{
public:
    Scene(ModelManager& modelManager) : _modelManager(modelManager){}

    GameEntity* CreateEntity(const std::string& name, uint32_t modelAssetId);

    GameEntity* GetEntity(uint32_t id);

    void DestroyEntity(uint32_t id);
    void DestroyAllEntites();
    void DestroyAllDynamicEntites();
    void DestroyEntitiesByModel(uint32_t modelAssetId);

    void CullingAndSubmit(RenderSystem& renderSystem, PipelineManager& pipelineManager, const glm::vec3& cameraPosition);

    RaycastHit Raycast(const Ray& ray);

private:
    ModelManager& _modelManager;

    std::vector<GameEntity> _entities;
    std::unordered_map<uint32_t, size_t> _idToIndex;

    uint32_t _nextEntityId{ 1 };
};
