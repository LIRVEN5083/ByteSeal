#pragma once

#include "vk_initializers.h"

namespace VK_GUI{
    void draw_fps_overlay(VK_INIT_ENGINE::_inited_engine& _init, CONTROLLER::Delta _delta);
    void draw_imgui(VK_INIT_ENGINE::_inited_engine& _init, VkCommandBuffer cmd, VkExtent2D _drawExtent);
    void update_imgui(VK_INIT_ENGINE::_inited_engine& _init, CONTROLLER::Delta _delta);
}