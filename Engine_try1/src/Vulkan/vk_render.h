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

    // Ключ для сортировки
    uint64_t sortKey{0};

    AABB worldAABB;
};

struct RenderContext {
    VkCommandBuffer cmd;
    VkExtent2D drawExtent;
    VkDescriptorSet globalDescriptor;
    VkDescriptorSet bindlessTextureSet;
};

class RenderPass {
protected:
    VK_INIT_ENGINE::_inited_engine& _init;

public:
    RenderPass(VK_INIT_ENGINE::_inited_engine& init) : _init(init) {}
    virtual ~RenderPass() = default;

    // Каждый пасс сам настраивает свои пайплайны через PipelineManager
    virtual void Init(PipelineManager& pipelineManager) = 0;

    // Главный метод отрисовки, который каждый класс реализует по-своему
    virtual void Execute(const RenderContext& ctx, const std::vector<RenderObject>& queue) = 0;
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