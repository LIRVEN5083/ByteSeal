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

class TransformBufferManager {
private:
    VkDevice m_Device;
    VmaAllocator m_Allocator;

    VkBuffer m_Buffer = VK_NULL_HANDLE;
    VmaAllocation m_Allocation = VK_NULL_HANDLE;
    VkDeviceAddress m_BaseDeviceAddress = 0;
    void* m_MappedData = nullptr;

    uint32_t m_MaxObjects = 1000; // Резервируем место, например, на 10к объектов
    uint32_t m_CurrentAllocatedObjects = 0;

public:
    void init(VkDevice device, VmaAllocator allocator, uint32_t maxObjects = 10000);

    // Вызывается нодой сцены при её создании (например, при загрузке меша)
    uint32_t AllocateTransformSlot(VkDeviceAddress& outAddress, uint32_t count);

    // Обновление матриц конкретного объекта (вызывается из графа сцены)
    void UpdateTransform(uint32_t slot, const glm::mat4& currentModel, const glm::mat4& prevModel);

    void cleanup();
};

class PipelineManager;

struct FrustumPlane {
    glm::vec3 normal;
    float distance;

    // Метод проверки: находится ли AABB хотя бы частично внутри (впереди плоскости)
    bool IsAABBInFront(const glm::vec3& min, const glm::vec3& max) const;
};

struct CameraFrustum {
    std::array<FrustumPlane, 6> planes;

    // Виден ли AABB
    bool IsBoxVisible(const glm::vec3& min, const glm::vec3& max) const;
};

// Математика Грибба-Хартмана ну мы типо делаем CPU Culling
CameraFrustum CreateFrustumFromMatrix(const glm::mat4& mat);

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

    bool bOverrideMaterial{ false };
    float metallic{ 0.0f };
    float roughness{ 0.0f };
    float opacity{ 1.0f };

    // Связь с ресурсом
    uint32_t modelAssetId{ 0 };
    bool bIsVisible{ true };

    glm::mat4 prevModelMatrix{ 1.0f };

    uint32_t transformSlot{ 0 };
    VkDeviceAddress matrixGPUAddress{ 0 };
    bool hasTransformSlot{ false };

    glm::mat4 GetLocalMatrix() const;

    glm::mat4 GetPrevLocalMatrix() const { return prevModelMatrix; }

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

struct Model;

class Scene{
public:
    Scene(ModelManager& modelManager, TransformBufferManager& transformManager) :
    _modelManager(modelManager), _transformManager(transformManager){}

    GameEntity* CreateEntity(const std::string& name, uint32_t modelAssetId);

    GameEntity* GetEntity(uint32_t id);

    void DestroyEntity(uint32_t id);
    void DestroyAllEntites();
    void DestroyAllDynamicEntites();
    void DestroyEntitiesByModel(uint32_t modelAssetId);

    void CullingAndSubmit(RenderSystem& renderSystem, PipelineManager& pipelineManager,
    TransformBufferManager& transformManager,
    const glm::vec3& cameraPosition,
    const glm::mat4& currentViewProjJittered,  // Матрица С дрожанием (для куллинга)
    const glm::mat4& currentViewProjNonJittered, // Текущая БЕЗ дрожания (для TAA)
    const glm::mat4& prevViewProjNonJittered);  // Прошлая БЕЗ дрожания (для TAA)

    RaycastHit Raycast(const Ray& ray);

private:
    TransformBufferManager& _transformManager;
    ModelManager& _modelManager;

    std::vector<GameEntity> _entities;
    std::unordered_map<uint32_t, size_t> _idToIndex;

    uint32_t _nextEntityId{ 1 };
};

class TextureManager;
struct SamplerOptions;

// Количество каскадов
static constexpr uint32_t SHADOW_CASCADES_COUNT = 4;

struct SkyCoefficients {
    glm::vec4 skyA, skyB, skyC, skyD, skyE, skyF, skyG, skyH, skyI, skyZ;
};

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

    SkyCoefficients ComputeSkyModel(const glm::vec3& lightDir, float turbidity = 2.0f, const glm::vec3& groundAlbedo = glm::vec3(0.3f));

    // Геттеры
    uint32_t GetShadowTextureIndex() const;
    const glm::mat4* GetCascadeMatrices() const;
    const float* GetCascadeSplits() const;
    uint32_t GetResolution() const;
    VkImageView GetShadowTextureView() const;
    VkImage GetShadowImage() const;

    void cleanup();

private:
    SkyCoefficients ComputeHosekWilkieParams(float turbidity, const glm::vec3& albedo, float sunElevation);

    VkDevice m_device;
    TextureManager& m_textureManager;
    CSMConfig m_config;

    GPUTexture m_shadowArrayTexture;

    // Результаты расчетов для передачи в шейдеры
    std::array<glm::mat4, SHADOW_CASCADES_COUNT> m_cascadeMatrices;
    std::array<float, SHADOW_CASCADES_COUNT> m_cascadeSplits;
};
