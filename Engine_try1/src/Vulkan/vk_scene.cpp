#include "vk_scene.h"
#include "vk_render.h"


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

void LightManager::init(){
     VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width = m_config.resolution;
    imageInfo.extent.height = m_config.resolution;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = SHADOW_CASCADES_COUNT;
    imageInfo.format = VK_FORMAT_D32_SFLOAT;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    imageInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D_ARRAY;
    viewInfo.format = VK_FORMAT_D32_SFLOAT;
    viewInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = SHADOW_CASCADES_COUNT;

    SamplerOptions shadowSamplerOptions{};
    shadowSamplerOptions.minFilter = 9729;
    shadowSamplerOptions.magFilter = 9729;
    // В Vulkan / OpenGL значения для CLAMP_TO_BORDER обычно: 33071 (GL_CLAMP_TO_BORDER) или 3 (VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER)
    shadowSamplerOptions.wrapS = 3;
    shadowSamplerOptions.wrapT = 3;

    m_shadowArrayTexture = m_textureManager.AllocateTexture(imageInfo, viewInfo, shadowSamplerOptions, ModelLifetime::Static);
}

void LightManager::UpdateCascades(const glm::mat4& viewMatrix, float fovY, float aspect, float cameraNear,
    float cameraFar, const glm::vec3& lightDir){

    float cascadeSplits[SHADOW_CASCADES_COUNT];

    // Считаем дистанции каскадов (Practical Split Scheme)
    for (uint32_t i = 0; i < SHADOW_CASCADES_COUNT; i++) {
        float p = m_config.cascadeSplits[i];
        float logSplit = cameraNear * std::pow(cameraFar / cameraNear, p);
        float uniSplit = cameraNear + (cameraFar - cameraNear) * p;
        cascadeSplits[i] = glm::mix(uniSplit, logSplit, m_config.splitLambda);
    }

    float lastSplitDist = cameraNear;

    for (uint32_t i = 0; i < SHADOW_CASCADES_COUNT; i++) {
        float splitDist = cascadeSplits[i];

         // Собираем ручками матрицу проекции без моей выебистой математики
        glm::mat4 proj = glm::perspective(fovY, aspect, lastSplitDist, splitDist);
        proj[1][1] *= -1.0f; // Любимая инверсия проекции

        glm::mat4 invCam = glm::inverse(proj * viewMatrix);

        // 8 угловых точек фрустума в NDC
        std::array<glm::vec4, 8> frustumCorners = {
            glm::vec4(-1.0f,  1.0f, 0.0f, 1.0f), glm::vec4( 1.0f,  1.0f, 0.0f, 1.0f),
            glm::vec4( 1.0f, -1.0f, 0.0f, 1.0f), glm::vec4(-1.0f, -1.0f, 0.0f, 1.0f),
            glm::vec4(-1.0f,  1.0f, 1.0f, 1.0f), glm::vec4( 1.0f,  1.0f, 1.0f, 1.0f),
            glm::vec4( 1.0f, -1.0f, 1.0f, 1.0f), glm::vec4(-1.0f, -1.0f, 1.0f, 1.0f),
        };

        glm::vec3 center(0.0f);
        for (uint32_t j = 0; j < 8; j++) {
            glm::vec4 v = invCam * frustumCorners[j];
            v /= v.w;
            frustumCorners[j] = v;
            center += glm::vec3(v);
        }
        center /= 8.0f; // Центр каскада вокруг игрока

        // Матрица вида света. Берем z-up
        glm::vec3 lightPos = center - glm::normalize(lightDir);
        glm::mat4 lightView = glm::lookAt(lightPos, center, glm::vec3(0.0f, 0.0f, 1.0f));

        // Границы в пространстве света
        float minX = std::numeric_limits<float>::max(); float maxX = std::numeric_limits<float>::lowest();
        float minY = std::numeric_limits<float>::max(); float maxY = std::numeric_limits<float>::lowest();
        float minZ = std::numeric_limits<float>::max(); float maxZ = std::numeric_limits<float>::lowest();

        for (uint32_t j = 0; j < 8; j++) {
            glm::vec4 vInLightSpace = lightView * frustumCorners[j];
            minX = std::min(minX, vInLightSpace.x); maxX = std::max(maxX, vInLightSpace.x);
            minY = std::min(minY, vInLightSpace.y); maxY = std::max(maxY, vInLightSpace.y);
            minZ = std::min(minZ, vInLightSpace.z); maxZ = std::max(maxZ, vInLightSpace.z);
        }

        // Большой Z-запас для направленного света (чтобы высокие объекты сзади не отсекались)
        float zMult = 10.0f;
        minZ = (minZ < 0) ? minZ * zMult : minZ / zMult;
        maxZ = (maxZ < 0) ? maxZ / zMult : maxZ * zMult;

        glm::mat4 lightProj = glm::ortho(minX, maxX, minY, maxY, minZ, maxZ);

        // В Vulkan для ортографической матрицы тоже нужен флип Y
        lightProj[1][1] *= -1.0f;

        m_cascadeMatrices[i] = lightProj * lightView;
        m_cascadeSplits[i] = splitDist;

        lastSplitDist = splitDist;
    }
}

uint32_t LightManager::GetShadowTextureIndex() const{
    return m_shadowArrayTexture.globalIndex;
}

const glm::mat4* LightManager::GetCascadeMatrices() const{
    return m_cascadeMatrices.data();
}

const float* LightManager::GetCascadeSplits() const{
    return m_cascadeSplits.data();
}

uint32_t LightManager::GetResolution() const{
    return m_config.resolution;
}

void LightManager::cleanUp(){
    if (m_shadowArrayTexture.globalIndex != 0) {
        m_textureManager.FreeTexture(m_shadowArrayTexture);
        m_shadowArrayTexture.globalIndex = 0;
    }
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
