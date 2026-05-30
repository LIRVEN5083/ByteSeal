#ifndef _INIT_CLASS_HPP
#define _INIT_CLASS_HPP

#define GLFW_INCLUDE_VULKAN
#define VK_USE_PLATFORM_WIN32_KHR
#define GLFW_EXPOSE_NATIVE_WIN32
#define GLM_FORCE_RADIANS

#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <cstdlib>
#include <cstring>
#include <cstdint>
#include <limits>
#include <chrono>
#include <stdexcept>
#include <algorithm>

#include <array>
#include <optional>
#include <vector>
#include <set>

#include "GLSL_files/load_GLSL.hpp"

// In fact this is count of Bufferization
const int MAX_FRAMES_IN_FLIGHT = 2;

const std::vector<const char*> validationLayers = {
    "VK_LAYER_KHRONOS_validation"
};

//#define NDEBUG
#ifdef NDEBUG
constexpr bool enableValidationLayers = false;
#else
constexpr bool enableValidationLayers = true;
#endif

const std::vector<const char*> deviceExtensions = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

//Binding - магистраль а у магистрали есть свои дорожки (location)
struct Vertex {
    glm::vec2 pos;
    glm::vec3 color;

    // Создание и заполнение привязки
    static VkVertexInputBindingDescription getBindingDescription() {
        VkVertexInputBindingDescription bindingDescription{};
        bindingDescription.binding = 0;
        bindingDescription.stride = sizeof(Vertex);
        bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
        return bindingDescription;
    }

    //Параметры шейдеров
    static std::array<VkVertexInputAttributeDescription, 2> getAttributeDescriptions() {
        std::array<VkVertexInputAttributeDescription, 2> attributeDescriptions{};

        attributeDescriptions[0].binding = 0;
        attributeDescriptions[0].location = 0;
        attributeDescriptions[0].format = VK_FORMAT_R32G32_SFLOAT;
        attributeDescriptions[0].offset = offsetof(Vertex, pos);

        attributeDescriptions[1].binding = 0;
        attributeDescriptions[1].location = 1;
        attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[1].offset = offsetof(Vertex, color);

        return attributeDescriptions;
    }
};

extern const std::vector<Vertex> vertices;
extern const std::vector<uint16_t> indices;

struct UniformBufferObject {
    glm::mat4 model;
    glm::mat4 view;
    glm::mat4 proj;
};

struct QueueFamilyIndices {
    std::optional<uint32_t> graphicsFamily;
    std::optional<uint32_t> presentFamily;

    bool isComplete() {
        return graphicsFamily.has_value() && presentFamily.has_value();
    }
};

struct SwapChainSupportDetails {
    VkSurfaceCapabilitiesKHR capabilities; // Границы экрана
    std::vector<VkSurfaceFormatKHR> formats; // Формат цвета
    std::vector<VkPresentModeKHR> presentModes; // Режим вывода
};

class HelloTriangleApplication {
public:
    void run();
    HelloTriangleApplication(GLFWwindow* window, uint32_t x, uint32_t y);
private:
    GLFWwindow* window;
    uint32_t x;
    uint32_t y;
    VkInstance instance = VK_NULL_HANDLE;
    VkDebugUtilsMessengerEXT debugMessenger = VK_NULL_HANDLE;
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    VkDevice device = VK_NULL_HANDLE;
    VkQueue graphicsQueue = VK_NULL_HANDLE;
    VkQueue presentQueue = VK_NULL_HANDLE;
    VkSurfaceKHR surface = VK_NULL_HANDLE;

    VkSwapchainKHR swapChain = VK_NULL_HANDLE;
    std::vector<VkImage> swapChainImages; // СЫРЫЕ фотографии фури

    VkFormat swapChainImageFormat;
    VkExtent2D swapChainExtent;

    VkRenderPass renderPass = VK_NULL_HANDLE;
    VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
    VkPipeline graphicsPipeline = VK_NULL_HANDLE;

    // Размер этих броу равен размеру буферизации нашего "пРиЛОженИя", т.е MAX_FRAMES_IN_FLIGHT
    std::vector<VkFramebuffer> swapChainFramebuffers; // Буфер фоточек
    std::vector<VkImageView> swapChainImageViews; // Уникальная обёртка на каждый кадр

    VkCommandPool commandPool;
    std::vector<VkCommandBuffer> commandBuffers;
    std::vector<VkSemaphore> imageAvailableSemaphores;
    std::vector<VkSemaphore> renderFinishedSemaphores;
    std::vector<VkFence> inFlightFences;
    uint32_t currentFrame = 0;

    bool framebufferResized = false;

    VkBuffer vertexBuffer;
    VkDeviceMemory vertexBufferMemory;
    VkBuffer indexBuffer;
    VkDeviceMemory indexBufferMemory;

    // UBO - Uniform Buffer Object
    VkDescriptorSetLayout descriptorSetLayout;

    std::vector<VkBuffer> uniformBuffers;
    std::vector<VkDeviceMemory> uniformBuffersMemory;

    // Connect UBO with desccriptors
    VkDescriptorPool descriptorPool;
    std::vector<VkDescriptorSet> descriptorSets;

    // В uniformBuffersMapped лежит именно массив указателей(по одному для каждого кадра в обработке),
    // которые ведут в «виртуальную» на процессор видеопамять.
    // Это нужно что-бы быстро на лету кидать в видеокарту новые данные о камере и получить адресса в памяти к источнику быстрых вычесленний
    std::vector<void*> uniformBuffersMapped;

    void initVulkan();

    void createDescriptorSets();

    void createDescriptorPool();

    void updateUniformBuffer(uint32_t currentImage);

    void createUniformBuffers();

    void createDescriptorSetLayout();

    void createIndexBuffer();

    void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);

    void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory);

    void createVertexBuffer();

    void recreateSwapChain();

    void cleanupSwapChain();

    void createSyncObjects();

    void drawFrame();

    void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex);

    void createCommandBuffers();

    void createCommandPool();

    void createFramebuffers();

    void createGraphicsPipeline();

    void createRenderPass();

    void createSwapChain();

    void createImageViews();

    VkResult CreateDebugUtilsMessengerEXT(
        VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
        const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger);

    void setupDebugMessenger();

    void DestroyDebugUtilsMessengerEXT(
        VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger,
        const VkAllocationCallbacks* pAllocator);

    void createSurface();

    void pickPhysicalDevice();

    void createLogicalDevice();

    void mainLoop();

    void cleanup();

    static void framebufferResizeCallback(GLFWwindow* window, int width, int height);

    void initWindow();

    void createInstance();

    void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);

    std::vector<const char*> getRequiredExtensions();

    bool checkValidationLayerSupport();

    static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
        VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
        VkDebugUtilsMessageTypeFlagsEXT messageType,
        const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
        void* pUserData);

    uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);

    QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device);

    bool isDeviceSuitable(VkPhysicalDevice device);

    bool checkDeviceExtensionSupport(VkPhysicalDevice device);

    SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device); // Проверка если вообще устройство которое поддерживает базовые (Границы экрана, Формат цвета, режим вывода)

    VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats); // Заполняем формат

    VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes); //  Заполняем режим вывода

    VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities); // Заполняем границы экрана

    VkShaderModule createShaderModule(const std::vector<char>& code);

};

#endif