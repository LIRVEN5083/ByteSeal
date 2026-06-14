#include "vk_initializers.h"

VkCommandPoolCreateInfo vkinit::command_pool_create_info(uint32_t queueFamilyIndex, VkCommandPoolCreateFlags flags){
    VkCommandPoolCreateInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    info.pNext = nullptr;
    info.queueFamilyIndex = queueFamilyIndex;
    info.flags = flags;
    return info;
}

VkCommandBufferAllocateInfo vkinit::command_buffer_allocate_info(VkCommandPool pool, uint32_t count){
    VkCommandBufferAllocateInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    info.pNext = nullptr;

    info.commandPool = pool;
    info.commandBufferCount = count;
    info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    return info;
}

VkFenceCreateInfo vkinit::fence_create_info(VkFenceCreateFlags flags /*= 0*/){
    VkFenceCreateInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    info.pNext = nullptr;

    info.flags = flags;

    return info;
}

VkSemaphoreCreateInfo vkinit::semaphore_create_info(VkSemaphoreCreateFlags flags /*= 0*/){
    VkSemaphoreCreateInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    info.pNext = nullptr;
    info.flags = flags;
    return info;
}

VkCommandBufferBeginInfo vkinit::command_buffer_begin_info(VkCommandBufferUsageFlags flags /*= 0*/){
    VkCommandBufferBeginInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    info.pNext = nullptr;

    info.pInheritanceInfo = nullptr;
    info.flags = flags;
    return info;
}

VkImageSubresourceRange vkinit::image_subresource_range(VkImageAspectFlags aspectMask){
    VkImageSubresourceRange subImage{};
    subImage.aspectMask = aspectMask;
    // Обработка одного единственного изображение (У нас сейчас не интегрирован MipMaping)
    subImage.baseMipLevel = 0;
    // Захватить вообще все уровни MipMaping
    subImage.levelCount = VK_REMAINING_MIP_LEVELS;
    // Обрабатываем с самого первого уровня
    subImage.baseArrayLayer = 0;
    // Взять все существующие количество слоёв 
    // (Сам не понимаю в чём разница между levelCount, но я так понял это НЕ связанно с MipMaping)
    subImage.layerCount = VK_REMAINING_ARRAY_LAYERS;

    return subImage;
}

// Обёртка на один Semaphore, но используем мы её дважды. Первый Semaphore: кого ждать. Второй Semaphore: кому давать сигнал.
VkSemaphoreSubmitInfo vkinit::semaphore_submit_info(VkPipelineStageFlags2 stageMask, VkSemaphore semaphore){
    VkSemaphoreSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
    submitInfo.pNext = nullptr;
    submitInfo.semaphore = semaphore;
    submitInfo.stageMask = stageMask;
    submitInfo.deviceIndex = 0;
    submitInfo.value = 1;

    return submitInfo;
}

// Упаковываем на отправку командный буфер
VkCommandBufferSubmitInfo vkinit::command_buffer_submit_info(VkCommandBuffer cmd){
    VkCommandBufferSubmitInfo info{};
    info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO;
    info.pNext = nullptr;
    info.commandBuffer = cmd;
    info.deviceMask = 0;

    return info;
}

// Передаём два созданых Semaphore и commandBuffer в структуру, а потом структуру отправим в функцию Vulkan 
VkSubmitInfo2 vkinit::submit_info(VkCommandBufferSubmitInfo* cmd, VkSemaphoreSubmitInfo* signalSemaphoreInfo,
    VkSemaphoreSubmitInfo* waitSemaphoreInfo){
    VkSubmitInfo2 info = {};
    info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2;
    info.pNext = nullptr;

    info.waitSemaphoreInfoCount = waitSemaphoreInfo == nullptr ? 0 : 1;
    info.pWaitSemaphoreInfos = waitSemaphoreInfo;

    info.signalSemaphoreInfoCount = signalSemaphoreInfo == nullptr ? 0 : 1;
    info.pSignalSemaphoreInfos = signalSemaphoreInfo;

    info.commandBufferInfoCount = 1;
    info.pCommandBufferInfos = cmd;

    return info;
}
