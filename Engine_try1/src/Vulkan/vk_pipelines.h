#pragma once

#include <fstream>
#include "vk_initializers.h"

namespace vkutil{
bool load_shader_module(const char* filePath,
    VkDevice device,
    VkShaderModule* outShaderModule);
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

    void disable_blending();

    void set_color_attachment_format(VkFormat format);

    void set_depth_format(VkFormat format);

    void disable_depthtest();

    void enable_depthtest(bool depthWriteEnable, VkCompareOp op);

    void enable_blending_additive();

    void enable_blending_alphablend();
};