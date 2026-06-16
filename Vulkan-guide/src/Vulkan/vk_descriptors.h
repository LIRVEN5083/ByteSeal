#pragma once

#include "vk_images.h"

/*
 [1. Биндинги шейдера]
        │
        ▼
┌─────────────────────────────┐
│  VkDescriptorSetLayout      │ ◄─── Создается на основе биндингов
└──────────┬──────────────┬───┘
           │              │
    Используется для      Передается при аллокации
    описания функции      памяти под данные
           │              │
           ▼              ▼
┌─────────────────────┐  ┌─────────────────────┐
│  VkPipelineLayout   │  │   VkDescriptorSet   │ ◄─── Сюда привязываются
└──────────┬──────────┘  └──────────┬──────────┘      реальные буферы/картинки
           │                        │
    Передается при                  Передается во время
    создании пайплайна              записи команд
           │                        │
           ▼                        ▼
┌─────────────────────┐             │
│     VkPipeline      │             │
└──────────┬──────────┘             │
           │                        │
           └───────────┬────────────┘
                       ▼
             [ vkCmdBindPipeline ]
             [ vkCmdBindDescriptorSets ]
 */

/*  1. VkDescriptorSetLayout: Это «чертеж» или сигнатура.
       Вы описываете для Vulkan: «В моем compute-шейдере на binding 0 будет буфер, а на binding 1 — текстура».
    2. VkPipelineLayout: Вы берете этот чертеж (или несколько) и связываете его с пайплайном.
       Без этого шага вы не сможете создать VkPipeline для compute-шейдера. 
       Драйвер должен заранее знать структуру памяти.
    3. VkDescriptorSet: Это уже сам заполненный контейнер, который указывает на конкретные куски памяти на видеокарте (конкретный VkBuffer или VkImageView).
       Он аллоцируется из пула (VkDescriptorPool) строго по чертежу из шага 1.
*/

// Простенькая структура что-бы создавать VkDescriptorSetLayout
struct DescriptorLayoutBuilder {

    std::vector<VkDescriptorSetLayoutBinding> bindings;

    // Создание binding и помещение его в массив значений bindings
    void add_binding(uint32_t binding, VkDescriptorType type);
    // Очистка массива bindings
    void clear();
    // Создание структуры VkDescriptorSet
    VkDescriptorSetLayout build(VkDevice device, VkShaderStageFlags shaderStages, void* pNext = nullptr, VkDescriptorSetLayoutCreateFlags flags = 0);
};

struct DescriptorAllocator {

    struct PoolSizeRatio {
        VkDescriptorType type;
        float ratio;
    };

    // Наш пул аллокаторов
    VkDescriptorPool pool;

    // Создание пула аллокаторов
    void init_pool(VkDevice device, uint32_t maxSets, std::span<PoolSizeRatio> poolRatios);

    // Быстрый сброс всей памяти. Сразу аннулируем все дескрипторы
    void clear_descriptors(VkDevice device);

    // Уничтожаем пул аллокаторов
    void destroy_pool(VkDevice device);

    // Берём память из pool и смотрим на наш layout в него и оборачиваем Выделенный дексриптор
    VkDescriptorSet allocate(VkDevice device, VkDescriptorSetLayout layout);
};