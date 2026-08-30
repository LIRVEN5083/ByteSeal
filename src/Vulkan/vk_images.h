#pragma once 

#include "vk_types.h"

namespace vkinit{
	VkImageSubresourceRange image_subresource_range(VkImageAspectFlags aspectMask);
}

namespace vkutil {

	// Переключение режима для картинки
	void transition_image(VkCommandBuffer cmd, VkImage image, VkImageLayout currentLayout, VkImageLayout newLayout);

	// Ну даже не знаю что же это может быть. Точно не копирование с нашей бомже текстуры в swapChain
	void copy_image_to_image(VkCommandBuffer cmd, VkImage source, VkImage destination, VkExtent2D srcSize, VkExtent2D dstSize);

	// Samplers info
	VkFilter GetVkFilter(int gltfFilter);

	VkSamplerMipmapMode GetVkMipmapMode(int gltfFilter);

	VkSamplerAddressMode GetVkAddressMode(int gltfWrap);

	// Перегруженные операторы что-бы не создавать каждый раз новые семплеры
	struct SamplerCreateInfoHash {
		std::size_t operator()(const VkSamplerCreateInfo& k) const {
			std::size_t h = 0;
			auto hash_combine = [&h](std::size_t v) { h ^= v + 0x9e3779b9 + (h << 6) + (h >> 2); };
			hash_combine(std::hash<int>()(k.magFilter));
			hash_combine(std::hash<int>()(k.minFilter));
			hash_combine(std::hash<int>()(k.mipmapMode));
			hash_combine(std::hash<int>()(k.addressModeU));
			hash_combine(std::hash<int>()(k.addressModeV));
			return h;
		}
	};

	struct SamplerCreateInfoEqual {
		bool operator()(const VkSamplerCreateInfo& a, const VkSamplerCreateInfo& b) const {
			return a.magFilter == b.magFilter &&
				   a.minFilter == b.minFilter &&
				   a.mipmapMode == b.mipmapMode &&
				   a.addressModeU == b.addressModeU &&
				   a.addressModeV == b.addressModeV;
		}
	};
}