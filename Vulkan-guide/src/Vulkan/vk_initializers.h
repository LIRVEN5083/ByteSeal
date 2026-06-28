#pragma once

#include "vk_types.h"

namespace vkinit {

    // Заполнение структуры для командного пула одной функцией
    VkCommandPoolCreateInfo command_pool_create_info(
        uint32_t queueFamilyIndex,
        VkCommandPoolCreateFlags flags /*= 0*/);

    // Заполнение структуры для Алокация памяти одной функцией 
    VkCommandBufferAllocateInfo command_buffer_allocate_info(
        VkCommandPool pool,
        uint32_t count /*= 1*/);

    // Создание Fence
    VkFenceCreateInfo fence_create_info(VkFenceCreateFlags flags /*= 0*/);

    // Создание Semaphore
    VkSemaphoreCreateInfo semaphore_create_info(VkSemaphoreCreateFlags flags /*= 0*/);

    // Открыть командный буфер на запись
    VkCommandBufferBeginInfo command_buffer_begin_info(VkCommandBufferUsageFlags flags /*= 0*/);

    // Поставить флаг на Aspect
    VkImageSubresourceRange image_subresource_range(VkImageAspectFlags aspectMask);

    // Обёртка на один Semaphore, но используем мы её дважды. Первый Semaphore: кого ждать. Второй Semaphore: кому давать сигнал.
    VkSemaphoreSubmitInfo semaphore_submit_info(VkPipelineStageFlags2 stageMask, VkSemaphore semaphore);

    // Упаковываем на отправку командный буфер
    VkCommandBufferSubmitInfo command_buffer_submit_info(VkCommandBuffer cmd);
    
    // Передаём два созданых Semaphore и commandBuffer в структуру, а потом структуру отправим в функцию Vulkan 
    VkSubmitInfo2 submit_info(VkCommandBufferSubmitInfo* cmd, VkSemaphoreSubmitInfo* signalSemaphoreInfo,
        VkSemaphoreSubmitInfo* waitSemaphoreInfo);

    // Это структура-заполнитель для описания текстуры
    VkImageCreateInfo image_create_info(VkFormat format, VkImageUsageFlags usageFlags, VkExtent3D extent);

    // И не нужно 5 пядей во лбу что-бы сказать что это опять блядский тех паспорт для драйвера который потом пойдёт в дескриптор текстуры.
    VkImageViewCreateInfo imageview_create_info(VkFormat format, VkImage image, VkImageAspectFlags aspectFlags);

    // Это структура-заполнитель нужный что-бы быстро поставить логику очистки и сохранения
    VkRenderingAttachmentInfo attachment_info(
        VkImageView view, VkClearValue* clear, VkImageLayout layout /*= VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL*/);

    // Это структура-заполнитель ОПЯТЬ же для ебанных тех. данных
    VkRenderingInfo rendering_info(VkExtent2D renderExtent, VkRenderingAttachmentInfo* colorAttachment, VkRenderingAttachmentInfo* depthAttachment);

    // Shader stage - это завёрнутый шейдер модуль для графического конвеера
    VkPipelineShaderStageCreateInfo pipeline_shader_stage_create_info(
        VkShaderStageFlagBits stage,
        VkShaderModule shaderModule,
        const char* entryPoint = "main");

    // Создание Layout для графического конвеера
    VkPipelineLayoutCreateInfo pipeline_layout_create_info();

    VkRenderingAttachmentInfo depth_attachment_info(
    VkImageView view, VkImageLayout layout /*= VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL*/);
}