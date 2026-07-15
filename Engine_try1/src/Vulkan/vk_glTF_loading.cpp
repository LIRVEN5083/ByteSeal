#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STBI_MSC_SECURE_CRT

#include "vk_glTF_loading.h"

VK_LOADING::BoundingBox::BoundingBox(){

}

VK_LOADING::BoundingBox::BoundingBox(glm::vec3 min, glm::vec3 max){

}

VK_LOADING::BoundingBox VK_LOADING::BoundingBox::getAABB(glm::mat4 m){

}

void VK_LOADING::Texture::updateDescriptor(){

}

void VK_LOADING::Texture::destroy(){

}

void VK_LOADING::Texture::fromglTfImage(tinygltf::Image& gltfimage, std::string path, TextureSampler textureSampler,
    VK_INIT_ENGINE::_inited_engine* _init, VkQueue copyQueue){

}


VK_LOADING::Primitive::Primitive(uint32_t firstIndex, uint32_t indexCount, uint32_t vertexCount, Material& material):
firstIndex(firstIndex), indexCount(indexCount), vertexCount(vertexCount), material(material){

}

void VK_LOADING::Primitive::setBoundingBox(glm::vec3 min, glm::vec3 max){

}

VK_LOADING::Mesh::Mesh(glm::mat4 matrix){

}

VK_LOADING::Mesh::~Mesh(){

}

void VK_LOADING::Mesh::setBoundingBox(glm::vec3 min, glm::vec3 max){

}

glm::mat4 VK_LOADING::Node::localMatrix(){

}

glm::mat4 VK_LOADING::Node::getMatrix(){

}

void VK_LOADING::Node::update(){

}

VK_LOADING::Node::~Node(){

}

void VK_LOADING::Model::loadNode(VK_LOADING::Node* parent, const tinygltf::Node& node, uint32_t nodeIndex,
    const tinygltf::Model& model, LoaderInfo& loaderInfo, float globalscale){

}

void VK_LOADING::Model::getNodeProps(const tinygltf::Node& node, const tinygltf::Model& model, size_t& vertexCount,
    size_t& indexCount){

}

void VK_LOADING::Model::loadTextures(tinygltf::Model& gltfModel, VK_INIT_ENGINE::_inited_engine* _init,
    VkQueue transferQueue){

}

VkSamplerAddressMode VK_LOADING::Model::getVkWrapMode(int32_t wrapMode){

}

VkFilter VK_LOADING::Model::getVkFilterMode(int32_t filterMode){

}

void VK_LOADING::Model::loadTextureSamplers(tinygltf::Model& gltfModel){

}

void VK_LOADING::Model::loadMaterials(tinygltf::Model& gltfModel){

}

void VK_LOADING::Model::loadFromFile(std::string filename, VK_INIT_ENGINE::_inited_engine* _init, VkQueue transferQueue,
    float scale){

}

void VK_LOADING::Model::drawNode(Node* node, VkCommandBuffer commandBuffer){

}

void VK_LOADING::Model::draw(VkCommandBuffer commandBuffer){

}

void VK_LOADING::Model::calculateBoundingBox(Node* node, Node* parent){

}

void VK_LOADING::Model::getSceneDimensions(){

}

VK_LOADING::Node* VK_LOADING::Model::findNode(Node* parent, uint32_t index){

}

VK_LOADING::Node* VK_LOADING::Model::nodeFromIndex(uint32_t index){

}
