#pragma once
#include "vk_types.h"

class PipelineManager;
struct RealPipeline;
struct IBL_TEXTURES;

struct ComputeContext {
    VkCommandBuffer cmd;
    VkDescriptorSet bindlessSet;
};

class ComputePass {
protected:
    VK_INIT_ENGINE::_inited_engine& _init;
    ComputePassType _type;
    bool _isEnabled;
public:
    ComputePass(VK_INIT_ENGINE::_inited_engine& init, ComputePassType type)
        : _init(init), _type(type), _isEnabled(true) {}

    virtual ~ComputePass() = default;

    ComputePassType GetType() const { return _type; }

    void SetEnabled(bool enabled) { _isEnabled = enabled; }
    bool IsEnabled() const { return _isEnabled; }
    void Toggle() { _isEnabled = !_isEnabled;}

    virtual void Init(PipelineManager& pipelineManager) = 0;

    virtual void Execute(const ComputeContext& ctx) = 0;

};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ПРОХОД ДЛЯ ЕБУЧЕГО IBL
class IBLProcessorComputePass : public ComputePass {
public:
    IBLProcessorComputePass(VK_INIT_ENGINE::_inited_engine& init,
                            const IBL_TEXTURES* iblTextures,
                            GPUTexture panoramaTexture, PipelineManager& pipelineManager)
        : ComputePass(init, ComputePassType::IBL), // Один общий тип пасса
          _ibl(iblTextures),
          _panorama(panoramaTexture),
          _pipelineManager(pipelineManager){}

    ~IBLProcessorComputePass() override = default;

    void Init(PipelineManager& pipelineManager) override;

    void Execute(const ComputeContext& ctx) override;

    void TriggerRecalculation(GPUTexture newPanorama) {
        _panorama = newPanorama;
        _needsRecalculation = true;
        SetEnabled(true);
    }

private:
    PipelineManager& _pipelineManager;

    RealPipeline* _brdfPipeline{ nullptr };
    RealPipeline* _panoramaPipeline{ nullptr };
    RealPipeline* _diffusePipeline{ nullptr };
    RealPipeline* _specularPipeline{ nullptr };

    static inline bool _BRDF_LUT_IS_INITED = false;

    const IBL_TEXTURES* _ibl;
    GPUTexture _panorama;

    void InsertImageBarrier(VkCommandBuffer cmd, VkImage image, VkAccessFlags srcAccess,
    VkAccessFlags dstAccess, VkImageLayout oldLayout, VkImageLayout newLayout, VkPipelineStageFlags srcStage,
    VkPipelineStageFlags dstStage, uint32_t baseMip, uint32_t mipCount, uint32_t layerCount);

    bool _needsRecalculation = true;
};


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class ComputeRenderSystem {
public:
    ComputeRenderSystem(VK_INIT_ENGINE::_inited_engine& init, PipelineManager& pipelineManager)
        : _init(init), _pipelineManager(pipelineManager) {}

    void init();

    void cleanup();

    ComputePass* AddPass(std::unique_ptr<ComputePass> pass);

    bool Dispatch(VkDescriptorSet bindlessTextureSet);

    VkSemaphore GetComputeSemaphore() const { return _computeFinishedSemaphore; }

    void SetPassEnabled(ComputePassType type, bool enabled);

    void RefreshIBL(GPUTexture newPanorama);
private:
    VK_INIT_ENGINE::_inited_engine& _init;
    VkQueue _computeQueue = VK_NULL_HANDLE;
    VkCommandPool _computeCommandPool = VK_NULL_HANDLE;
    VkCommandBuffer _computeCommandBuffer = VK_NULL_HANDLE;

    VkSemaphore _computeFinishedSemaphore = VK_NULL_HANDLE;
    VkFence _computeFence = VK_NULL_HANDLE;

    PipelineManager& _pipelineManager;

    std::vector<std::unique_ptr<ComputePass>> _computePasses;
};