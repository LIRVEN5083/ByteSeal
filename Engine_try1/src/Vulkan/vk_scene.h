#pragma once
#include "vk_glTF_loading.h"

class ModelManager;
class RenderSystem;
struct GPUSceneData;
struct AABB;

#include "vk_types.h"
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
    glm::quat rotation{ 1.0f, 0.0f, 0.0f, 0.0f };
    glm::vec3 scale{ 1.0f };

    glm::vec3 uiEulerRotation{ 0.0f };

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

class TextureManager;
struct SamplerOptions;

// Количество каскадов
static constexpr uint32_t SHADOW_CASCADES_COUNT = 4;

struct CSMConfig {
    uint32_t resolution = 4096; // Разрешение карты теней

    // Коэффициенты разбиения фрустума (от 0.0 до 1.0).
    // Первые каскады должны быть маленькими для высокой четкости вблизи игрока.
    float cascadeSplits[SHADOW_CASCADES_COUNT] = { 0.07f, 0.2f, 0.45f, 1.0f };

    // Practical Split
    float splitLambda = 0.85f;
};

// Реализация SCM (Cascad map) и множественного освещения
class LightManager{
public:

    LightManager(VkDevice device, TextureManager& textureManager, CSMConfig config = {})
       : m_device(device), m_textureManager(textureManager), m_config(config) {}

    void init();

    void UpdateCascades(const glm::mat4& viewMatrix, float fovY, float aspect, float cameraNear, float cameraFar, const glm::vec3& lightDir);

    // Геттеры
    uint32_t GetShadowTextureIndex() const;
    const glm::mat4* GetCascadeMatrices() const;
    const float* GetCascadeSplits() const;
    uint32_t GetResolution() const;
    VkImageView GetShadowTextureView() const;
    VkImage GetShadowImage() const;

    void cleanUp();

private:
    VkDevice m_device;
    TextureManager& m_textureManager;
    CSMConfig m_config;

    GPUTexture m_shadowArrayTexture;

    // Результаты расчетов для передачи в шейдеры
    std::array<glm::mat4, SHADOW_CASCADES_COUNT> m_cascadeMatrices;
    std::array<float, SHADOW_CASCADES_COUNT> m_cascadeSplits;
};
