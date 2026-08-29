#pragma once

#include "vk_initializers.h"
#include "vk_glTF_loading.h"
#include "vk_scene.h"
#include "../Utils/File_loader.h"
#include "../Utils/Light.h"

class ModelManager;
class Scene;
class PipelineManager;
struct GPUSceneData;
class RenderSystem;
class TextureManager;
class ComputeRenderSystem;

namespace VK_GUI{
    // TODO: Тема навайбкожена
    void apply_theme();

    class GUI
    {
    public:
        void draw_imgui(VK_INIT_ENGINE::_inited_engine& _init, VkCommandBuffer cmd, VkExtent2D _drawExtent);
        void update_imgui(VK_INIT_ENGINE::_inited_engine& _init, CONTROLLER::Delta& _delta, CONTROLLER::Camera _camera, ModelManager& _modelManager,
            std::unique_ptr<Scene>& _scene, GPUSceneData& sceneData, PipelineManager& pipelineManager, RenderSystem& _renderSystem, TextureManager& _textureManager,
            ComputeRenderSystem& _computeSystem);
    private:
        void draw_inspector_window(VK_INIT_ENGINE::_inited_engine& _init, std::unique_ptr<Scene>& _scene, GPUSceneData& sceneData, CONTROLLER::Camera _camera);
        void draw_settings();
        void draw_skybox_window(RenderSystem& _renderSystem, GPUSceneData& sceneData, VK_INIT_ENGINE::_inited_engine& _init,
            TextureManager& _textureManager, ComputeRenderSystem& _computeSystem);
        void draw_model_properties_window(VK_INIT_ENGINE::_inited_engine& _init, ModelManager& modelManager);
        void draw_model_list_overlay(VK_INIT_ENGINE::_inited_engine& _init, ModelManager& _modelManager,
            std::unique_ptr<Scene>& _scene, GPUSceneData& sceneData, PipelineManager& pipelineManager, RenderSystem& _renderSystem);
        void draw_fps_overlay(VK_INIT_ENGINE::_inited_engine& _init, CONTROLLER::Delta _delta);
        void draw_gizmo(VK_INIT_ENGINE::_inited_engine& _init, std::unique_ptr<Scene>& _scene, GPUSceneData& sceneData, ModelManager& _modelManager, CONTROLLER::Camera _camera);
        void draw_view_navigation_widget(GPUSceneData& sceneData, CONTROLLER::Camera& _camera);
        void gizmo_mode();
        void draw_click(VK_INIT_ENGINE::_inited_engine& _init, std::unique_ptr<Scene>& _scene, GPUSceneData& sceneData, CONTROLLER::Camera _camera);
        void draw_main_menu_bar(CONTROLLER::Delta& _delta);

        // TODO:: Для model manager и инспектора
        // -1 ну типо то что инспектор пуст
        static inline int selectedModelId = -1;
        // Отображать инспектор чи нет
        static inline bool showInspector = false;

        // TODO:: Для Ray Casting
        static inline int selectedEntityId = -1;
        // Координаты мыши
        static inline ImVec2 mouseClickPos;

        // TODO:: Для инспектора
        static inline ImVec2 inspectorPos = ImVec2(0,0);
        static inline float windowWidth;
        static inline float halfHeight;

        // TODO:: Для gizmo
        static inline bool showTrsWindow = false;
        static inline ImGuizmo::OPERATION currentGizmoOperation = ImGuizmo::TRANSLATE;
        static inline ImGuizmo::MODE currentGizmoMode = ImGuizmo::WORLD;

        // TODO:: Для SkyBox
        static inline bool showSkyBoxWindow = false;
        static inline int skyboxType = 0;

        //  TODO:: Параметры скайбокса
        static inline float skyboxTime = 12.0f;
        static inline float skyboxSunPower = 1.0f;
        static inline float skyboxLightColor[3] = { 1.0f, 1.0f, 1.0f };
        static inline float skyboxAmbientColor[3] = { 1.0f, 1.0f, 1.0f };

        // TODO:: Для Settings
        static inline bool showSettings = false;
    };
}