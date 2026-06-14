#include "vk_engine.h"

#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>

#define VMA_IMPLEMENTATION
#include "vk_mem_alloc.h"

#include <vk_initializers.h>
#include <vk_types.h>
#include <vk_images.h>

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

        vkDeviceWaitIdle(_device);

        for (int i = 0; i < FRAME_OVERLAP; i++) {

            //already written from before
            vkDestroyCommandPool(_device, _frames[i]._commandPool, nullptr);

            //destroy sync objects
            vkDestroyFence(_device, _frames[i]._renderFence, nullptr);
        }

        // Умный очистка семафор
        for (uint32_t i = 0; i < _swapchainImages.size(); i++) {
            vkDestroySemaphore(_device, _swapchainSemaphores[i], nullptr);
            vkDestroySemaphore(_device, _renderSemaphores[i], nullptr);
        }

        //flush the global deletion queue
        _mainDeletionQueue.flush();

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
    const uint32_t SemaphoreId = _frameNumber % _swapchainSemaphores.size();
    
    // Ждём когда GPU прекратит рендерить прошлую картинку в течении 1 сек.
    VK_CHECK(vkWaitForFences(_device, 1, &get_current_frame()._renderFence, true, 1000000000));

    get_current_frame()._deletionQueue.flush();

    // Возвращаем все Fences в исходное состояние
    VK_CHECK(vkResetFences(_device, 1, &get_current_frame()._renderFence));


    // О май гад это же ImageIndex из Vk-tutorial
    uint32_t swapchainImageIndex;
    // Запрашиваем картинку из 
    VK_CHECK(vkAcquireNextImageKHR(_device, _swapchain, 1000000000, _swapchainSemaphores[SemaphoreId], nullptr, &swapchainImageIndex));

    // Просто запишешь это в красивую структуру. Это временная запись commandBuffer 
    VkCommandBuffer cmd = get_current_frame()._mainCommandBuffer;

    // Мы теперь можем очищать командный буфер, чтобы опять его перезаписать
    VK_CHECK(vkResetCommandBuffer(cmd, 0));

    // Передаём данные о структуре записи
    VkCommandBufferBeginInfo cmdBeginInfo = vkinit::command_buffer_begin_info(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

    // Разрешение нашей картинки с которой мы будем работать
    _drawExtent.width = _drawImage.imageExtent.width;
    _drawExtent.height = _drawImage.imageExtent.height;

    // Открываем на запись. Теперь в наш удобненький cmd можно слать команды
    VK_CHECK(vkBeginCommandBuffer(cmd, &cmdBeginInfo));

    // Переключаем холст на запись
    vkutil::transition_image(cmd, _drawImage.image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);

    // Рисуем в холст
    draw_background(cmd);

    // Моя в сотый РАЗ любимаЯ настройка типов данных для различных операций с нмим
    // Мы переводим ебанный холст swapChain и холст "обыкновенный" в режимы: получателя, отправителя
    // Потому-что блядское апаратное копирование GPU очень нежное и ему нужно указать всё до последней детали 
    vkutil::transition_image(cmd, _drawImage.image, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
    vkutil::transition_image(cmd, _swapchainImages[swapchainImageIndex], VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

    // УРАА блядское копирование, можно теперь танцевать и радоваться
    vkutil::copy_image_to_image(cmd, _drawImage.image, _swapchainImages[swapchainImageIndex], _drawExtent, _swapchainExtent);

    // make the swapchain image into presentable mode.
    // «Presentable mode» (режим показа на экране) — это состояние,
    // в котором картинка физически готова к тому, чтобы операционная система вывела её на наш монитор.

    vkutil::transition_image(cmd, _swapchainImages[swapchainImageIndex], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);


    // Закрываем запись в коммандный буффер
    VK_CHECK(vkEndCommandBuffer(cmd));

    // Структура для отправки командного буфера
    VkCommandBufferSubmitInfo cmdinfo = vkinit::command_buffer_submit_info(cmd);

    // Структура для ожидающего семафора
    VkSemaphoreSubmitInfo waitInfo = vkinit::semaphore_submit_info(VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT_KHR, _swapchainSemaphores[SemaphoreId]);

    // Структура для отправляющего сигнал семафора
    VkSemaphoreSubmitInfo signalInfo = vkinit::semaphore_submit_info(VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT, _renderSemaphores[swapchainImageIndex]);

    // Структура с прошлимы заполнеными структурами
    VkSubmitInfo2 submit = vkinit::submit_info(&cmdinfo, &signalInfo, &waitInfo);

    // И теперь отправляем все наши упакованные данные
    VK_CHECK(vkQueueSubmit2(_graphicsQueue, 1, &submit, get_current_frame()._renderFence));

    // Заполняем инфу о структуре ДЛЯ ОТПРАВКИ ИЗОБРАЖЕНИЯ! (Которое уже на нашей видеокарте).
    // Мы буквально операционной системе говрим вывести готовое изображение.
    VkPresentInfoKHR presentInfo = {};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.pNext = nullptr;
    // Ес чо то можно в систему вывести даже другие SwapChains. К примеру на 3 экрана сразу.
    presentInfo.pSwapchains = &_swapchain;
    presentInfo.swapchainCount = 1;

    presentInfo.pWaitSemaphores = &_renderSemaphores[swapchainImageIndex];
    presentInfo.waitSemaphoreCount = 1;

    presentInfo.pImageIndices = &swapchainImageIndex;

    VK_CHECK(vkQueuePresentKHR(_graphicsQueue, &presentInfo));

    // Это сранный счётчик для чередование на четное/нечетное ну типо вот эта херня: 0, 1, 0, 1
    _frameNumber++;
}

void VulkanEngine::draw_background(VkCommandBuffer cmd)
{
    // Делаем чистый цвет. Который будет мерцать: 120 frame period.
    VkClearColorValue clearValue;
    float flash = std::abs(std::sin(_frameNumber / 120.f));
    clearValue = { { 0.0f, 0.0f, flash, 1.0f } };

    // Зона покрытием окраски. VK_IMAGE_ASPECT_COLOR_BIT - говорит что красим ВСЁ. ЕСЛИ ЧТО этот флаг это Aspect
    VkImageSubresourceRange clearRange = vkinit::image_subresource_range(VK_IMAGE_ASPECT_COLOR_BIT);

    // Записываем в коммандный буфер данные о покраске
    vkCmdClearColorImage(cmd, _drawImage.image, VK_IMAGE_LAYOUT_GENERAL, &clearValue, 1, &clearRange);
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

    // И переменную из VkBoot-strap тупа присваиваем реальной переменной Vulkan. Чудо аллаха не иначе
    _device = vkbDevice.device;
    _chosenGPU = physicalDevice.physical_device;

    // Слава святому VkBoot-strap, мы можем на уже созданую логическую GPU просто получить значения ДЛЯ ГРАФИЧЕСКОЙ ОЧЕРЕДИ!
    _graphicsQueue = vkbDevice.get_queue(vkb::QueueType::graphics).value();
    // А так же слава богу легко получить значение для ГРАФИЧЕСКОЙ СЕМЬИ!
    _graphicsQueueFamily = vkbDevice.get_queue_index(vkb::QueueType::graphics).value();

    // ИНИЦИАЛИЗАЦИЯ мулти тул АллоКАТАРА который сразу запросит куча памяти у GPU
    VmaAllocatorCreateInfo allocatorInfo = {};
    allocatorInfo.physicalDevice = _chosenGPU;
    allocatorInfo.device = _device;
    allocatorInfo.instance = _instance;
    // Ну типо сказанно что этот флаг может нам позволить получать адресса GPU
    allocatorInfo.flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;
    vmaCreateAllocator(&allocatorInfo, &_allocator);

    _mainDeletionQueue.push_function([&]() {
        vmaDestroyAllocator(_allocator);
        });
}

void VulkanEngine::init_swapchain(){
    create_swapchain(_windowExtent.width, _windowExtent.height);
    //draw image size will match the window
    VkExtent3D drawImageExtent = {
        _windowExtent.width,
        _windowExtent.height,
        1
    };

    //hardcoding the draw format to 32 bit float
    _drawImage.imageFormat = VK_FORMAT_R16G16B16A16_SFLOAT;
    _drawImage.imageExtent = drawImageExtent;

    VkImageUsageFlags drawImageUsages{};
    drawImageUsages |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    drawImageUsages |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    drawImageUsages |= VK_IMAGE_USAGE_STORAGE_BIT;
    drawImageUsages |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    VkImageCreateInfo rimg_info = vkinit::image_create_info(_drawImage.imageFormat, drawImageUsages, drawImageExtent);

    //for the draw image, we want to allocate it from gpu local memory
    VmaAllocationCreateInfo rimg_allocinfo = {};
    rimg_allocinfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
    rimg_allocinfo.requiredFlags = VkMemoryPropertyFlags(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    //allocate and create the image
    vmaCreateImage(_allocator, &rimg_info, &rimg_allocinfo, &_drawImage.image, &_drawImage.allocation, nullptr);

    //build a image-view for the draw image to use for rendering
    VkImageViewCreateInfo rview_info = vkinit::imageview_create_info(_drawImage.imageFormat, _drawImage.image, VK_IMAGE_ASPECT_COLOR_BIT);

    VK_CHECK(vkCreateImageView(_device, &rview_info, nullptr, &_drawImage.imageView));

    //add to deletion queues
    _mainDeletionQueue.push_function([=]() {
        vkDestroyImageView(_device, _drawImage.imageView, nullptr);
        vmaDestroyImage(_allocator, _drawImage.image, _drawImage.allocation);
        });
}

void VulkanEngine::init_commands(){
    // Создаём commandPool
    /* Смотри сюда. Тут как сделано в Vk - tutorial нам говорят 1 командный пул на много командных буферов
    *  Но если делать vkResetCommandPool то это достаточно медленная операция по поиску адресов в GPU и их удаления
    *  А вот автор Vk - guide нам говорит: Делаем на каждый буфер по 1 пулу. И теперь мы можем очищая пул СРАЗУ
    *  уничтожать все ему присущие командные буферы при этом оставляя по очерёдный командный буфер
    */
    /* Старый код который мы красиво снизу сократим
    VkCommandPoolCreateInfo commandPoolInfo = {};
    commandPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    commandPoolInfo.pNext = nullptr;
    commandPoolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    commandPoolInfo.queueFamilyIndex = _graphicsQueueFamily;
    */

    // Красивая реализация из файла vk_initializers.h
    VkCommandPoolCreateInfo commandPoolInfo = vkinit::command_pool_create_info(_graphicsQueueFamily, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);

    // А вот тут прикол короче. На количество графических буферов мы делаем такое-же количество командных буфер, 
    // а так как 1 command buffer = 1 command pool
    // То делаем каждому command buffer его собственный command pool
    for (int i = 0; i < FRAME_OVERLAP; i++) {

        VK_CHECK(vkCreateCommandPool(_device, &commandPoolInfo, nullptr, &_frames[i]._commandPool));

        /* Старый код
        // allocate the default command buffer that we will use for rendering
        VkCommandBufferAllocateInfo cmdAllocInfo = {};
        cmdAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        // pNext - это современный костыль, который нужен чтобы
        // присобачить новую фичу не переписывая весь код
        cmdAllocInfo.pNext = nullptr;
        cmdAllocInfo.commandPool = _frames[i]._commandPool;
        cmdAllocInfo.commandBufferCount = 1;
        cmdAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        */

        // Ну очень эллегантная реализация в ввиде функции из файла vk_initializers.h
        VkCommandBufferAllocateInfo cmdAllocInfo = vkinit::command_buffer_allocate_info(_frames[i]._commandPool, 1);

        VK_CHECK(vkAllocateCommandBuffers(_device, &cmdAllocInfo, &_frames[i]._mainCommandBuffer));
    }
}

void VulkanEngine::init_sync_structures(){
    //create syncronization structures
    //one fence to control when the gpu has finished rendering the frame,
    //and 2 semaphores to syncronize rendering with swapchain
    //we want the fence to start signalled so we can wait on it on the first frame
    VkFenceCreateInfo fenceCreateInfo = vkinit::fence_create_info(VK_FENCE_CREATE_SIGNALED_BIT);
    // Флага для Semaphore не существует так-что просто 0
    VkSemaphoreCreateInfo semaphoreCreateInfo = vkinit::semaphore_create_info(0);

    for (int i = 0; i < FRAME_OVERLAP; i++) {
        // Нам нужен лишь один fence потому-что всего два коммандных буфера 0, 1, 0, 1 
        VK_CHECK(vkCreateFence(_device, &fenceCreateInfo, nullptr, &_frames[i]._renderFence));

        /*
                [ CPU ] --- (vkQueueSubmit) ---> Отправляет команды на GPU
                                            |
                                            v
        [ Этап А ] ---(vkAcquireNextImage)---> [ _swapchainSemaphore ] (Зеленый свет: картинка свободна!)
                                            |
                                            v
        [ Этап Б ] ======= РЕНДЕРИНГ (Очередь GPU рисует кадр) =======
                                            |
                                            v
        [ Этап В ] <---(vkQueuePresent)------- [ _renderSemaphore ] (Зеленый свет: всё нарисовано!)
        */
    }

    // Меняем короче semaphore привязанные к каждому кадру на обычный вектор
    uint32_t swapchainImageCount = static_cast<uint32_t>(_swapchainImages.size());
    _swapchainSemaphores.resize(swapchainImageCount);
    _renderSemaphores.resize(swapchainImageCount);

    for (uint32_t i = 0; i < swapchainImageCount; i++) {
        VK_CHECK(vkCreateSemaphore(_device, &semaphoreCreateInfo, nullptr, &_swapchainSemaphores[i]));
        VK_CHECK(vkCreateSemaphore(_device, &semaphoreCreateInfo, nullptr, &_renderSemaphores[i]));
    }
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
