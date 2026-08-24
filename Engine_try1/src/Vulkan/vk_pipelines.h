#pragma once

#include <fstream>
#include "vk_initializers.h"

namespace vkutil{
    bool load_shader_module(const char* filePath,
        VkDevice device,
        VkShaderModule* outShaderModule);

    void generate_mipmaps(VkCommandBuffer cmd, VkImage image, VkExtent2D imageSize);
};

/* Структура из библиотеки VK для создания графического конвеера
typedef struct VkGraphicsPipelineCreateInfo {
    VkStructureType                                  sType;
    const void*                                      pNext;
    VkPipelineCreateFlags                            flags;
    uint32_t                                         stageCount;				-Хранит количество шейдер модулей
    const VkPipelineShaderStageCreateInfo*           pStages;					-Хранит в себе шейдер модули
    const VkPipelineVertexInputStateCreateInfo*      pVertexInputState;			-Хранит конфигурация для ВВОДА вершин
    const VkPipelineInputAssemblyStateCreateInfo*    pInputAssemblyState;		-Как работать с вершинами
    const VkPipelineTessellationStateCreateInfo*     pTessellationState;		-Информация для возможной тесселезации
    const VkPipelineViewportStateCreateInfo*         pViewportState;			-Хранит структуру ViewPort
    const VkPipelineRasterizationStateCreateInfo*    pRasterizationState;		-Как растеризуетьтся
    const VkPipelineMultisampleStateCreateInfo*      pMultisampleState;			-MSAA
    const VkPipelineDepthStencilStateCreateInfo*     pDepthStencilState;		-Настройки буфера глубины и трафарета
    const VkPipelineColorBlendStateCreateInfo*       pColorBlendState;			-Смешивание пикселей (Alpha канал)
    const VkPipelineDynamicStateCreateInfo*          pDynamicState;				-Параметры изменяемые без пересоздания Pipeline
    VkPipelineLayout                                 layout;					-Layout для шейдера, место передачи данных
    VkRenderPass                                     renderPass;				-Нам не нужен, из-за dynamic render
    uint32_t                                         subpass;					-Нам не нужен, из-за dynamic render
    VkPipeline                                       basePipelineHandle;
    int32_t                                          basePipelineIndex;
} VkGraphicsPipelineCreateInfo;
 */

// Класс для создания графических конвееров
class PipelineBuilder {
public:
    std::vector<VkPipelineShaderStageCreateInfo> _shaderStages;

    VkPipelineInputAssemblyStateCreateInfo _inputAssembly;
    VkPipelineRasterizationStateCreateInfo _rasterizer;
    VkPipelineColorBlendAttachmentState _colorBlendAttachment;
    VkPipelineMultisampleStateCreateInfo _multisampling;
    VkPipelineLayout _pipelineLayout;
    VkPipelineDepthStencilStateCreateInfo _depthStencil;
    VkPipelineRenderingCreateInfo _renderInfo;
    VkFormat _colorAttachmentformat;

    // Базовый конструктор инициируем пустыми полями
    PipelineBuilder(){ clear(); }

    // Очиста данных экземпляра класса, записыванием пустыми полями
    void clear();

    // Метод создания графического конвеера
    VkPipeline build_pipeline(VkDevice device);

    void set_shaders(VkShaderModule vertexShader, VkShaderModule fragmentShader);

    void set_input_topology(VkPrimitiveTopology topology);

    void set_polygon_mode(VkPolygonMode mode);

    void set_cull_mode(VkCullModeFlags cullMode, VkFrontFace frontFace);

    void set_multisampling_none();

    void set_multisampling(VkSampleCountFlagBits samples = VK_SAMPLE_COUNT_2_BIT);

    void set_multisampling_alpha(VkSampleCountFlagBits samples = VK_SAMPLE_COUNT_2_BIT);

    void disable_blending();

    void set_color_attachment_format(VkFormat format);

    void set_depth_format(VkFormat format);

    void disable_depthtest();

    void enable_depthtest(bool depthWriteEnable, VkCompareOp op);

    void enable_blending_additive();

    void enable_blending_alphablend();
};

struct PipelineKey {
    RenderPassType passType;
    PipelineOpacity opacity;

    bool operator==(const PipelineKey& other) const {
        return passType == other.passType && opacity == other.opacity;
    }
};

namespace std {
    template<>
    struct hash<PipelineKey> {
        size_t operator()(const PipelineKey& key) const noexcept {
            // Комбинируем два enum в один size_t без коллизий
            return (static_cast<size_t>(key.passType) << 8) | static_cast<size_t>(key.opacity);
        }
    };
}

struct PipelineCreateInfo {
    // Ident name
    std::string name;
    // Проход рендера
    RenderPassType passType;
    // Enum class
    PipelineOpacity opacity;
    // MSAA
    bool useMSAA{ false };

    // Shaders
    std::string vertexShaderPath;
    std::string fragmentShaderPath;
    std::string computeShaderPath;
};


struct RealPipeline {
    std::string name;
    VkPipeline pipeline{ VK_NULL_HANDLE };
    VkPipelineLayout layout{ VK_NULL_HANDLE };
    RenderPassType passType;
    PipelineOpacity opacity;

    uint16_t id{ 0 };

    std::vector<uint32_t> vertSPIRVCode;
    std::vector<uint32_t> fragSPIRVCode;

    std::string vertexShaderPath;
    std::string fragmentShaderPath;
    std::string computeShaderPath;

    VkFormat colorFormat{ VK_FORMAT_UNDEFINED };
    VkFormat depthFormat{ VK_FORMAT_UNDEFINED };
    VkSampleCountFlagBits maxSamples{ VK_SAMPLE_COUNT_1_BIT };
    bool isCompute = false;
};

class PipelineManager{
public:
    PipelineManager(VkDevice device) : _device(device){}

    void InitCommonLayout(VkDescriptorSetLayout globalSetLayout, VkDescriptorSetLayout bindlessSetLayout);

    RealPipeline* CreateComputePipeline(const PipelineCreateInfo& info);

    RealPipeline* CreatePipelineFromMemory(const PipelineCreateInfo& info, const std::vector<uint32_t>& compCode);

    RealPipeline* CreatePipeline(const PipelineCreateInfo& info, VkFormat colorFormat, VkFormat depthFormat, VkSampleCountFlagBits maxSamples);

    RealPipeline* CreatePipelineFromMemory(const PipelineCreateInfo& info,
                                           const std::vector<uint32_t>& vertCode,
                                           const std::vector<uint32_t>& fragCode,
                                           VkFormat colorFormat, VkFormat depthFormat, VkSampleCountFlagBits maxSamples);

    RealPipeline* GetPipeline(RenderPassType passType, PipelineOpacity opacity);

    RealPipeline* GetPipelineByName(const std::string& name);

    bool DestroyPipeline(const std::string& name);

    void DestroyAllPipelines();

    bool ReloadAllPipelines();

    void cleanup();

    VkPipelineLayout GetCommonLayout() const;
    VkPipelineLayout GetShadowLayout() const;

private:
    VkDevice _device;
    VkPipelineLayout _commonLayout{VK_NULL_HANDLE};
    VkPipelineLayout _shadowLayout{VK_NULL_HANDLE};

    // Карта по именам
    std::unordered_map<std::string, RealPipeline> _pipelinesByName;

    // Карта для пассов
    std::unordered_map<PipelineKey, RealPipeline*> _pipelinesByKey;
};