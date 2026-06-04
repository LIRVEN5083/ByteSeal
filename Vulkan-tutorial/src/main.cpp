#include "InitClass.hpp"

GLFWwindow* window;
uint32_t WindowSizeX = 1920;
uint32_t WindowSizeY = 1080;

//Mouse Position
float lastX = float(WindowSizeX) / 2.0f;
float lastY = float(WindowSizeY) / 2.0f;
float yaw = -90.0f;
float pitch = 0.0f;

//Camera position
float valueZ = 5.0f;
float valueX = 0.0f;
float valueY = 1.5f;

//Delta time
float speed = 10.0f;

int times = 0;

const std::vector<Vertex> vertices = {
    {{-0.5f, -0.5f, 0.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
    {{0.5f, -0.5f, 0.0f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}},
    {{0.5f, 0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},
    {{-0.5f, 0.5f, 0.0f}, {1.0f, 1.0f, 1.0f}, {0.0f, 1.0f}},

    {{ 1.0f, -0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}}, 
    {{ 2.0f, -0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}}, 
    {{ 2.0f,  0.5f, -0.5f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}}, 
    {{ 1.0f,  0.5f, -0.5f}, {1.0f, 1.0f, 1.0f}, {0.0f, 1.0f}} 
};

const std::vector<uint16_t> indices = {
    0, 1, 2, 2, 3, 0,
    4, 5, 6, 6, 7, 4
};

int main() {
    HelloTriangleApplication app(window, WindowSizeX, WindowSizeY);

    try {
        app.run();
    }
    catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}