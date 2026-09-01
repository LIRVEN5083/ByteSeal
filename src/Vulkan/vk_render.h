#pragma once
#include "vk_glTF_loading.h"
struct RealPipeline;
#include "vk_types.h"

class ComputeRenderSystem;
class TextureManager;

struct RenderObject{
    VkBuffer indexBuffer;
    VkDeviceAddress vertexBufferAddress;
    uint32_t indexCount;
    uint32_t firstIndex;

    VkPipeline pipeline;
    VkPipelineLayout pipelineLayout;

    uint32_t colorTextureID;
    uint32_t metallicRoughnessTextureID;
    uint32_t normalTextureID;
    uint32_t occlusionTextureID;

    glm::vec4 baseColorFactor;
    glm::vec4 materialFactors;

    glm::mat4 render_matrix;

    PipelineOpacity opacity;

    // Ключ для сортировки
    uint64_t sortKey{0};
};

struct RenderContext {
    VkCommandBuffer cmd;
    VkExtent2D drawExtent;
    VkDescriptorSet globalDescriptor;
    VkDescriptorSet bindlessTextureSet;
    VkPipelineLayout pipelineLayout;

    class PipelineManager* pipelineManager;
    class LightManager* lightManager;
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ПРОХОДЫ РЕНДЕРА

class RenderPass {
protected:
    VK_INIT_ENGINE::_inited_engine& _init;
    RenderPassType _type;
    bool _isActive{true};

public:
    RenderPass(VK_INIT_ENGINE::_inited_engine& init, RenderPassType type)
        : _init(init), _type(type) {}

    virtual ~RenderPass() = default;

    RenderPassType GetType() const { return _type; }

    // Переключение состояния RenderPass
    void SetEnabled(bool enabled) { _isActive = enabled; }
    bool IsEnabled() const { return _isActive; }
    void Toggle() { _isActive = !_isActive; }


    // Вызывается при старте и после ReloadShaders
    virtual void Init(PipelineManager& pipelineManager) = 0;

    // Запись команд рендеринга для текущего пасса
    virtual void Execute(const RenderContext& ctx, const std::vector<RenderObject>& queue) = 0;
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ОСНОВНОЙ ПРОХОД РЕНДЕРА
class ForwardRenderPass : public RenderPass{
public:
    ForwardRenderPass(VK_INIT_ENGINE::_inited_engine& init)
        : RenderPass(init, RenderPassType::Forward){}
    ~ForwardRenderPass() override = default;

    void Init(PipelineManager& pipelineManager) override;

    void Execute(const RenderContext& ctx, const std::vector<RenderObject>& queue) override;

private:
    void DrawFilteredObjects(const RenderContext& ctx, const std::vector<RenderObject>& queue, PipelineOpacity opacityFilter);

    RealPipeline* _opaquePipeline{ nullptr };
    RealPipeline* _alphaTestedPipeline{ nullptr };
    RealPipeline* _transparentPipeline{ nullptr };
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ПРОХОД ДЛЯ СЕТКИ
class GridRenderPass : public RenderPass {
public:
    GridRenderPass(VK_INIT_ENGINE::_inited_engine& init)
        : RenderPass(init, RenderPassType::Grid) {}
    ~GridRenderPass() override = default;

    void Init(PipelineManager& pipelineManager) override;
    void Execute(const RenderContext& ctx, const std::vector<RenderObject>& queue) override;

private:
    RealPipeline* _gridPipeline{ nullptr };
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ПРОХОД ДЛЯ КАСКАДНЫХ ТЕНЕЙ
class ShadowCSMRenderPass : public RenderPass {
public:
    ShadowCSMRenderPass(VK_INIT_ENGINE::_inited_engine& init)
        : RenderPass(init, RenderPassType::ShadowCSM) {}
    ~ShadowCSMRenderPass() override = default;

    void Init(PipelineManager& pipelineManager) override;
    void Execute(const RenderContext& ctx, const std::vector<RenderObject>& queue) override;

private:
    RealPipeline* _shadowPipeline{ nullptr };
};
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ПРОХОД для SkyBox

enum class SkyBoxType : uint32_t{
    Panoramic = 0,
    Cubemap = 1,
    Procedural = 2
};

class SkyBoxRenderPass : public RenderPass{
public:
    SkyBoxRenderPass(VK_INIT_ENGINE::_inited_engine& init)
        : RenderPass(init, RenderPassType::Skybox) {}
    ~SkyBoxRenderPass() override = default;

    void Init(PipelineManager& pipelineManager) override;

    void Execute(const RenderContext& ctx, const std::vector<RenderObject>& queue) override;

    void SetSkyboxType(SkyBoxType type) { _currentType = type; }
    SkyBoxType GetSkyboxType() const { return _currentType; }
    void SetPanoramicTexture(const GPUTexture& texture);
    GPUTexture& GetPanoramicTexture() {return _panoramicTexture;}
    bool HasTexture() const { return _hasTexture; }

private:
    RealPipeline* _procPipeline{ nullptr };
    RealPipeline* _panoramicPipeline{ nullptr };
    SkyBoxType _currentType{ SkyBoxType::Procedural };

    GPUTexture _panoramicTexture{};
    bool _hasTexture{ false };
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class RenderSystem{
public:
    RenderSystem(VK_INIT_ENGINE::_inited_engine& init) : _init(init){}

    RenderPass* AddPass(std::unique_ptr<RenderPass> pass, PipelineManager& pipelineManager);

    void Submit (RenderObject ro);

    // Сортировка по ключу
    void PrepareFrame();

    void Draw(VkCommandBuffer cmd, VkExtent2D drawExtent,
              VkDescriptorSet globalDescriptor, VkDescriptorSet bindlessTextureSet,
              PipelineManager& pipelineManager, LightManager& lightManager);

    void RefreshPasses(PipelineManager& pipelineManager);

    void SetPassEnabled(RenderPassType type, bool enabled);

    void UpdateSkyBoxTexture(GPUTexture& newTex, TextureManager& textureManager, ComputeRenderSystem& computeSystem);

    void ToggleSkyBox();

    // Очистка очереди
    void ClearQueue() { _mainDrawQueue.clear(); }
private:
    VK_INIT_ENGINE::_inited_engine& _init;
    std::vector<RenderObject> _mainDrawQueue;
    std::vector<std::unique_ptr<RenderPass>> _renderPasses;
};