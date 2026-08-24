#pragma once
#include "vk_types.h"

class PipelineManager;

struct ComputeContext {
    VkCommandBuffer cmd;
    VkDescriptorSet bindlessSet;
};

class ComputePass {
public:
    ComputePass(VK_INIT_ENGINE::_inited_engine& init, const std::string& passName)
        : _init(init), _name(passName), _isEnabled(true) {}

    virtual ~ComputePass() = default;

    virtual void Init(PipelineManager& pipelineManager) = 0;

    virtual void Execute(const ComputeContext& ctx) = 0;

    void SetEnabled(bool enabled) { _isEnabled = enabled; }
    bool IsEnabled() const { return _isEnabled; }
    const std::string& GetName() const { return _name; }

protected:
    VK_INIT_ENGINE::_inited_engine& _init;
    std::string _name;
    bool _isEnabled;
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class ComputeRenderSystem {
public:
    ComputeRenderSystem(VK_INIT_ENGINE::_inited_engine& init) : _init(init){}

    void init();

    void cleanup();

    ComputePass* AddPass(std::unique_ptr<ComputePass> pass, PipelineManager& pipelineManager);

    bool Dispatch(VkDescriptorSet bindlessTextureSet);

    VkSemaphore GetComputeSemaphore() const { return _computeFinishedSemaphore; }

    void SetPassEnabled(const std::string& name, bool enabled);

private:
    VK_INIT_ENGINE::_inited_engine& _init;
    VkQueue _computeQueue = VK_NULL_HANDLE;
    VkCommandPool _computeCommandPool = VK_NULL_HANDLE;
    VkCommandBuffer _computeCommandBuffer = VK_NULL_HANDLE;

    VkSemaphore _computeFinishedSemaphore = VK_NULL_HANDLE;
    VkFence _computeFence = VK_NULL_HANDLE;

    std::vector<std::unique_ptr<ComputePass>> _computePasses;
};