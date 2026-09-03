#pragma once
#include "vk_types.h"

class PipelineManager;
struct RealPipeline;
struct IBL_TEXTURES;

struct ComputeContext {
    VkCommandBuffer cmd;
    VkDescriptorSet bindlessSet;
    PipelineManager* pipelineManager;
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

    virtual void Execute(const ComputeContext& ctx) = 0;
};

enum class TonemapOperator : uint32_t {
    Linear = 0,
    Reinhard = 1,
    ACES = 2,
    Filmic = 3
};

struct PostProcessSettings {
    // Оператор тонмаппинга (дефолтом ставим имбовый кинематографический ACES)
    TonemapOperator tonemapOp{ TonemapOperator::ACES };

    // Общая экспозиция/яркость (1.0f — стандарт)
    float exposure{ 0.5f };

    // Гамма-коррекция для монитора (2.2f — стандарт для sRGB экранов)
    float gamma{ 2.2f };

    // Насыщенность цветов (0.0f — черно-белое, 1.0f — стандарт, >1.0f — сочнее)
    float saturation{ 1.0f };

    // Контрастность изображения (1.0f — стандарт)
    float contrast{ 1.0f };

    // Цветовой баланс / тинт (дефолт 1.0f по всем осям — чистый белый, без искажений)
    float colorTint[3]{ 1.0f, 1.0f, 1.0f };
};

class ColorCorrectionComputePass : public ComputePass{
public:
    ColorCorrectionComputePass(VK_INIT_ENGINE::_inited_engine& init, std::string pipelineName)
        : ComputePass(init, ComputePassType::ColorCorrection), _pipelineName(pipelineName) {}

    ~ColorCorrectionComputePass() override = default;

    void Execute(const ComputeContext& ctx) override;

    void UpdateSettings(const PostProcessSettings& newSettings) { _settings = newSettings; }

private:
    std::string _pipelineName;
    PostProcessSettings _settings{};
};

class TonemapComputePass : public ComputePass{
public:
    TonemapComputePass(VK_INIT_ENGINE::_inited_engine& init, std::string pipelineName)
        : ComputePass(init, ComputePassType::TonMapping), _pipelineName(pipelineName) {}

    ~TonemapComputePass() override = default;

    void Execute(const ComputeContext& ctx) override;

    void UpdateSettings(const PostProcessSettings& newSettings) { _settings = newSettings; }
private:
    std::string _pipelineName;
    PostProcessSettings _settings{};
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

class PostProcessComputeSystem {
public:
    PostProcessComputeSystem(VK_INIT_ENGINE::_inited_engine& init)
        : _init(init) {}

    ~PostProcessComputeSystem() = default;

    ComputePass* AddPass(std::unique_ptr<ComputePass> pass);

    void Execute(VkCommandBuffer mainCmd, VkDescriptorSet bindlessSet, PipelineManager& pipelineManager);

private:
    VK_INIT_ENGINE::_inited_engine& _init;
    std::vector<std::unique_ptr<ComputePass>> _passes;
};