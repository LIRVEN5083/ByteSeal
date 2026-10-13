#pragma once
#include "vk_types.h"
#include "vk_render.h"
#include "vk_compute.h"
#include "vk_scene.h"
#include "vk_descriptors.h"
#include "vk_scene.h"
#include "../Utils/Light.h"

// Пул и буфер на каждый кадр
struct FrameData {
    // Очередь удаления
    DeletionQueue _deletionQueue;

    // Сцена
    AllocatedBuffer gpuSceneDataBuffer;

    // пул команд (аллокатор для командного буфера)
    // Так-же пул намертво привязан к очереди, то-есть команндные пулы для вычеслений и графики это должны быть разные сущности
    VkCommandPool _commandPool;
    VkCommandBuffer _mainCommandBuffer;

    // Наша новая система аллокации дескрипторов
    DescriptorAllocatorGrowable _frameDescriptors;

    VkDescriptorSet sceneDescriptorSet;
};

namespace VK_APPLICATION {

    class VulkanApplication{

    public:
        int _frameNumber {0};
        bool stop_rendering{ false };
        VulkanApplication(VK_INIT_ENGINE::_inited_engine& inited_engine);

        //TODO: Основной цикл выполнения программы
        void run();
        // Очистка ресурсов выполнения программы
        void cleanup();

    private:
        VK_INIT_ENGINE::_inited_engine& _init;
        FrameData _frames[FRAME_OVERLAP];

        bool showInterface = true;

        bool resize_requested = false;
        VkExtent2D _drawExtent;
        float renderScale = 1.0f;

        GPUSceneData sceneData;
        VkDescriptorSetLayout _gpuSceneDataDescriptorLayout;
        VkDescriptorPool _sceneDescriptorPool;

        CONTROLLER::Movement _movement;
        CONTROLLER::Camera _camera;
        CONTROLLER::Delta _delta;

        std::unique_ptr<PipelineManager> _pipelineManager;
        TextureManager _textureManager;
        MeshManager _meshManager;
        ModelManager _modelManager{_init, _meshManager, _textureManager};
        TransformBufferManager _transformManager;

        RenderSystem _renderSystem{_init};
        std::unique_ptr<ComputeRenderSystem> _computeSystem;
        PostProcessComputeSystem _postProcessSystem{_init};

        std::unique_ptr<Scene> _activeScene;
        std::unique_ptr<LightManager> _lightManager;
        ProceduralPonarama _proceduralPonarama;
        TAA _TAA{_init};
        VK_GUI::GUI _gui;

        // Основной цикл рендера
        void renderLoop();

        // Пересоздания свапчейна
        void resize_swapchain();
        void destroy_swapchain();

        // Создание коммандных буферов и дескриптор пулов для SceneData
        void init_commands();
        // Дескрипторы для SceneData
        void init_descriptors();
        // Инициация систем
        void init_scene();
        // Инициация систем рендера, проходов рендера, шейдеров
        void init_render();

        // Обновления каждый кадр данных о сцене
        VkDescriptorSet update_scene_data(FrameData& currentFrame);
    };
}