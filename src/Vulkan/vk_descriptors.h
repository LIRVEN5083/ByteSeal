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

// DescriptorAllocatorPool (Теперь уже DescriptorAllocatorGrawable - хранит не один пул, а сразу много)
// poolRatios — массив коэффициентов, который говорит, сколько дескрипторов каждого типа нам нужно
// PoolSizeRatio — это структура (обычно содержащая VkDescriptorType type и float ratio).
// Есть UBO - для передачи глобальных настроек,
// а есть SSBO - для передачи огромных массивов данных
/*
 DescriptorSet - это наша память шейдеов (для которых мы пишем set, binding, uniform)
 и для них нужна память и мы её выделяем с помощью структуры DescriptorAllocatorGrowable,
 но тут мы уже в одном экземпляре можем совершать любое количество аллокаций"
*/
struct DescriptorAllocatorGrowable {
public:
    struct PoolSizeRatio {
        VkDescriptorType type;
        float ratio;
    };

    // Функция init нужна чисто для создания первого readPool чтобы шайтан машина заработала
    void init(VkDevice device, uint32_t initialSets, std::span<PoolSizeRatio> poolRatios);
    // Очистка аллокаций
    void clear_pools(VkDevice device);
    // Уничтожение наборов аллокаторов
    void destroy_pools(VkDevice device);

    // Создание VkDescriptorSet
    VkDescriptorSet allocate(VkDevice device, VkDescriptorSetLayout layout, void* pNext = nullptr);
private:
    // Нужна для передачи пула, если пулов нету или они забиты то мы создаём новый пул через create_pool
    VkDescriptorPool get_pool(VkDevice device);
    // Создание пула
    VkDescriptorPool create_pool(VkDevice device, uint32_t setCount, std::span<PoolSizeRatio> poolRatios);

    // Нужны для определения аллокации VkDescriptorPool
    std::vector<PoolSizeRatio> ratios;
    // Заполненные пулы.
    // Места в них больше нет. Сюда их откладывают,
    // чтобы не опрашивать зря, а в конце кадра/сцены очистить все разом.
    std::vector<VkDescriptorPool> fullPools;
    // Готовые пулы. Пустые пулы
    // Мы их берём и с помощью них записываем новые DescriptorSet
    std::vector<VkDescriptorPool> readyPools;
    // Ограничение на количество созданных VkDescriptorSet для VkDescriptorPool
    uint32_t setsPerPool;
};

/*
typedef struct VkWriteDescriptorSet {
    VkStructureType                  sType;
    const void*                      pNext;
    VkDescriptorSet                  dstSet;
    uint32_t                         dstBinding;
    uint32_t                         dstArrayElement;
    uint32_t                         descriptorCount;
    VkDescriptorType                 descriptorType;
    const VkDescriptorImageInfo*     pImageInfo;        <----- Сырой кусок памяти, можно всегда юзать но не эффективно
    const VkDescriptorBufferInfo*    pBufferInfo;       <----- Кусок памяти для вычислительных шейдеров и текстур
    const VkBufferView*              pTexelBufferView;
} VkWriteDescriptorSet;
*/

struct DescriptorWriter {
    std::deque<VkDescriptorImageInfo> imageInfos;
    std::deque<VkDescriptorBufferInfo> bufferInfos;
    std::vector<VkWriteDescriptorSet> writes;

    // write - служит командой записи
    // И короче на каждый экземпляр Image, buffer мы делаем write,
    // но нам нужно хранить и imageInfos, bufferInfos, так как write содержит просто указатель,
    // короче нам нужны физ наличие этих данных
    // Создание pImageInfo
    void write_image(int binding,VkImageView image,VkSampler sampler , VkImageLayout layout, VkDescriptorType type);
    // Создание pBufferInfo
    void write_buffer(int binding,VkBuffer buffer,size_t size, size_t offset,VkDescriptorType type);

    void clear();
    // VkUpdateDescriptorSet - отправка write
    void update_set(VkDevice device, VkDescriptorSet set);
};