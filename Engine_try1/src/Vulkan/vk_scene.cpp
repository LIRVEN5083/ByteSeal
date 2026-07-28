#include "vk_scene.h"
#include "vk_glTF_loading.h"


glm::mat4 GameEntity::GetLocalMatrix() const {
    glm::mat4 translationMat = glm::translate(glm::mat4{1.0f}, position);

    glm::mat4 rotationMat = glm::rotate(glm::mat4{1.0f}, glm::radians(rotation.x), glm::vec3(1.0f, 0.0f, 0.0f)) *
                             glm::rotate(glm::mat4{1.0f}, glm::radians(rotation.y), glm::vec3(0.0f, 1.0f, 0.0f)) *
                             glm::rotate(glm::mat4{1.0f}, glm::radians(rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));

    glm::mat4 scaleMat = glm::scale(glm::mat4{1.0f}, scale);

    return translationMat * rotationMat * scaleMat;
}

GameEntity* Scene::CreateEntity(const std::string& name, uint32_t modelAssetId){
    uint32_t newId = _nextEntityId++;

    GameEntity entity;
    entity.id = newId;
    entity.name = name;
    entity.modelAssetId = modelAssetId;

    _idToIndex[newId] = _entities.size();

    _entities.push_back(entity);

    return &_entities.back();
}

GameEntity* Scene::GetEntity(uint32_t id){
    auto it = _idToIndex.find(id);
    if (it != _idToIndex.end()) {
        size_t vectorIndex = it->second;
        return &_entities[vectorIndex];
    }
    return nullptr;
}

void Scene::DestroyEntity(uint32_t id){
    auto it = _idToIndex.find(id);
    if (it == _idToIndex.end()) return;

    size_t indexToRemove = it->second;
    size_t lastIndex = _entities.size() - 1;

    if (indexToRemove < lastIndex) {
        GameEntity& lastEntity = _entities.back();
        _entities[indexToRemove] = std::move(lastEntity);
        _idToIndex[lastEntity.id] = indexToRemove;
    }

    _entities.pop_back();

    _idToIndex.erase(it);
}

void Scene::DestroyAllEntites(){
    _entities.clear();
    _idToIndex.clear();
    _nextEntityId = 1;
}

void Scene::DestroyAllDynamicEntites(){
    std::erase_if(_entities, [this](const GameEntity& entity) {
            if (_modelManager.has_model(entity.modelAssetId)) {
                return _modelManager.GetModel(entity.modelAssetId).lifetime == ModelLifetime::Dynamic;
            }
            return false;
        });

    _idToIndex.clear();
    for (size_t i = 0; i < _entities.size(); ++i) {
        _idToIndex[_entities[i].id] = i;
    }
}

void Scene::DestroyEntitiesByModel(uint32_t modelAssetId){
    std::erase_if(_entities, [this, modelAssetId](const GameEntity& entity) {
        // Проверяем, совпадает ли ID модели
        if (entity.modelAssetId == modelAssetId) {
            if (_modelManager.has_model(modelAssetId)) {
                const Model& model = _modelManager.GetModel(modelAssetId);
                // Если модель статическая, то её удалить НАХУЙ БЛЯТЬ нельзя!
                if (model.lifetime == ModelLifetime::Static) {
                    return false;
                }
            }
            return true;
        }
        return false;
    });

    _idToIndex.clear();
    for (size_t i = 0; i < _entities.size(); ++i) {
        _idToIndex[_entities[i].id] = i;
    }
}

void Scene::CullingAndSubmit(RenderSystem& renderSystem, VkPipeline defaultPipeline, VkPipelineLayout defaultLayout){
    if (_entities.empty()) return;

    for (const auto& entity : _entities) {
        if (!entity.bIsVisible) continue;

        // Получаем данные о место нахождении обьекта
        glm::mat4 entityWorldMatrix = entity.GetLocalMatrix();

        // Запрашиваем структуру модели
        if (!_modelManager.has_model(entity.modelAssetId)) continue;
        const Model& model = _modelManager.GetModel(entity.modelAssetId);
        if (!model.bIsValid) continue;

        // Обходим внутренние ноды модели (саб-меши), если они есть
        for (const auto& meshNode : model.meshNodes) {
            if (!meshNode || !meshNode->mesh) continue;

            auto& mesh = meshNode->mesh;
            for (const auto& surface : mesh->surfaces) {

                // Формируем RenderObject для рендер-системы
                RenderObject ro;
                ro.indexBuffer = mesh->meshBuffers.indexBuffer.buffer;
                ro.vertexBufferAddress = mesh->meshBuffers.vertexBufferAddress;
                ro.indexCount = surface.count;
                ro.firstIndex = surface.startIndex;
                ro.pipeline = defaultPipeline;
                ro.pipelineLayout = defaultLayout;

                // Обходим материалы
                if (surface.material) {
                    ro.colorTextureID = surface.material->colorTextureID;
                    ro.metallicRoughnessTextureID = surface.material->metallicRoughnessTextureID;
                }

                // Перемножение локальной ноды на внешние изменения
                ro.render_matrix = entityWorldMatrix * meshNode->worldTransform;


                // Отправляем в НАШУ СУПЕР ДУПЕР RENDER SYSTEM
                renderSystem.Submit(ro);
            }
        }
    }
}
