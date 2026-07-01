#pragma once

#include "vk_types.h"
#include "vk_descriptors.h"
#include "vk_loader.h"

// Боже храни MAX_FRAMES_IN_FLIGHT
constexpr unsigned int FRAME_OVERLAP = 2;

// Короче мы будем передавать данные (16 чисел) в push_constant (Это в нашем шейдере),
// если точнее то в кеш видеокарты.
struct ComputePushConstants {
	glm::vec4 data1;
	glm::vec4 data2;
	glm::vec4 data3;
	glm::vec4 data4;
};

// Эфекты
struct ComputeEffect {
	const char* name;

	VkPipeline pipeline;
	VkPipelineLayout layout;

	ComputePushConstants data;
};

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

	// Наша новая система аллокации дескрипторов
	DescriptorAllocatorGrowable _frameDescriptors;
};

class VulkanEngine {
public:

	bool _isInitialized{ false };
	int _frameNumber {0};
	bool stop_rendering{ false };
	VkExtent2D _windowExtent{ 1920 , 1080 };

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

	// Возможность изменять размер окна
	bool resize_requested = false;

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
	AllocatedImage _depthImage;

	// Пизда пингвина которую мы юзали для swapChain, 
	// такой-же ебанный костыль что-бы передать нужное "разрешение"
	VkExtent2D _drawExtent;
	float renderScale = 1.0f;

	// Наша аллокатор, который хранит в себе пул дескрипторов
	DescriptorAllocatorGrowable globalDescriptorAllocator;

	// Set - реальные данные куда мы передаём, но кроме инициализации в него ещё наужно закинуть буферы с данными
	VkDescriptorSet _drawImageDescriptors;
	// setLayout - это бланк, шаблон по которому мы передаём данные в set который мы делаем аллокацией памяти
	VkDescriptorSetLayout _drawImageDescriptorLayout;

	// Ну типо дескрипторы для сцены
	GPUSceneData sceneData;
	VkDescriptorSetLayout _gpuSceneDataDescriptorLayout;

	// Инструкция описывающая интерфейс данных для градиентного шейдера 
	// (Сейчас это просто Layout для compute shaders, но мне лень переписывать названия в файлах)
	VkPipelineLayout _gradientPipelineLayout;

	// Второй графический конвеер (ТРЕУГОЛЬНИК) для растеризованой графики (Vetex, fragment shader) и его Layout
	VkPipelineLayout _trianglePipelineLayout;
	VkPipeline _trianglePipeline;

	// Графический конвеер
	VkPipelineLayout _meshPipelineLayout;
	VkPipeline _meshPipeline;
	// Данные для BDA
	GPUMeshBuffers rectangle;

	// Мы будет реализовывать staging buffer
	VkFence _immFence;
	VkCommandBuffer _immCommandBuffer;
	VkCommandPool _immCommandPool;

	// Типо что-бы с помощтю imguid менять цвета и приколдесы шейдера котоыре мы передаём через push_constant
	std::vector<ComputeEffect> backgroundEffects;
	int currentBackgroundEffect{ 0 };

	// Мы создаём просто командный буфер и в него записываем команду через функтор который мы передаём
	// Потом базовые подожди пока закончит
	// И в середине уже сам функтор который что либо делает с созданным командным буфером.
	// А потом его отправляет в очередь.
	// Если не душнить, то это просто удобный способ отправить команду видеокарте
	void immediate_submit(std::function<void(VkCommandBuffer cmd)>&& function);

	// Отрисовка интерфейса
	void draw_imgui(VkCommandBuffer cmd, VkImageView targetImageView);

	// Загрузка данных с DDR на GDDR
	GPUMeshBuffers uploadMesh(std::span<uint32_t> indices, std::span<Vertex> vertices);

	std::vector<std::shared_ptr<MeshAsset>> testMeshes;

	// Создание пустого изображения на GPU
	AllocatedImage create_image(VkExtent3D size, VkFormat format, VkImageUsageFlags usage, bool mipmapped = false);
	// Нарисовать на пустой алоцированном холсте картинку (копирование данных на пустую картинку)
	AllocatedImage create_image(void* data, VkExtent3D size, VkFormat format, VkImageUsageFlags usage, bool mipmapped = false);
	// Удаление изображение из GPU
	void destroy_image(const AllocatedImage& img);

// Temporary data about TEXTURES /////////////////////////////////////////////////////////////////////////////////////
	AllocatedImage _whiteImage;
	AllocatedImage _blackImage;
	AllocatedImage _greyImage;
	AllocatedImage _errorCheckerboardImage;

	VkSampler _defaultSamplerLinear;
	VkSampler _defaultSamplerNearest;

	VkDescriptorSetLayout _singleImageDescriptorLayout;

private:
	void init_vulkan();

	void init_descriptors();

	void init_swapchain();

	void init_commands();

	void init_sync_structures();

	// Инициации конвееров
	void init_pipelines();
	// Инициация ComputePipeline (которые на фоне)
	void init_background_pipelines();
	// Инициация GraphicPipeline для треугольника
	void init_triangle_pipeline();
	// Инициализия GraphicPipeline
	void init_mesh_pipeline();
	// Инициация Dear IMGUI
	void init_imgui();
	// Рисуем в GraphicPipeline
	void draw_geometry(VkCommandBuffer cmd);

	// Алокация и возврат буфера
	AllocatedBuffer create_buffer(size_t allocSize, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage);
	// Удаление буфера
	void destroy_buffer(const AllocatedBuffer& buffer);

	// Омерзительный костыль
	void init_default_data();

	void create_swapchain(uint32_t width, uint32_t height);

	void destroy_swapchain();

	void resize_swapchain();
};