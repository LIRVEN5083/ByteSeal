#include "vk_compute.h"
#include "vk_pipelines.h"

void ComputeRenderSystem::init(){
    vkGetDeviceQueue(_init._device, _init._computeQueueFamily, 0, &_computeQueue);

    // Создаем пул команд с флагом сброса
    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.queueFamilyIndex = _init._computeQueueFamily;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

    if (vkCreateCommandPool(_init._device, &poolInfo, nullptr, &_computeCommandPool) != VK_SUCCESS) {
        throw std::runtime_error("[ComputeRenderSystem] Failed to create command pool!");
    }

    // Выделяем буфер
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = _computeCommandPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = 1;

    if (vkAllocateCommandBuffers(_init._device, &allocInfo, &_computeCommandBuffer) != VK_SUCCESS) {
        throw std::runtime_error("[ComputeRenderSystem] Failed to allocate command buffer!");
    }

    // Этот пупс связывает эту систему и основную систему рендера
    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    vkCreateSemaphore(_init._device, &semaphoreInfo, nullptr, &_computeFinishedSemaphore);

    // Пупсик для синхрона в цикле
    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT; // Сразу готов к первому кадру
    vkCreateFence(_init._device, &fenceInfo, nullptr, &_computeFence);
}

void ComputeRenderSystem::cleanup(){
    vkDestroyFence(_init._device, _computeFence, nullptr);
    vkDestroySemaphore(_init._device, _computeFinishedSemaphore, nullptr);
    vkDestroyCommandPool(_init._device, _computeCommandPool, nullptr);
}

ComputePass* ComputeRenderSystem::AddPass(std::unique_ptr<ComputePass> pass, PipelineManager& pipelineManager){
    pass->Init(pipelineManager);
    _computePasses.push_back(std::move(pass));
    return _computePasses.back().get();
}

bool ComputeRenderSystem::Dispatch(VkDescriptorSet bindlessTextureSet){
    bool hasActivePasses = false;
    for (const auto& pass : _computePasses) {
        if (pass->IsEnabled()) { hasActivePasses = true; break; }
    }
    // Нету вычислительных проходов - иди нахуй
    if (!hasActivePasses) return false;

    // Ждем прошлый Fence
    vkWaitForFences(_init._device, 1, &_computeFence, VK_TRUE, UINT64_MAX);
    vkResetFences(_init._device, 1, &_computeFence);

    // Заапись
    vkResetCommandBuffer(_computeCommandBuffer, 0);
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    if (vkBeginCommandBuffer(_computeCommandBuffer, &beginInfo) != VK_SUCCESS) return false;

    ComputeContext ctx{};
    ctx.cmd = _computeCommandBuffer;
    ctx.bindlessSet = bindlessTextureSet;

    // Вызываем красивенько все наши вычислительные ПрОООходы
    for (auto& pass : _computePasses) {
        if (pass->IsEnabled()) {
            pass->Execute(ctx);
        }
    }

    // Закрытие записи
    vkEndCommandBuffer(_computeCommandBuffer);

    // Submit в Compute-очередь
    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &_computeCommandBuffer;
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = &_computeFinishedSemaphore; // Сигналим графике

    vkQueueSubmit(_computeQueue, 1, &submitInfo, _computeFence);

    return true;
}

void ComputeRenderSystem::SetPassEnabled(const std::string& name, bool enabled){
    for (auto& pass : _computePasses) {
        if (pass->GetName() == name) {
            pass->SetEnabled(enabled);
            return;
        }
    }
}
