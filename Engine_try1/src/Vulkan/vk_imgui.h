#pragma once

#include "vk_glTF_loading.h"
#include "vk_initializers.h"
#include "vk_scene.h"
#include "../Utils/File_loader.h"

namespace VK_GUI{
    // TODO: Тема навайбкожена
    void apply_theme();

    class GUI
    {
    public:
        void draw_imgui(VK_INIT_ENGINE::_inited_engine& _init, VkCommandBuffer cmd, VkExtent2D _drawExtent);
        void update_imgui(VK_INIT_ENGINE::_inited_engine& _init, CONTROLLER::Delta& _delta, ModelManager& _modelManager, std::unique_ptr<Scene>& _scene);
    private:
        void draw_inspector_window(VK_INIT_ENGINE::_inited_engine& _init, ModelManager& modelManager);
        void draw_model_list_overlay(VK_INIT_ENGINE::_inited_engine& _init, ModelManager& _modelManager, std::unique_ptr<Scene>& _scene);
        void draw_fps_overlay(VK_INIT_ENGINE::_inited_engine& _init, CONTROLLER::Delta _delta);

        // -1 ну типо то что инспектор пуст
        static inline int selectedModelId = -1;
        static inline bool showInspector = false;
    };
}