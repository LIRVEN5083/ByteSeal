#pragma once

#include "vk_initializers.h"
#include "vk_glTF_loading.h"
#include "vk_scene.h"
#include "../Utils/File_loader.h"

namespace VK_GUI{
    // TODO: Тема навайбкожена
    void apply_theme();

    class GUI
    {
    public:
        void draw_imgui(VK_INIT_ENGINE::_inited_engine& _init, VkCommandBuffer cmd, VkExtent2D _drawExtent);
        void update_imgui(VK_INIT_ENGINE::_inited_engine& _init, CONTROLLER::Delta& _delta, ModelManager& _modelManager,
            std::unique_ptr<Scene>& _scene, const GPUSceneData& sceneData, PipelineManager& pipelineManager);
    private:
        void draw_inspector_window(VK_INIT_ENGINE::_inited_engine& _init, ModelManager& modelManager);
        void draw_model_list_overlay(VK_INIT_ENGINE::_inited_engine& _init, ModelManager& _modelManager,
            std::unique_ptr<Scene>& _scene, const GPUSceneData& sceneData, PipelineManager& pipelineManager);
        void draw_fps_overlay(VK_INIT_ENGINE::_inited_engine& _init, CONTROLLER::Delta _delta);
        void draw_context_menu_trs(VK_INIT_ENGINE::_inited_engine& _init, std::unique_ptr<Scene>& _scene, const GPUSceneData& sceneData);
        void draw_gizmo(VK_INIT_ENGINE::_inited_engine& _init, std::unique_ptr<Scene>& _scene, const GPUSceneData& sceneData, ModelManager& _modelManager);
        void gizmo_mode();

        // TODO:: Для model manager и инспектора
        // -1 ну типо то что инспектор пуст
        static inline int selectedModelId = -1;
        // Отображать инспектор чи нет
        static inline bool showInspector = false;

        // TODO:: Для Ray Casting
        static inline int selectedEntityId = -1;
        static inline bool showContextMenu = false;
        // Координаты мыши
        static inline ImVec2 mouseClickPos;

        // TODO:: Для gizmo
        static inline bool showTrsWindow = false;
        static inline ImGuizmo::OPERATION currentGizmoOperation = ImGuizmo::TRANSLATE;
        static inline ImGuizmo::MODE currentGizmoMode = ImGuizmo::WORLD;
    };
}