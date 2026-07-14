#pragma once

#include "../Vulkan/vk_types.h"
#include "SDL3/SDL.h"
#include "SDL3/SDL_vulkan.h"

namespace CONTROLLER{
    struct Movement{
        float valueZ{2.0f};
        float valueX{0.0f};
        float valueY{0.0f};
        float speed{5.0f};
    };

    struct View{
        float yaw{0.0f};
        float pitch{0.0f};
        float sensetivity{0.0f};
        glm::vec3 front{0.0f};
        glm::vec3 Wfront{0.0f};
        glm::vec3 right{0.0f};
    };

    struct Delta{
        std::chrono::high_resolution_clock::time_point lastFrameTime;
        std::chrono::high_resolution_clock::time_point startTime;
        float moveStep;
    };

    void IncreaseSpeed(float& speed);

    void DecreaseSpeed(float& speed);

}