#pragma once 

#include "vk_initializers.h"

namespace vkutil {

	// Переключение режима для картинки
	void transition_image(VkCommandBuffer cmd, VkImage image, VkImageLayout currentLayout, VkImageLayout newLayout);

	// Ну даже не знаю что же это может быть. Точно не копирование с нашей бомже текстуры в swapChain
	void copy_image_to_image(VkCommandBuffer cmd, VkImage source, VkImage destination, VkExtent2D srcSize, VkExtent2D dstSize);
}