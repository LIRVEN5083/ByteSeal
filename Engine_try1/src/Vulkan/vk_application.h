#pragma once
#include "vk_descriptors.h"
#include "vk_images.h"
#include "vk_pipelines.h"
#include "vk_glTF_loading.h"

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
};

namespace VK_APPLICATION {

    class VulkanApplication{

        public:
        int _frameNumber {0};
        bool stop_rendering{ false };
        VulkanApplication(VK_INIT_ENGINE::_inited_engine& inited_engine);

        void cleanup();
        void run();
        GPUMeshBuffers upload_meshes(std::span<uint32_t> indices, std::span<Vertex> vertices);

        private:
        VK_INIT_ENGINE::_inited_engine& _init;
        FrameData _frames[FRAME_OVERLAP];

        bool resize_requested = false;
        VkExtent2D _drawExtent;
        float renderScale = 1.0f;

        GPUSceneData sceneData;
        VkDescriptorSetLayout _gpuSceneDataDescriptorLayout;
        DescriptorAllocatorGrowable globalDescriptorAllocator;

        CONTROLLER::Movement _movement;
        CONTROLLER::Camera _camera;
        CONTROLLER::Delta _delta;

        // HardCoded data
        VkPipelineLayout _gridPipelineLayout;
        VkPipeline _gridPipeline;

        VkPipelineLayout _BasePipelineLayout;
        VkPipeline _BasePipeline;

        std::vector<std::shared_ptr<MeshAsset>> testMeshes;

        void renderLoop();
        void resize_swapchain();
        void destroy_swapchain();

        void init_descriptors();
        void init_grid_pipeline();
        void init_base_pipeline();
        void init_commands();

        void draw_grid(VkCommandBuffer cmd, VkDescriptorSet globalDescriptor);
        void draw_meshes(VkCommandBuffer cmd, VkDescriptorSet globalDescriptor);

        VkDescriptorSet update_scene_data(FrameData& currentFrame);
        void made_move();
    };
}