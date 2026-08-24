#pragma once
#include "vk_types.h"
#include "vk_render.h"
#include "vk_scene.h"
#include "vk_descriptors.h"
#include "vk_scene.h"

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

        void cleanup();
        void run();

        private:
        VK_INIT_ENGINE::_inited_engine& _init;
        FrameData _frames[FRAME_OVERLAP];

        bool resize_requested = false;
        VkExtent2D _drawExtent;
        float renderScale = 1.0f;

        GPUSceneData sceneData;
        VkDescriptorSetLayout _gpuSceneDataDescriptorLayout;
        VkDescriptorPool _sceneDescriptorPool;


        CONTROLLER::Movement _movement;
        CONTROLLER::Camera _camera;
        CONTROLLER::Delta _delta;

        VkSampleCountFlagBits _maxSamples;

        // Для теста нодов
        float angle{0.0f};

        std::unique_ptr<PipelineManager> _pipelineManager;
        TextureManager _textureManager;
        MeshManager _meshManager;
        ModelManager _modelManager{_init, _meshManager, _textureManager};
        RenderSystem _renderSystem{_init};
        std::unique_ptr<Scene> _activeScene;
        std::unique_ptr<LightManager> _lightManager;
        VK_GUI::GUI _gui;

        void renderLoop();
        void resize_swapchain();
        void destroy_swapchain();

        void init_descriptors();
        void init_pipeline_manager();
        void init_commands();
        void init_scene();

        VkDescriptorSet update_scene_data(FrameData& currentFrame);
    };
}