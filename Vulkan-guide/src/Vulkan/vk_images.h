#pragma once 

#include "vk_initializers.h"

namespace vkutil {
	// Переключение режима для картинки
	void transition_image(VkCommandBuffer cmd, VkImage image, VkImageLayout currentLayout, VkImageLayout newLayout);
}