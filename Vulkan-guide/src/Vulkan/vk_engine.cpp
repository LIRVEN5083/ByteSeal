#include "vk_engine.h"

#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>

#include <vk_initializers.h>
#include <vk_types.h>

#include "VkBootstrap.h"

#include <chrono>
#include <thread>

// Включить/Отключить слои валидации
constexpr bool bUseValidationLayers = true;

// Единственный экземпляр класса нашего движка с единственной ссылкой на него
VulkanEngine* loadedEngine = nullptr;

// Получить наш экземпляр класса Engine
VulkanEngine& VulkanEngine::Get() { return *loadedEngine; }

void VulkanEngine::init()
{
    // Нужно короче что-бы движок был запущен в единтсвенном экземпляре
    assert(loadedEngine == nullptr);
    loadedEngine = this;

    // Инициализация SDL3, ну по факту так-же как и GLFW
    SDL_Init(SDL_INIT_VIDEO);

    // Просто в переменную записываем флаг, который указывает что SDL работает в связке с Vulkan
    SDL_WindowFlags window_flags = (SDL_WindowFlags)(SDL_WINDOW_VULKAN);

    // Заполнение структуры для создания окна
    _window = SDL_CreateWindow(
        "ByteSeal Engine",
        _windowExtent.width, _windowExtent.height,
        window_flags
    );

    // Вызов базовых методов, которые нужны для инициализации Vulkan
    init_vulkan();

    init_swapchain();

    init_commands();

    init_sync_structures();

    // Просто булевая переменная которая говорит запустился рендер чи нет
    _isInitialized = true;
}

// cleanup - стандартная очистка данных после завершений runtime программы
void VulkanEngine::cleanup()
{
    if (_isInitialized) {

        // Да, я даже тут буду как не самый умный человек писать комментарии
        // Уничтожение swapChain
        destroy_swapchain();
        
        // Уничтожение surface
        vkDestroySurfaceKHR(_instance, _surface, nullptr);

        // Уничтожение логического устройства (Logical Device)
        vkDestroyDevice(_device, nullptr);

        // Аннигилируем слои валидации
        vkb::destroy_debug_utils_messenger(_instance, _debug_messenger);

        // Уничтожение точки входа Vulkan
        vkDestroyInstance(_instance, nullptr);

        // Испипиление окошка SDL3
        SDL_DestroyWindow(_window);
    }

    // Затираем указатель на движок
    loadedEngine = nullptr;
}

void VulkanEngine::draw()
{
    // Ниче не надо
}

void VulkanEngine::run()
{
    SDL_Event e;
    bool bQuit = false;

    // Основной цикл событий и отрисовки
    while (!bQuit) {
        // Цикл ловли событий
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_EVENT_QUIT) {
                bQuit = true;
            }
            
            if (e.type == SDL_EVENT_WINDOW_MINIMIZED) {
                stop_rendering = true;
            }
            if (e.type == SDL_EVENT_WINDOW_RESTORED) {
                stop_rendering = false;
            }
        }

        // do not draw if we are minimized
        if (stop_rendering) {
            // throttle the speed to avoid the endless spinning
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            continue;
        }

        draw();
    }
}

void VulkanEngine::init_vulkan(){
    vkb::InstanceBuilder builder;

    // Длаем короче VkInstance - точку вкхода вулкана и сразу закидываем ему слои валидации
    // (Слава богу Vk-bootstrap нас щядит от инициализации на 1к строк)
    auto inst_ret = builder.set_app_name("ByteSeal_Instance")
        .request_validation_layers(bUseValidationLayers)
        .use_default_debug_messenger()
        .require_api_version(1, 3, 0)
        .build();

    vkb::Instance vkb_inst = inst_ret.value();

    // Передаём полю класса нашего экземпляра _instance (Ну т.е Instance который нужен Vulkan) 
    _instance = vkb_inst.instance;
    // Передаём полю класса нашего экземпляра __debug_messenger (Настоящей твари из Vulkan которую задрочишся настраивать с нуля)
    _debug_messenger = vkb_inst.debug_messenger;

    // Тут вообще Surface как нехер делать. Просто передаёшь настоящий Instance, окошко SDL3, Аллокатор(Мы просто пихнём nullptr) и О БОЖЕ ПО ССЫЛКЕ передадим _surface куда запишуться данные
    SDL_Vulkan_CreateSurface(_window, _instance, nullptr, &_surface);

    // vulkan 1.3 приколы
    VkPhysicalDeviceVulkan13Features features{ .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES };
    features.dynamicRendering = true;
    features.synchronization2 = true;

    // vulkan 1.2 приколы
    VkPhysicalDeviceVulkan12Features features12{ .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES };
    features12.bufferDeviceAddress = true;
    features12.descriptorIndexing = true;

    // boot-strap (благослави его создателя), позволяет без херни получить физические данные нашей видеокарты (VkPhysicalDevice). 
    // Нам короче нужна видюха толька такая которая поддерживает SDL3, а также фичи Vulkan 1.2, 1.3
    vkb::PhysicalDeviceSelector selector{ vkb_inst };
    vkb::PhysicalDevice physicalDevice = selector
        .set_minimum_version(1, 3)
        .set_required_features_13(features)
        .set_required_features_12(features12)
        .set_surface(_surface)
        .select()
        .value();

    //Ну типо просто конфигурируем заполненую структуры из VKboot-strap
    vkb::DeviceBuilder deviceBuilder{ physicalDevice };
    vkb::Device vkbDevice = deviceBuilder.build().value();

    // И переменную из Vkboot-strap тупа присваиваем реальной переменной Vulkan. Чудо аллаха не иначе
    _device = vkbDevice.device;
    _chosenGPU = physicalDevice.physical_device;
}

void VulkanEngine::init_swapchain(){
    create_swapchain(_windowExtent.width, _windowExtent.height);
}

void VulkanEngine::init_commands(){

}

void VulkanEngine::init_sync_structures(){

}

void VulkanEngine::create_swapchain(uint32_t width, uint32_t height){
    // Создаём структуру для создания swapChain (VkPhysicalDevice, VkLogicalDevice, VkSurface)
    vkb::SwapchainBuilder swapchainBuilder{ _chosenGPU,_device,_surface };

    // Это тупа формат свап чейна, лол
    _swapchainImageFormat = VK_FORMAT_B8G8R8A8_UNORM;

    vkb::Swapchain vkbSwapchain = swapchainBuilder
        // .use_default_format_selection() - типо хер его, просто заполнение структуры на формат SwapChain
        .set_desired_format(VkSurfaceFormatKHR{ .format = _swapchainImageFormat, .colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR })
        // use vsync present mode - флажок FIFO_KHR - означает у нас будет крайне жёсткая верт.синхронизация 
        .set_desired_present_mode(VK_PRESENT_MODE_FIFO_KHR)
        // Это размер буфера. Ну тип можно поставить FULL HD но оно же тип будет образаться и растягиваться от размера окна
        .set_desired_extent(width, height)
        // DST_BIT - картинка может быть приемником для копирования. SRC_BIT - картинка может быть источником для копирования
        .add_image_usage_flags(VK_IMAGE_USAGE_TRANSFER_DST_BIT)
        .build()
        .value();

    // Реальному полю вулкан мы присваеваем херню которую писали выше .set_desired_extent(width, height)
    _swapchainExtent = vkbSwapchain.extent;
    // Реальное поле со свап чейн теперь есть и заполнено темой сверху
    _swapchain = vkbSwapchain.swapchain;
    // Просто инициация массива с картинками 
    _swapchainImages = vkbSwapchain.get_images().value();
    // Просто инициация массива обёрток на массив с картинками 
    _swapchainImageViews = vkbSwapchain.get_image_views().value();
}

void VulkanEngine::destroy_swapchain(){
    // Ес чо это удаление swapChainKHR так ещё и он автоматом удаляет swapChainImage, а вот swapChainImageViews уже будь добр удаляй сам
    vkDestroySwapchainKHR(_device, _swapchain, nullptr);

    // Ну вот о чём и говорил, тут онли самому удалять
    for (int i = 0; i < _swapchainImageViews.size(); i++) {
        vkDestroyImageView(_device, _swapchainImageViews[i], nullptr);
    }
}
