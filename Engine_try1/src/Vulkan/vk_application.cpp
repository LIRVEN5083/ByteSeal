#include "vk_application.h"

VK_APPLICATION::VulkanApplication::VulkanApplication(VK_INIT_ENGINE::_inited_engine& inited_engine)
: _init(inited_engine){}

void VK_APPLICATION::VulkanApplication::cleanup(){
    if (_init._isInitialized){
        vkDeviceWaitIdle(_init._device);
    }
}

void VK_APPLICATION::VulkanApplication::run(){
    SDL_Event e;
    bool bQuit = false;

    // Основной цикл вообще всего
    while (!bQuit) {
        // Основной цикл событий
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
        renderLoop();
    }
}

void VK_APPLICATION::VulkanApplication::renderLoop(){}
