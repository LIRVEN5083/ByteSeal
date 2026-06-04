#include "movement.hpp"

void mouse_callback(GLFWwindow* window, double xpos, double ypos) {
    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos;
    lastX = xpos;
    lastY = ypos;

    float sensitivity = 0.05f;
    xoffset *= sensitivity;
    yoffset *= sensitivity;

    yaw += xoffset;
    pitch += yoffset;

    if (pitch > 89.0f) pitch = 89.0f;
    if (pitch < -89.0f) pitch = -89.0f;
}

float getTime() {
    static auto start_Time = std::chrono::steady_clock::now();
    auto current_Time = std::chrono::steady_clock::now();

    std::chrono::duration<float> time = current_Time - start_Time;

    return time.count();
}

void IncreaseSpeed(float& speed) {
    speed += 0.1;
    if (times == 2) {
        std::cout << "current speed: " << speed << "\n";
        times = 0;
    }
    times++;
}

void DecreaseSpeed(float& speed) {
    if (speed > 0.2) {
        speed -= 0.1;
    }
    if (times == 2) {
        std::cout << "current speed: " << speed << "\n";
        times = 0;
    }
    times++;
}

void RegSpeed(GLFWwindow* window, int key, int scancode, int action, int mode) {

    if (action == GLFW_PRESS || action == GLFW_REPEAT) {
        if (key == GLFW_KEY_Q) { IncreaseSpeed(speed); }
        if (key == GLFW_KEY_E) { DecreaseSpeed(speed); }
    }
}