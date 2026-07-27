#pragma once
#include "vk_descriptors.h"
#include "vk_scene.h"

// Очередь удаления
// Короче мы используем функцию для создание и пишем лямбду на удаление.
// А когда вызываем flush то удаляем накопленую очередь функций (которые удаляют созданные обьекты)
struct DeletionQueue
{
    std::deque<std::function<void()>> deletors;

    void push_function(std::function<void()>&& function) {
        deletors.push_back(function);
    }

    void flush() {
        // reverse iterate the deletion queue to execute all the functions
        for (auto it = deletors.rbegin(); it != deletors.rend(); it++) {
            (*it)(); //call functors
        }

        deletors.clear();
    }
};

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

        // HardCoded data
        VkPipelineLayout _gridPipelineLayout;
        VkPipeline _gridPipeline;

        VkPipelineLayout _BasePipelineLayout;
        VkPipeline _BasePipeline;

        std::vector<std::string> modelsToLoad = {
            "../Model/pudge_dota_2.glb"
        };

        // Для теста нодов
        float angle{0.0f};

        TextureManager _textureManager;
        MeshManager _meshManager;
        ModelManager _modelManager{_init, _meshManager, _textureManager};
        RenderSystem _renderSystem{_init};
        // Храним нашу сцену в виде умного указателя
        std::unique_ptr<Scene> _activeScene;

        StaticModelConf _confStatic;
        DynamicModelConf _confDynamic;

        void renderLoop();
        void resize_swapchain();
        void destroy_swapchain();

        void init_descriptors();
        void init_grid_pipeline();
        void init_base_pipeline();
        void init_commands();
        void init_scene();

        void draw_grid(VkCommandBuffer cmd, VkDescriptorSet globalDescriptor);
        //void draw_model(VkCommandBuffer cmd, VkDescriptorSet globalDescriptor);
        void draw_imgui(VkCommandBuffer cmd);

        VkDescriptorSet update_scene_data(FrameData& currentFrame);
        void update_time();
        void update_imgui();
        void draw_fps_overlay();
        void made_move();
    };
}