#pragma once

#include "../Vulkan/vk_types.h"
#include "SDL3/SDL.h"
#include "SDL3/SDL_vulkan.h"
#include <chrono>
#include <iostream>

namespace CONTROLLER{
    struct Movement{
        float valueZ{2.0f};
        float valueX{0.0f};
        float valueY{0.0f};
        float speed{0.0f};
    };

    struct View{
        float yaw{0.0f};
        float pitch{0.0f};
        float sensetivity{0.0f};
    };

    struct Delta{
        uint64_t lastTime = SDL_GetTicksNS();
        uint64_t startTime{lastTime};
    };

    void IncreaseSpeed(float& speed);

    void DecreaseSpeed(float& speed);

}