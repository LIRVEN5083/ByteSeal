#pragma once
#include "vk_init_engine.h"
#include "vk_descriptors.h"
#include <vk_initializers.h>
#include <vk_images.h>
#include <vk_pipelines.h>

constexpr unsigned int FRAME_OVERLAP = 2;

struct AllocatedImage {
    VkImage image;
    VkImageView imageView;
    VmaAllocation allocation;
    VkExtent3D imageExtent;
    VkFormat imageFormat;
};

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

    // Fence - это синхронизатор для CPU -> GPU. Мы не можем подвердить отправку нового commandBuffer и перезаписать старый, пока старый не выполнится
    VkFence _renderFence;

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
            void renderLoop();
        private:
            VK_INIT_ENGINE::_inited_engine& _init;

    };
}