#include "InitClass.hpp"

GLFWwindow* window;
uint32_t WindowSizeX = 500;
uint32_t WindowSizeY = 500;

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