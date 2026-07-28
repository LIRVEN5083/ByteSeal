#include "vk_imgui.h"

void VK_GUI::draw_fps_overlay(VK_INIT_ENGINE::_inited_engine& _init, CONTROLLER::Delta _delta){
    float fps = (_delta.delta > 0.00001f && std::isfinite(_delta.delta)) ? (1.0f / _delta.delta) : 0.0f;

    static float smoothedFps = 60.0f;

    if (std::isfinite(fps) && fps > 0.0f) {
        smoothedFps = glm::mix(smoothedFps, fps, 0.05f);
    }

    ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoDecoration |
                                   ImGuiWindowFlags_AlwaysAutoResize |
                                   ImGuiWindowFlags_NoSavedSettings |
                                   ImGuiWindowFlags_NoFocusOnAppearing |
                                   ImGuiWindowFlags_NoNav |
                                   ImGuiWindowFlags_NoMove;

    int windowWidth = 0;
    int windowHeight = 0;
    SDL_GetWindowSize(_init._window, &windowWidth, &windowHeight);

    float padding = 10.0f;
    float posX = static_cast<float>(windowWidth) - padding;
    float posY = padding;

    ImGui::SetNextWindowPos(ImVec2(posX, posY), ImGuiCond_Always, ImVec2(1.0f, 0.0f));
    ImGui::SetNextWindowBgAlpha(0.35f);

    if (ImGui::Begin("##FPS_Overlay", nullptr, windowFlags)) {
        ImGui::Text("FPS: %.1f", smoothedFps);
    }
    ImGui::End();
}

void VK_GUI::draw_imgui(VK_INIT_ENGINE::_inited_engine& _init, VkCommandBuffer cmd, VkExtent2D _drawExtent){
    VkRenderingAttachmentInfo colorAttachment = vkinit::attachment_info(_init._drawImage.imageView, nullptr, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD; // <-- СОХРАНЯЕМ цвет машины!
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;

    VkRenderingAttachmentInfo depthAttachment = vkinit::depth_attachment_info(_init._depthImage.imageView, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL);
    depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD; // <-- СОХРАНЯЕМ глубину машины!
    depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;

    VkRenderingInfo renderInfo = vkinit::rendering_info(_drawExtent, &colorAttachment, nullptr);
    vkCmdBeginRendering(cmd, &renderInfo);

    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cmd);

    vkCmdEndRendering(cmd);
}

void VK_GUI::update_imgui(VK_INIT_ENGINE::_inited_engine& _init, CONTROLLER::Delta _delta){
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplSDL3_NewFrame();
    ImGui::NewFrame();

    draw_fps_overlay(_init, _delta);

    ImGui::Render();
}
