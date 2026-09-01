#include "vk_compute.h"
#include "vk_pipelines.h"
#include "vk_glTF_loading.h"

void IBLProcessorComputePass::Init(PipelineManager& pipelineManager){}

void IBLProcessorComputePass::Execute(const ComputeContext& ctx){
    VkCommandBuffer cmd = ctx.cmd;
    fmt::print("[IBL Processor] Starting full asynchronous PBR-IBL generation...\n");

    _brdfPipeline     = _pipelineManager.GetPipelineByName("IBL_BrdfLUT");
    _panoramaPipeline = _pipelineManager.GetPipelineByName("IBL_EquirectToCubemap");
    _diffusePipeline  = _pipelineManager.GetPipelineByName("IBL_DiffuseIrradiance");
    _specularPipeline = _pipelineManager.GetPipelineByName("IBL_SpecularPreFilter");

    GPUDrawPushConstants push {};

    if (!_BRDF_LUT_IS_INITED){
        if (_brdfPipeline) {
            vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, _brdfPipeline->pipeline);
            vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, _brdfPipeline->layout, 1, 1, &ctx.bindlessSet, 0, nullptr);

            push.colorTextureID = 6; // Наш BRDF LUT хранится под индексом 6
            vkCmdPushConstants(cmd, _brdfPipeline->layout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT,
                0, sizeof(GPUDrawPushConstants), &push);

            vkCmdDispatch(cmd, 32, 32, 1); // 512x512 при local_size = 16x16

            InsertImageBarrier(cmd, _ibl->BRDF_LUT.image.image,
                VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT,
                VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_GENERAL,
                VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 1, 1);
        }
        _BRDF_LUT_IS_INITED = true;
    }


    if (_panoramaPipeline) {
        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, _panoramaPipeline->pipeline);
        vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, _panoramaPipeline->layout, 1, 1, &ctx.bindlessSet, 0, nullptr);

        push.colorTextureID = 1; // Mip 0 для Specular кубмапы лежит под индексом 1
        vkCmdPushConstants(cmd, _panoramaPipeline->layout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT,
            0, sizeof(GPUDrawPushConstants), &push);

        vkCmdDispatch(cmd, 32, 32, 6);

        InsertImageBarrier(cmd, _ibl->Specular.image.image,
            VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT,
            VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_GENERAL,
            VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 1, 6); // baseMip=0, count=1
    }

    if (_diffusePipeline) {
        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, _diffusePipeline->pipeline);
        vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, _diffusePipeline->layout, 1, 1, &ctx.bindlessSet, 0, nullptr);

        push.colorTextureID = 0; // Diffuse кубмапа лежит под индексом 0
        vkCmdPushConstants(cmd, _diffusePipeline->layout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT,
            0, sizeof(GPUDrawPushConstants), &push);

        vkCmdDispatch(cmd, 2, 2, 6); // 32x32 кубмапа

        InsertImageBarrier(cmd, _ibl->Diffuse.image.image,
            VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT,
            VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_GENERAL,
            VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 1, 6);
    }

    if (_specularPipeline) {
        // ВАЖНО: Перед чтением Mip 0 префильтром, убедимся, что видеокарта дотащила туда все пиксели
        InsertImageBarrier(cmd, _ibl->Specular.image.image,
            VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT,
            VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_GENERAL,
            VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
            0, 1, 6); // baseMip = 0, count = 1 (только нулевой мип)

        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, _specularPipeline->pipeline);
        vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, _specularPipeline->layout, 1, 1, &ctx.bindlessSet, 0, nullptr);

        uint32_t size = 256; // Для Mip 1 размер действительно 256x256

        for (uint32_t mip = 1; mip < 5; ++mip) {
            uint32_t groups = std::max(1u, size / 16);

            // Твоя логика пуш-констант
            push.colorTextureID = 1 + mip; // Запись пойдет в дескрипторы 2, 3, 4, 5
            float roughness = static_cast<float>(mip) / 4.0f;
            push.materialFactors = glm::vec4(roughness, 0.0f, 0.0f, 0.0f);

            vkCmdPushConstants(cmd, _specularPipeline->layout,
                VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT,
                0, sizeof(GPUDrawPushConstants), &push);

            vkCmdDispatch(cmd, groups, groups, 6);

            // Барьер для текущего мипа (запись завершена)
            InsertImageBarrier(cmd, _ibl->Specular.image.image,
                VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT,
                VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_GENERAL,
                VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                mip, 1, 6);

            size /= 2;
        }
    }

    SetEnabled(false);
    fmt::print("[IBL Processor] All 4 pipelines finished successfully. Pass disabled self.\n");
}

void IBLProcessorComputePass::InsertImageBarrier(VkCommandBuffer cmd, VkImage image, VkAccessFlags srcAccess,
    VkAccessFlags dstAccess, VkImageLayout oldLayout, VkImageLayout newLayout, VkPipelineStageFlags srcStage,
    VkPipelineStageFlags dstStage, uint32_t baseMip, uint32_t mipCount, uint32_t layerCount) { // <- Добавили baseMip

    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.srcAccessMask = srcAccess;
    barrier.dstAccessMask = dstAccess;
    barrier.oldLayout = oldLayout;
    barrier.newLayout = newLayout;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = image;
    // Теперь передаем и baseMip, и mipCount корректно:
    barrier.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, baseMip, mipCount, 0, layerCount };

    vkCmdPipelineBarrier(cmd, srcStage, dstStage, 0, 0, nullptr, 0, nullptr, 1, &barrier);
}

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

ComputePass* ComputeRenderSystem::AddPass(std::unique_ptr<ComputePass> pass){
    pass->Init(_pipelineManager);
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

    if (_computeQueue != VK_NULL_HANDLE) {
        vkQueueWaitIdle(_computeQueue);
    }

    return true;
}

void ComputeRenderSystem::SetPassEnabled(ComputePassType type, bool enabled){
    for (auto& pass : _computePasses) {
        if (pass->GetType() == type) {
            pass->SetEnabled(enabled);
            return;
        }
    }
}

void ComputeRenderSystem::RefreshIBL(GPUTexture newPanorama){
    for (auto& pass : _computePasses) {
        if (pass->GetType() == ComputePassType::IBL) {
            auto* iblPass = dynamic_cast<IBLProcessorComputePass*>(pass.get());
            if (iblPass) {
                iblPass->TriggerRecalculation(newPanorama);
                return;
            }
        }
    }
    fmt::print("[ComputeSystem Error] IBLProcessorComputePass not found for recalculation!\n");
}
