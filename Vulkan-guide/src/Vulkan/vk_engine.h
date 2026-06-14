#pragma once

#include "vk_types.h"

// Боже храни MAX_FRAMES_IN_FLIGHT
constexpr unsigned int FRAME_OVERLAP = 2;

// Пул и буфер на каждый кадр
struct FrameData {
	// Fence - это синхронизатор для CPU -> GPU. Мы не можем подвердить отправку нового commandBuffer и перезаписать старый, пока старый не выполнится
	VkFence _renderFence;

	// пул команд (аллокатор для командного буфера)
	// Так-же пул намертво привязан к очереди, то-есть команндные пулы для вычеслений и графики это должны быть разные сущности
	VkCommandPool _commandPool;
	VkCommandBuffer _mainCommandBuffer;
};

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



	// FrameData - обьявляенная структура выше
	FrameData _frames[FRAME_OVERLAP];

	// Это метод получение номера 0, 1, 0, 1 командного буфера. Я это ещё в презентации обьяснял. 
	FrameData& get_current_frame() { return _frames[_frameNumber % FRAME_OVERLAP]; };

	/*
	 VkQueue _graphicsQueue; - Это просто логическая очередь
	uint32_t _graphicsQueueFamily; - Это графическая семья выделенная в видео карте (как правило апаратные ядра для графической, математической и трансферной операций) 
	и это просто блоки видеокарты для разных вычеслений к которым идут логические очереди очереди"
	*/
	VkQueue _graphicsQueue;
	uint32_t _graphicsQueueFamily;

	// Semaphore - синхронизатор для GPU. Т.е разные операции GPU -> GPU с помощью Semaphore ждут друг друга
	std::vector<VkSemaphore> _swapchainSemaphores;
	std::vector<VkSemaphore> _renderSemaphores;

private:
	void init_vulkan();

	void init_swapchain();

	void init_commands();

	void init_sync_structures();

	void create_swapchain(uint32_t width, uint32_t height);

	void destroy_swapchain();
};