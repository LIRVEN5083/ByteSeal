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

    VkSemaphoreSubmitInfo semaphore_submit_info(VkPipelineStageFlags2 stageMask, VkSemaphore semaphore);

    VkCommandBufferSubmitInfo command_buffer_submit_info(VkCommandBuffer cmd);

    VkSubmitInfo2 submit_info(VkCommandBufferSubmitInfo* cmd, VkSemaphoreSubmitInfo* signalSemaphoreInfo,
        VkSemaphoreSubmitInfo* waitSemaphoreInfo);
}