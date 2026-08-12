#include "vk_scene.h"
#include "vk_glTF_loading.h"


glm::mat4 GameEntity::GetLocalMatrix() const {
    glm::mat4 model = glm::translate(glm::mat4(1.0f), position);

    // Переводим кватернион напрямую в mat4
    model = model * glm::toMat4(rotation);

    model = glm::scale(model, scale);
    return model;
}

AABB GameEntity::GetWorldAABB(ModelManager& modelManager) const{
    AABB localBox = modelManager.GetModelAABB(this->modelAssetId);

    glm::mat4 modelMatrix = GetLocalMatrix();

    glm::vec3 vertices[8] = {
        localBox.min,
        {localBox.max.x, localBox.min.y, localBox.min.z},
        {localBox.min.x, localBox.max.y, localBox.min.z},
        {localBox.min.x, localBox.min.y, localBox.max.z},
        {localBox.min.x, localBox.max.y, localBox.max.z},
        {localBox.max.x, localBox.min.y, localBox.max.z},
        {localBox.max.x, localBox.max.y, localBox.min.z},
        localBox.max
    };

    glm::vec3 minBound(std::numeric_limits<float>::max());
    glm::vec3 maxBound(-std::numeric_limits<float>::max());

    for (int i = 0; i < 8; ++i) {
        glm::vec3 worldVert = glm::vec3(modelMatrix * glm::vec4(vertices[i], 1.0f));
        minBound = glm::min(minBound, worldVert);
        maxBound = glm::max(maxBound, worldVert);
    }

    return AABB{ minBound, maxBound };
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

void Scene::CullingAndSubmit(RenderSystem& renderSystem, PipelineManager& pipelineManager, const glm::vec3& cameraPosition){

    RealPipeline* gridPipeline = pipelineManager.GetPipeline("Grid");
    if (gridPipeline)
    {
        RenderObject gridRo{};
        gridRo.render_matrix = glm::mat4(1.0f);

        gridRo.indexBuffer = VK_NULL_HANDLE;
        gridRo.vertexBufferAddress = 0;
        gridRo.indexCount = 6;
        gridRo.firstIndex = 0;

        gridRo.pipeline = gridPipeline->pipeline;
        gridRo.pipelineLayout = gridPipeline->layout;

        gridRo.colorTextureID = 0;
        gridRo.metallicRoughnessTextureID = 0;
        gridRo.normalTextureID = 0;
        gridRo.occlusionTextureID = 0;
        gridRo.baseColorFactor = glm::vec4(1.0f);
        gridRo.materialFactors = glm::vec4(1.0f, 0.0f, 0.0f, 0.0f);

        uint64_t gridOpacity = 2;
        uint64_t gridKey = 0;
        gridKey |= (gridOpacity & 0x3ULL) << 62;
        gridKey |= (static_cast<uint64_t>(gridPipeline->id) & 0x3FFFULL) << 48; // Сдвиг на 48 бит, чтобы ключ совпал по формату с мешами

        gridRo.sortKey = gridKey;

        renderSystem.Submit(gridRo);
    }

     if (_entities.empty()) return;

    for (const auto& entity : _entities)
    {
        if (!entity.bIsVisible) continue;
        if (!_modelManager.has_model(entity.modelAssetId)) continue;

        Model& model = _modelManager.GetModel(entity.modelAssetId);
        if (!model.bIsValid || !model.rootNode) continue;

        glm::mat4 entityWorldMatrix = entity.GetLocalMatrix();
        model.rootNode->UpdateMatrices(entityWorldMatrix);

        // Считаем мировую позицию самого объекта для сортировки прозрачности
        glm::vec3 entityWorldPos = glm::vec3(entityWorldMatrix[3].x, entityWorldMatrix[3].y, entityWorldMatrix[3].z);
        float distanceToCamera = glm::distance(entityWorldPos, cameraPosition);

        for (const auto& meshNode : model.meshNodes) {
            if (!meshNode->mesh) continue;

            for (const auto& surface : meshNode->mesh->surfaces) {
                if (!surface.material) continue;

                std::string targetPipelineName = surface.material->pipelineName;
                if (targetPipelineName.empty()) {
                    targetPipelineName = "BaseMesh";
                }

                RealPipeline* pipeline = pipelineManager.GetPipeline(targetPipelineName);
                if (!pipeline) continue;

                RenderObject ro;
                ro.render_matrix = meshNode->worldTransform;
                ro.indexBuffer = meshNode->mesh->meshBuffers.indexBuffer.buffer;
                ro.vertexBufferAddress = meshNode->mesh->meshBuffers.vertexBufferAddress;
                ro.indexCount = surface.count;
                ro.firstIndex = surface.startIndex;
                ro.pipeline = pipeline->pipeline;
                ro.pipelineLayout = pipeline->layout;

                ro.colorTextureID = surface.material->colorTextureID;
                ro.metallicRoughnessTextureID = surface.material->metallicRoughnessTextureID;
                ro.normalTextureID = surface.material->normalTextureID;
                ro.occlusionTextureID = surface.material->occlusionTextureID;
                ro.baseColorFactor = surface.material->baseColorFactor;

                ro.materialFactors = glm::vec4(
                    surface.material->roughnessFactor,
                    surface.material->metallicFactor,
                    0.0f, // padding
                    0.0f  // padding
                );

                // Определяем секцию прозрачности
                uint64_t opacitySection = 0; // Opaque (BaseMesh)
                if (pipeline->opacity == PipelineOpacity::Transparent) {
                    opacitySection = 2; // Transparent (TransparentMesh)
                } else if (pipeline->opacity == PipelineOpacity::AlphaTested) {
                    opacitySection = 1;
                }

                // Сборка ключа сортировки
                uint64_t key = 0;
                key |= (opacitySection & 0x3ULL) << 62;
                key |= (static_cast<uint64_t>(pipeline->id) & 0x3FFFULL) << 48;
                key |= (static_cast<uint64_t>(ro.colorTextureID) & 0xFFFFULL) << 32;

                if (opacitySection == 2) {
                    float safeDist = (distanceToCamera < 0.01f) ? 0.01f : distanceToCamera;
                    float invDist = 1.0f / safeDist;

                    // Умножаем на миллион для сохранения высокой точности float в uint32
                    uint32_t quantizedDist = static_cast<uint32_t>(invDist * 1000000.0f);
                    key |= (quantizedDist & 0xFFFFFFFFULL);
                }

                ro.sortKey = key;
                renderSystem.Submit(ro);
            }
        }
    }
}

RaycastHit Scene::Raycast(const Ray& ray){
    RaycastHit closestHit;

    for (auto& entity : _entities) {
        // Если объект скрыт в ImGui, кликнуть по нему нельзя
        if (!entity.bIsVisible) continue;

        // Берём наш AABB модельки
        AABB worldBox = entity.GetWorldAABB(_modelManager);

        float dist = 0.0f;
        // Вызываем алгоритм Смита
        if (ray.IntersectsAABB(worldBox, dist)) {
            // Ищем самый ближний объект к камере
            if (dist < closestHit.distance) {
                closestHit.hit = true;
                closestHit.distance = dist;
                closestHit.entity = &entity; // Запоминаем указатель на GameEntity
            }
        }
    }

    return closestHit;
}

Ray::Ray(const glm::vec3& origin, const glm::vec3& direction)
: _origin(origin), _direction(glm::normalize(direction)) {}

Ray Ray::FromScreen(float screenX, float screenY, float screenWidth, float screenHeight, const GPUSceneData& sceneData){
    float x = (2.0f * screenX) / screenWidth - 1.0f;
    float y = (2.0f * screenY) / screenHeight - 1.0f;

    glm::vec4 rayClip = glm::vec4(x, y, 0.0f, 1.0f);

    glm::vec4 rayEye = glm::inverse(sceneData.proj) * rayClip;

    rayEye.z = -1.0f;
    rayEye.w = 0.0f; // Сбрасываем W, так как нам нужен чистый вектор направления

    glm::mat4 invView = glm::inverse(sceneData.view);
    glm::vec3 rayDirWorld = glm::normalize(glm::vec3(invView * rayEye));

    glm::vec3 rayOriginWorld = glm::vec3(invView[3]);

    return Ray(rayOriginWorld, rayDirWorld);
}

bool Ray::IntersectsAABB(const AABB& box, float& outDist) const{
    glm::vec3 invDir = 1.0f / _direction;

    // Ось X
    float tmin = (box.min.x - _origin.x) * invDir.x;
    float tmax = (box.max.x - _origin.x) * invDir.x;
    if (tmin > tmax) std::swap(tmin, tmax);

    // Ось Y
    float tymin = (box.min.y - _origin.y) * invDir.y;
    float tymax = (box.max.y - _origin.y) * invDir.y;
    if (tymin > tymax) std::swap(tymin, tymax);

    if ((tmin > tymax) || (tymin > tmax)) return false;
    if (tymin > tmin) tmin = tymin;
    if (tymax < tmax) tmax = tymax;

    // Ось Z
    float tzmin = (box.min.z - _origin.z) * invDir.z;
    float tzmax = (box.max.z - _origin.z) * invDir.z;
    if (tzmin > tzmax) std::swap(tzmin, tzmax);

    if ((tmin > tzmax) || (tzmin > tmax)) return false;
    if (tzmin > tmin) tmin = tzmin;
    if (tzmax < tmax) tmax = tzmax;

    // Проверка: если tmax < 0, то весь бокс находится позади луча
    if (tmax < 0) return false;

    // Если tmin < 0, значит начало луча находится внутри самого бокса
    if (tmin < 0) {
        outDist = tmax; // Возвращаем точку выхода из бокса
    } else {
        outDist = tmin; // Возвращаем точку входа в бокс
    }

    return true;
}
