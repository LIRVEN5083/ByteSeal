#pragma once

#include "vk_types.h"
#include "vk_init_engine.h"

#if defined(_WIN32) && defined(ERROR) && defined(TINYGLTF_ENABLE_DRACO)
#undef ERROR
#pragma message ("ERROR constant already defined, undefining")
#endif

#define TINYGLTF_NO_STB_IMAGE_WRITE
#include "tinygltf/tiny_gltf.h"

namespace VK_LOADING{
    struct Node;

    // Это визуальная колизия обьекта
    // Вирутальная коробка в которой 3Д модель
    struct BoundingBox {
        glm::vec3 min;
        glm::vec3 max;
        bool valid = false;
        BoundingBox();
        BoundingBox(glm::vec3 min, glm::vec3 max);
        BoundingBox getAABB(glm::mat4 m);
    };

    // Как читать текстуру
    struct TextureSampler {
        // Фильтрация пикселей что-бы не было артефактов
        // Magnification - слишком близко
        VkFilter magFilter;
        // Minification - слишком далеко
        VkFilter minFilter;
        // VkSampelrAddressMode - правило если текстура закончилась
        // UV координаты от 0 до 1
        // Правило для U координаты
        VkSamplerAddressMode addressModeU;
        // Правило для V координаты
        VkSamplerAddressMode addressModeV;
        // W - это третья координата
        VkSamplerAddressMode addressModeW;
    };

    // Изображение
    struct Texture {
        VK_INIT_ENGINE::_inited_engine* _init;
        VkImage image;
        VkImageLayout imageLayout;
        VkImageView view;

        uint32_t width, height;
        uint32_t mipLevels;

        uint32_t layerCount;
        VkDescriptorImageInfo descriptor;
        VkSampler sampler;

        // Место аллокации
        VmaAllocation allocation;

        void updateDescriptor();
        void destroy();
        void fromglTfImage(tinygltf::Image& gltfimage, std::string path, TextureSampler textureSampler, VK_INIT_ENGINE::_inited_engine* _init, VkQueue copyQueue);
    };

    // Определение параметров материалов нашей модели
    struct Material {
        enum AlphaMode{ ALPHAMODE_OPAQUE, ALPHAMODE_MASK, ALPHAMODE_BLEND };
        AlphaMode alphaMode = ALPHAMODE_OPAQUE;
        float alphaCutoff = 1.0f;
        float metallicFactor = 1.0f;
        float roughnessFactor = 1.0f;
        glm::vec4 baseColorFactor = glm::vec4(1.0f);
        glm::vec4 emissiveFactor = glm::vec4(0.0f);
        VK_LOADING::Texture *baseColorTexture;
        VK_LOADING::Texture *metallicRoughnessTexture;
        VK_LOADING::Texture *normalTexture;
        VK_LOADING::Texture *occlusionTexture;
        VK_LOADING::Texture *emissiveTexture;
        bool doubleSided = false;
        struct TexCoordSets {
            uint8_t baseColor = 0;
            uint8_t metallicRoughness = 0;
            uint8_t specularGlossiness = 0;
            uint8_t normal = 0;
            uint8_t occlusion = 0;
            uint8_t emissive = 0;
        } texCoordSets;
        struct Extension {
            VK_LOADING::Texture *specularGlossinessTexture;
            VK_LOADING::Texture *diffuseTexture;
            glm::vec4 diffuseFactor = glm::vec4(1.0f);
            glm::vec3 specularFactor = glm::vec3(0.0f);
        } extension;
        struct PbrWorkflows {
            bool metallicRoughness = true;
            bool specularGlossiness = false;
        } pbrWorkflows;
        VkDescriptorSet descriptorSet = VK_NULL_HANDLE;
        int index = 0;
        bool unlit = false;
        float emissiveStrength = 1.0f;
    };

    struct Primitive {
        uint32_t firstIndex;
        uint32_t indexCount;
        uint32_t vertexCount;
        Material &material;
        bool hasIndices;
        BoundingBox bb;
        Primitive(uint32_t firstIndex, uint32_t indexCount, uint32_t vertexCount, Material& material);
        void setBoundingBox(glm::vec3 min, glm::vec3 max);
    };

    struct Mesh {
        std::vector<Primitive*> primitives;
        BoundingBox bb;
        BoundingBox aabb;
        glm::mat4 matrix;
        uint32_t index;
        Mesh(glm::mat4 matrix);
        ~Mesh();
        void setBoundingBox(glm::vec3 min, glm::vec3 max);
    };
}