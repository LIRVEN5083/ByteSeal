#include "vk_render.h"


void ForwardRenderPass::Init(PipelineManager& pipelineManager){
    _opaquePipeline      = pipelineManager.GetPipeline(RenderPassType::Forward, PipelineOpacity::Opaque);
    _alphaTestedPipeline  = pipelineManager.GetPipeline(RenderPassType::Forward, PipelineOpacity::AlphaTested);
    _transparentPipeline  = pipelineManager.GetPipeline(RenderPassType::Forward, PipelineOpacity::Transparent);
}

void ForwardRenderPass::Execute(const RenderContext& ctx, const std::vector<RenderObject>& queue){
    VkClearValue clearColor;
    clearColor.color = { { 0.3f, 0.3f, 0.3f, 1.0f } };

    VkRenderingAttachmentInfo colorAttachment = vkinit::attachment_info(_init._msaaColorImage.imageView, &clearColor, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.resolveMode = VK_RESOLVE_MODE_NONE;
    colorAttachment.resolveImageView = VK_NULL_HANDLE;

    VkClearValue depthClear;
    depthClear.depthStencil.depth = 0.0f;
    VkRenderingAttachmentInfo depthAttachment = vkinit::depth_attachment_info(_init._msaaDepthImage.imageView, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL);
    depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    depthAttachment.clearValue = depthClear;

    VkRenderingInfo renderInfo = vkinit::rendering_info(ctx.drawExtent, &colorAttachment, &depthAttachment);

    vkCmdBeginRendering(ctx.cmd, &renderInfo);

    VkViewport viewport = { 0.0f, 0.0f, (float)ctx.drawExtent.width, (float)ctx.drawExtent.height, 1.0f, 0.0f };
    vkCmdSetViewport(ctx.cmd, 0, 1, &viewport);

    VkRect2D scissor = { {0, 0}, ctx.drawExtent };
    vkCmdSetScissor(ctx.cmd, 0, 1, &scissor);

    VkDescriptorSet setsToBind[] = { ctx.globalDescriptor, ctx.bindlessTextureSet };

    VkPipeline currentPipeline = VK_NULL_HANDLE;
    VkBuffer currentIndexBuffer = VK_NULL_HANDLE;

    for (const auto& object : queue) {

        if (object.pipeline != currentPipeline) {
            vkCmdBindPipeline(ctx.cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, object.pipeline);
            vkCmdBindDescriptorSets(ctx.cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, object.pipelineLayout, 0, 2, setsToBind, 0, nullptr);
            currentPipeline = object.pipeline;
        }

        if (object.indexBuffer != VK_NULL_HANDLE) {
            if (object.indexBuffer != currentIndexBuffer) {
                vkCmdBindIndexBuffer(ctx.cmd, object.indexBuffer, 0, VK_INDEX_TYPE_UINT32);
                currentIndexBuffer = object.indexBuffer;
            }
        }

        GPUDrawPushConstants push_constants;
        push_constants.render_matrix = object.render_matrix;
        push_constants.vertexBuffer = object.vertexBufferAddress;
        push_constants.colorTextureID = object.colorTextureID;
        push_constants.metallicRoughnessTextureID = object.metallicRoughnessTextureID;
        push_constants.normalTextureID = object.normalTextureID;
        push_constants.occlusionTextureID = object.occlusionTextureID;
        push_constants.baseColorFactor = object.baseColorFactor;
        push_constants.materialFactors = object.materialFactors;

        vkCmdPushConstants(ctx.cmd, object.pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(GPUDrawPushConstants), &push_constants);

        if (object.indexBuffer != VK_NULL_HANDLE) {
            vkCmdDrawIndexed(ctx.cmd, object.indexCount, 1, object.firstIndex, 0, 0);
        } else {
            vkCmdDraw(ctx.cmd, object.indexCount, 1, 0, 0);
        }
    }

    vkCmdEndRendering(ctx.cmd);
}

void ForwardRenderPass::DrawFilteredObjects(const RenderContext& ctx, const std::vector<RenderObject>& queue,
    PipelineOpacity opacityFilter){
    VkBuffer currentIndexBuffer = VK_NULL_HANDLE;

    for (const auto& object : queue) {
        if (object.opacity != opacityFilter) continue; // Фильтруем объекты для текущего под-этапа

        if (object.indexBuffer != VK_NULL_HANDLE) {
            if (object.indexBuffer != currentIndexBuffer) {
                vkCmdBindIndexBuffer(ctx.cmd, object.indexBuffer, 0, VK_INDEX_TYPE_UINT32);
                currentIndexBuffer = object.indexBuffer;
            }
        }

        GPUDrawPushConstants push_constants;
        push_constants.render_matrix = object.render_matrix;
        push_constants.vertexBuffer = object.vertexBufferAddress;

        push_constants.colorTextureID = object.colorTextureID;
        push_constants.metallicRoughnessTextureID = object.metallicRoughnessTextureID;
        push_constants.normalTextureID = object.normalTextureID;
        push_constants.occlusionTextureID = object.occlusionTextureID;

        push_constants.baseColorFactor = object.baseColorFactor;
        push_constants.materialFactors = object.materialFactors;

        vkCmdPushConstants(ctx.cmd, ctx.pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(GPUDrawPushConstants), &push_constants);
        if (object.indexBuffer != VK_NULL_HANDLE) {
            vkCmdDrawIndexed(ctx.cmd, object.indexCount, 1, object.firstIndex, 0, 0);
        } else {
            vkCmdDraw(ctx.cmd, object.indexCount, 1, 0, 0);
        }
    }
}

void GridRenderPass::Init(PipelineManager& pipelineManager){
    _gridPipeline = pipelineManager.GetPipelineByName("Grid");
}

void GridRenderPass::Execute(const RenderContext& ctx, const std::vector<RenderObject>& queue){
    if (!_gridPipeline) return;

    VkRenderingAttachmentInfo colorAttachment = vkinit::attachment_info(_init._msaaColorImage.imageView, nullptr, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

    colorAttachment.resolveMode = VK_RESOLVE_MODE_AVERAGE_BIT;
    colorAttachment.resolveImageView = _init._drawImage.imageView;
    colorAttachment.resolveImageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkRenderingAttachmentInfo depthAttachment = vkinit::depth_attachment_info(_init._msaaDepthImage.imageView, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL);
    depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
    depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

    VkRenderingInfo renderInfo = vkinit::rendering_info(ctx.drawExtent, &colorAttachment, &depthAttachment);

    vkCmdBeginRendering(ctx.cmd, &renderInfo);

    VkViewport viewport = { 0.0f, 0.0f, (float)ctx.drawExtent.width, (float)ctx.drawExtent.height, 1.0f, 0.0f };
    vkCmdSetViewport(ctx.cmd, 0, 1, &viewport);

    VkRect2D scissor = { {0, 0}, ctx.drawExtent };
    vkCmdSetScissor(ctx.cmd, 0, 1, &scissor);

    vkCmdBindPipeline(ctx.cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, _gridPipeline->pipeline);

    VkDescriptorSet setsToBind[] = { ctx.globalDescriptor, ctx.bindlessTextureSet };
    vkCmdBindDescriptorSets(ctx.cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, _gridPipeline->layout, 0, 2, setsToBind, 0, nullptr);

    GPUDrawPushConstants push_constants{};
    push_constants.render_matrix = glm::mat4(1.0f);
    push_constants.vertexBuffer = 0;
    push_constants.colorTextureID = 0;
    push_constants.metallicRoughnessTextureID = 0;

    vkCmdPushConstants(ctx.cmd, _gridPipeline->layout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(GPUDrawPushConstants), &push_constants);

    // Рисуем процедурную сетку без индексов
    vkCmdDraw(ctx.cmd, 6, 1, 0, 0);

    vkCmdEndRendering(ctx.cmd);
}

void ShadowCSMRenderPass::Init(PipelineManager& pipelineManager){
    _shadowPipeline = pipelineManager.GetPipeline(RenderPassType::ShadowCSM, PipelineOpacity::Opaque);
}

void ShadowCSMRenderPass::Execute(const RenderContext& ctx, const std::vector<RenderObject>& queue){
     if (queue.empty() || !_shadowPipeline || !ctx.lightManager) return;

    uint32_t resolution = ctx.lightManager->GetResolution();
    VkExtent2D shadowExtent = { resolution, resolution };

    VkClearValue depthClear;
    depthClear.depthStencil.depth = 0.0f; // Reversed-Z для теней

    // Твой LightManager регистрирует текстуру в TextureManager под определенным ID.
    // Нам нужен VkImageView всего массива слоев для Dynamic Rendering:
    VkImageView shadowArrayView = ctx.lightManager->GetShadowTextureView();

    VkRenderingAttachmentInfo depthAttachment{};
    depthAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
    depthAttachment.pNext = nullptr;
    depthAttachment.imageView = shadowArrayView;
    depthAttachment.imageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
    depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    depthAttachment.clearValue = depthClear;

    VkRenderingInfo renderInfo{};
    renderInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
    renderInfo.pNext = nullptr;
    renderInfo.renderArea = { {0, 0}, shadowExtent };
    renderInfo.layerCount = SHADOW_CASCADES_COUNT; // 4 слоя
    renderInfo.colorAttachmentCount = 0;
    renderInfo.pColorAttachments = nullptr;
    renderInfo.pDepthAttachment = &depthAttachment;

    vkCmdBeginRendering(ctx.cmd, &renderInfo);

    VkViewport viewport = { 0.0f, 0.0f, (float)resolution, (float)resolution, 0.0f, 1.0f };
    vkCmdSetViewport(ctx.cmd, 0, 1, &viewport);

    VkRect2D scissor = { {0, 0}, shadowExtent };
    vkCmdSetScissor(ctx.cmd, 0, 1, &scissor);

    vkCmdBindPipeline(ctx.cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, _shadowPipeline->pipeline);

    vkCmdBindDescriptorSets(ctx.cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, _shadowPipeline->layout, 0, 1, &ctx.globalDescriptor, 0, nullptr);

    VkBuffer currentIndexBuffer = VK_NULL_HANDLE;

    for (const auto& object : queue) {
        if (object.pipeline == VK_NULL_HANDLE) continue;

        if (object.indexBuffer != VK_NULL_HANDLE) {
            if (object.indexBuffer != currentIndexBuffer) {
                vkCmdBindIndexBuffer(ctx.cmd, object.indexBuffer, 0, VK_INDEX_TYPE_UINT32);
                currentIndexBuffer = object.indexBuffer;
            }
        }

        GPUShadowPushConstants push_constants;
        push_constants.worldMatrix = object.render_matrix;
        push_constants.vertexBuffer = object.vertexBufferAddress;

        vkCmdPushConstants(ctx.cmd, _shadowPipeline->layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(GPUShadowPushConstants), &push_constants);

        if (object.indexBuffer != VK_NULL_HANDLE) {
            vkCmdDrawIndexed(ctx.cmd, object.indexCount, SHADOW_CASCADES_COUNT, object.firstIndex, 0, 0);
        } else {
            vkCmdDraw(ctx.cmd, object.indexCount, SHADOW_CASCADES_COUNT, 0, 0);
        }
    }

    vkCmdEndRendering(ctx.cmd);
}

void RenderSystem::AddPass(std::unique_ptr<RenderPass> pass, PipelineManager& pipelineManager){
    pass->Init(pipelineManager);
    _renderPasses.push_back(std::move(pass));
}

void RenderSystem::Submit(RenderObject ro){
    _mainDrawQueue.push_back(ro);
}

void RenderSystem::PrepareFrame(){
    if (_mainDrawQueue.empty()) return;

    std::sort(_mainDrawQueue.begin(), _mainDrawQueue.end(), [](const RenderObject& a, const RenderObject& b) {
        return a.indexBuffer < b.indexBuffer;
    });
}

void RenderSystem::Draw(VkCommandBuffer cmd, VkExtent2D drawExtent, VkDescriptorSet globalDescriptor,
    VkDescriptorSet bindlessTextureSet, PipelineManager& pipelineManager){

    RenderContext ctx{
        cmd,
        drawExtent,
        globalDescriptor,
        bindlessTextureSet,
        pipelineManager.GetCommonLayout()
    };

    // Последовательно выполняем все зарегистрированные пассы
    for (auto& pass : _renderPasses) {
        pass->Execute(ctx, _mainDrawQueue);
    }
}

void RenderSystem::RefreshPasses(PipelineManager& pipelineManager){
    for (auto& pass : _renderPasses) {
        pass->Init(pipelineManager);
    }
    std::cout << "[RenderSystem] All render passes successfully re-linked to new pipelines.\n";
}
