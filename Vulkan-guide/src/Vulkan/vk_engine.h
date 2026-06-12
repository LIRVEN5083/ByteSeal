#pragma once

#include "vk_types.h"

class VulkanEngine {
public:

	bool _isInitialized{ false };
	int _frameNumber {0};
	bool stop_rendering{ false };
	VkExtent2D _windowExtent{ 1700 , 900 };

	struct SDL_Window* _window{ nullptr };

	static VulkanEngine& Get();

	//initializes everything in the engine
	void init();

	//shuts down the engine
	void cleanup();

	//draw loop
	void draw();

	//run main loop
	void run();

	// ИНИЦИАЦИЯ Vulkan и его важных составляющих
	VkInstance _instance;// Точка запуска Vulkan
	VkDebugUtilsMessengerEXT _debug_messenger;// Слои валидации
	VkPhysicalDevice _chosenGPU;// Данные о физ карте
	VkDevice _device; // Логическая видеокарта к которой мы будем присобачивать всякое дерьмо
	VkSurfaceKHR _surface;// Хрень которая связывает наше окошко SDL3 и рендеринг Vulkan



	// swapChain - Буфер кадра
	VkSwapchainKHR _swapchain; // Физическое обьявление SwapChain
	VkFormat _swapchainImageFormat; // Формат/Инструкция как работает SwapChain

	std::vector<VkImage> _swapchainImages; // Сырые изображения (просто байтовые комбинации) к примеру: 0, 1, 2 (Тройная буферизация)
	std::vector<VkImageView> _swapchainImageViews; // Инструкция к каждому кадру (Сырой картинки из swapChainImages)
	VkExtent2D _swapchainExtent; // И хуета которая хранит размер swapChain. К примеру хочу 4к или FullHD в картинку 500 на 500 пикселей.

private:
	void init_vulkan();

	void init_swapchain();

	void init_commands();

	void init_sync_structures();

	void create_swapchain(uint32_t width, uint32_t height);

	void destroy_swapchain();
};