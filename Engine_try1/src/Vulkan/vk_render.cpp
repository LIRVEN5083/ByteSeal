#include "vk_render.h"



void RenderSystem::Allocate(size_t count){
    _mainDrawQueue.reserve(count);
}

void RenderSystem::Submit(RenderObject ro){
    uint64_t pipelineBits = reinterpret_cast<uint64_t>(ro.pipeline) & 0xFFFFFFFF;
    uint64_t bufferBits = reinterpret_cast<uint64_t>(ro.indexBuffer) & 0xFFFFFFFF;
    ro.sortKey = (pipelineBits << 32) | bufferBits;

    _mainDrawQueue.push_back(ro);
}

void RenderSystem::PrepareFrame(){
    std::sort(_mainDrawQueue.begin(), _mainDrawQueue.end(), [](const RenderObject& a, const RenderObject& b) {
        return a.sortKey < b.sortKey;
    });
}

void RenderSystem::DrawForward(VkCommandBuffer cmd, VkExtent2D drawExtent, VkDescriptorSet globalDescriptor,
    VkDescriptorSet bindlessTextureSet){

    if (_mainDrawQueue.empty()) return;

    VkClearValue clearColor;
    clearColor.color = { { 0.3f, 0.3f, 0.3f, 1.0f } };

    VkClearValue depthClear;
    depthClear.depthStencil.depth = 0.0f;

    VkRenderingAttachmentInfo colorAttachment = vkinit::attachment_info(
        _init._msaaColorImage.imageView,
        &clearColor,
        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
    );
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

    colorAttachment.resolveMode = VK_RESOLVE_MODE_AVERAGE_BIT;
    colorAttachment.resolveImageView = _init._drawImage.imageView;
    colorAttachment.resolveImageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkRenderingAttachmentInfo depthAttachment = vkinit::depth_attachment_info(
        _init._msaaDepthImage.imageView,
        VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL
    );
    depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

    depthAttachment.clearValue = depthClear;

    VkRenderingInfo renderInfo = vkinit::rendering_info(drawExtent, &colorAttachment, &depthAttachment);

    vkCmdBeginRendering(cmd, &renderInfo);

    VkViewport viewport = { 0.0f, 0.0f, (float)drawExtent.width, (float)drawExtent.height, 1.0f, 0.0f };
    vkCmdSetViewport(cmd, 0, 1, &viewport);

    VkRect2D scissor = { {0, 0}, drawExtent };
    vkCmdSetScissor(cmd, 0, 1, &scissor);

    VkDescriptorSet setsToBind[] = { globalDescriptor, bindlessTextureSet };

    VkPipeline currentPipeline = VK_NULL_HANDLE;
    VkBuffer currentIndexBuffer = VK_NULL_HANDLE;

    for (const RenderObject& object : _mainDrawQueue) {
        if (object.pipeline != currentPipeline) {
            vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, object.pipeline);
            vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, object.pipelineLayout, 0, 2, setsToBind, 0, nullptr);
            currentPipeline = object.pipeline;
        }

        if (object.indexBuffer != VK_NULL_HANDLE) {
            if (object.indexBuffer != currentIndexBuffer) {
                vkCmdBindIndexBuffer(cmd, object.indexBuffer, 0, VK_INDEX_TYPE_UINT32);
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

        vkCmdPushConstants(cmd, object.pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(GPUDrawPushConstants), &push_constants);

        if (object.indexBuffer != VK_NULL_HANDLE) {
            vkCmdDrawIndexed(cmd, object.indexCount, 1, object.firstIndex, 0, 0);
        } else {
            vkCmdDraw(cmd, object.indexCount, 1, 0, 0);
        }
    }

    vkCmdEndRendering(cmd);
}