#pragma once

#include "vk_types.h"
#include "vk_glTF_loading.h"

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
};

class RenderPass {
protected:
    VK_INIT_ENGINE::_inited_engine& _init;
    RenderPassType _type;

public:
    RenderPass(VK_INIT_ENGINE::_inited_engine& init, RenderPassType type)
        : _init(init), _type(type) {}

    virtual ~RenderPass() = default;

    RenderPassType GetType() const { return _type; }

    // Вызывается при старте и после ReloadShaders
    virtual void Init(PipelineManager& pipelineManager) = 0;

    // Запись команд рендеринга для текущего пасса
    virtual void Execute(const RenderContext& ctx, const std::vector<RenderObject>& queue) = 0;
};

// Основной проход рендера
class ForwardRenderPass : public RenderPass{
public:
    ForwardRenderPass(VK_INIT_ENGINE::_inited_engine& init)
        : RenderPass(init, RenderPassType::Forward){}

    void Init(PipelineManager& pipelineManager) override;

    void Execute(const RenderContext& ctx, const std::vector<RenderObject>& queue) override;

private:
    void DrawFilteredObjects(const RenderContext& ctx, const std::vector<RenderObject>& queue, PipelineOpacity opacityFilter);

    RealPipeline* _opaquePipeline{ nullptr };
    RealPipeline* _alphaTestedPipeline{ nullptr };
    RealPipeline* _transparentPipeline{ nullptr };
};

class GridRenderPass : public RenderPass {
public:
    GridRenderPass(VK_INIT_ENGINE::_inited_engine& init)
        : RenderPass(init, RenderPassType::Forward) {}
    ~GridRenderPass() override = default;

    void Init(PipelineManager& pipelineManager) override;
    void Execute(const RenderContext& ctx, const std::vector<RenderObject>& queue) override;

private:
    RealPipeline* _gridPipeline{ nullptr };
};

class RenderSystem{
public:
    RenderSystem(VK_INIT_ENGINE::_inited_engine& init) : _init(init){}

    void AddPass(std::unique_ptr<RenderPass> pass, PipelineManager& pipelineManager);

    void Submit (RenderObject ro);

    // Сортировка по ключу
    void PrepareFrame();

    void Draw(VkCommandBuffer cmd, VkExtent2D drawExtent,
              VkDescriptorSet globalDescriptor, VkDescriptorSet bindlessTextureSet,
              PipelineManager& pipelineManager);

    // Вызывается из Engine.cpp сразу после успешного ReloadAllPipelines()
    void RefreshPasses(PipelineManager& pipelineManager);

    // Очистка очереди
    void ClearQueue() { _mainDrawQueue.clear(); }
private:
    VK_INIT_ENGINE::_inited_engine& _init;
    std::vector<RenderObject> _mainDrawQueue;
    std::vector<std::unique_ptr<RenderPass>> _renderPasses;
};