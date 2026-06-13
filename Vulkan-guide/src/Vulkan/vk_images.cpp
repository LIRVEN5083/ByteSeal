#include "vk_images.h"

void vkutil::transition_image(VkCommandBuffer cmd, VkImage image, VkImageLayout currentLayout, VkImageLayout newLayout){

    // Barrier - это как мьютекс в многопоточном программировании заставляет GPU делать всё пошагаво
    // ImageMemoryBarrier (или VkImageMemoryBarrier2 в Vulkan 1.3) — это конкретная записка-инструкция (пакет данных),
    // которая описывает, к какой именно картинке и как именно нужно применить барьер.
    VkImageMemoryBarrier2 imageBarrier{ .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2 };
    imageBarrier.pNext = nullptr;

    //[ Команды src ] -> | БАРЬЕР | -> [ Команды dst ].
    // src - source. Т.е команда ДО барьера
    // VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT - принудительно заблокировать все конвееры пока он не закончит работать с картинкой
    imageBarrier.srcStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT; // Ждать пока закончит работу
    imageBarrier.srcAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT; // Подготовка к загрузке новых данных
    // dst - destination. Т.е после барьера
    imageBarrier.dstStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT; // Ждать пока закончит работу
    imageBarrier.dstAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT | VK_ACCESS_2_MEMORY_READ_BIT; // Подготовка к загрузке новых данных а также возможность чтения прошлых

    // Имеет прямое отношение к src и dst
    // oldLayout и newLayout - это флаги
    imageBarrier.oldLayout = currentLayout;
    imageBarrier.newLayout = newLayout;

    // буквально написанно: если Layout типа глубинной картинки то Aspect выбирается под глубину а если иначе, то Aspect с чистыми цветами (RGBA)
    VkImageAspectFlags aspectMask = (newLayout == VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL) ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;
    // Оборачиваем aspectMask в структуру с дополнительными настройками
    imageBarrier.subresourceRange = vkinit::image_subresource_range(aspectMask);
    imageBarrier.image = image;

    // Структура которая собирает всё воединно
    VkDependencyInfo depInfo{};
    depInfo.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
    depInfo.pNext = nullptr;

    depInfo.imageMemoryBarrierCount = 1;
    depInfo.pImageMemoryBarriers = &imageBarrier;

    // Записываем команду синхронизации в коммандный буфер, передавая: Aspect, Работа нашего барьера, тип данных входных и выходных разметок памяти 
    vkCmdPipelineBarrier2(cmd, &depInfo);
}