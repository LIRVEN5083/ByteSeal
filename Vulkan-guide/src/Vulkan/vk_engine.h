#pragma once

#include "vk_types.h"
#include "vk_descriptors.h"

// Боже храни MAX_FRAMES_IN_FLIGHT
constexpr unsigned int FRAME_OVERLAP = 2;

// Обьясняю что нам пытается донести автор vkguide.dev
// Короче есть такой трабл что если фигачить картинку то ты не когда не узнаешь как она отобразиться у чувачка,
// поэтому мы херачим всё изображение на холст, а холст копируем в swapChain. Ну и типо у нас больше гибкости должно стать
// и это типо не должно ударить по оптимизации, потому-что апаратные средства видяхи позваляют щёлкать копирование текстур.
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

	// Метод выполнения кадра
	void draw();

	// Тут именно про отрисовку
	void draw_background(VkCommandBuffer cmd);

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

	// Очередь удаления
	DeletionQueue _mainDeletionQueue;

	// АллокатоР из биБЛИотеки Vulkan Memory Allocator. Выделять МНОГА паМяти GPU заранее, а потом мы просто запршиваем уже готовую паМять
	VmaAllocator _allocator;

	// Холст
	AllocatedImage _drawImage;

	// Пизда пингвина которую мы юзали для swapChain, 
	// такой-же ебанный костыль что-бы передать нужное "разрешение"
	VkExtent2D _drawExtent;

	// Наша аллокатор, который хранит в себе пул дескрипторов
	DescriptorAllocator globalDescriptorAllocator;

	// Set - реальные данные куда мы передаём, но кроме инициализации в него ещё наужно закинуть буферы с данными
	VkDescriptorSet _drawImageDescriptors;
	// setLayout - это бланк, шаблон по которому мы передаём данные в set который мы делаем аллокацией памяти
	VkDescriptorSetLayout _drawImageDescriptorLayout;

	// Скомпилированный бинарный код для градиентного шейдера
	VkPipeline _gradientPipeline;
	// Инструкция описывающая интерфейс данных для градиентного шейдера
	VkPipelineLayout _gradientPipelineLayout;

private:
	void init_vulkan();

	void init_descriptors();

	void init_swapchain();

	void init_commands();

	void init_sync_structures();

	void init_pipelines();
	void init_background_pipelines();

	void create_swapchain(uint32_t width, uint32_t height);

	void destroy_swapchain();
};