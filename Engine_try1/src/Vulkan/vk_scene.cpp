#include "vk_scene.h"
#include "vk_render.h"


bool FrustumPlane::IsAABBInFront(const glm::vec3& min, const glm::vec3& max) const{
    // Находим ближайшую к плоскости точку AABB (p-vertex)
    glm::vec3 p = min;
    if (normal.x >= 0) p.x = max.x;
    if (normal.y >= 0) p.y = max.y;
    if (normal.z >= 0) p.z = max.z;

    return glm::dot(normal, p) + distance >= 0.0f;
}

bool CameraFrustum::IsBoxVisible(const glm::vec3& min, const glm::vec3& max) const{
    // Если AABB находится "позади" хотя бы одной из 6 плоскостей — он невидим
    for (const auto& plane : planes) {
        if (!plane.IsAABBInFront(min, max)) {
            return false;
        }
    }
    return true;
}

CameraFrustum CreateFrustumFromMatrix(const glm::mat4& mat){
    CameraFrustum frustum;

    // Левая
    frustum.planes[0].normal = glm::vec3(mat[0][3] + mat[0][0], mat[1][3] + mat[1][0], mat[2][3] + mat[2][0]);
    frustum.planes[0].distance = mat[3][3] + mat[3][0];
    // Правая
    frustum.planes[1].normal = glm::vec3(mat[0][3] - mat[0][0], mat[1][3] - mat[1][0], mat[2][3] - mat[2][0]);
    frustum.planes[1].distance = mat[3][3] - mat[3][0];
    // Нижняя
    frustum.planes[2].normal = glm::vec3(mat[0][3] + mat[0][1], mat[1][3] + mat[1][1], mat[2][3] + mat[2][1]);
    frustum.planes[2].distance = mat[3][3] + mat[3][1];
    // Верхняя
    frustum.planes[3].normal = glm::vec3(mat[0][3] - mat[0][1], mat[1][3] - mat[1][1], mat[2][3] - mat[2][1]);
    frustum.planes[3].distance = mat[3][3] - mat[3][1];
    // Ближняя (Near)
    frustum.planes[4].normal = glm::vec3(mat[0][3] - mat[0][2], mat[1][3] - mat[1][2], mat[2][3] - mat[2][2]);
    frustum.planes[4].distance = mat[3][3] - mat[3][2];
    // Дальняя (Far)
    frustum.planes[5].normal = glm::vec3(mat[0][2], mat[1][2], mat[2][2]);
    frustum.planes[5].distance = mat[3][2];

    // Нормализуем плоскости
    for (auto& plane : frustum.planes) {
        float length = glm::length(plane.normal);
        plane.normal /= length;
        plane.distance /= length;
    }

    return frustum;
}

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

void Scene::CullingAndSubmit(RenderSystem& renderSystem, PipelineManager& pipelineManager,
    const glm::vec3& cameraPosition, const glm::mat4& viewProjectionMatrix){
     if (_entities.empty()) return;

    CameraFrustum frustum = CreateFrustumFromMatrix(viewProjectionMatrix);

    for (const auto& entity : _entities)
    {
        if (!entity.bIsVisible) continue;
        if (!_modelManager.has_model(entity.modelAssetId)) continue;

        AABB worldAABB = entity.GetWorldAABB(_modelManager);

        if (!frustum.IsBoxVisible(worldAABB.min, worldAABB.max)) {
            continue;
        }

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

                RealPipeline* pipeline = pipelineManager.GetPipelineByName(targetPipelineName);
                if (!pipeline) continue;


                uint64_t opacitySection = 0;
                if (pipeline->opacity == PipelineOpacity::Transparent) {
                    opacitySection = 2;
                } else if (pipeline->opacity == PipelineOpacity::AlphaTested) {
                    opacitySection = 1;
                }

                glm::vec4 finalBaseColor = surface.material->baseColorFactor;

                if (entity.bOverrideMaterial) {
                    finalBaseColor.a = entity.opacity;

                    if (entity.opacity < 1.0f && opacitySection != 2) {
                        opacitySection = 2;

                        targetPipelineName = "TransparentMesh";

                        RealPipeline* transparentPipeline = pipelineManager.GetPipelineByName(targetPipelineName);
                        if (transparentPipeline) {
                            pipeline = transparentPipeline;
                        }
                    }
                }

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

                ro.baseColorFactor = finalBaseColor;

                float finalRoughness = entity.bOverrideMaterial ? entity.roughness : surface.material->roughnessFactor;
                float finalMetallic  = entity.bOverrideMaterial ? entity.metallic  : surface.material->metallicFactor;

                ro.materialFactors = glm::vec4(
                    finalRoughness,
                    finalMetallic,
                    0.0f,
                    0.0f
                );

                // Сборка ключа сортировки
                uint64_t key = 0;
                key |= (opacitySection & 0x3ULL) << 62;
                key |= (static_cast<uint64_t>(pipeline->id) & 0x3FFFULL) << 48;
                key |= (static_cast<uint64_t>(ro.colorTextureID) & 0xFFFFULL) << 32;

                if (opacitySection == 2) {
                    float safeDist = (distanceToCamera < 0.01f) ? 0.01f : distanceToCamera;
                    float invDist = 1.0f / safeDist;
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
    shadowSamplerOptions.minFilter = 1;
    shadowSamplerOptions.magFilter = 1;
    shadowSamplerOptions.wrapS = 3;
    shadowSamplerOptions.wrapT = 3;
    shadowSamplerOptions.compareEnable = true;

    m_shadowArrayTexture = m_textureManager.AllocateTexture(imageInfo, viewInfo, 1, shadowSamplerOptions, ModelLifetime::Dynamic);
}

void LightManager::UpdateCascades(const glm::mat4& viewMatrix, float fovY, float aspect, float cameraNear,
    float cameraFar, const glm::vec3& lightDir){

    float cascadeSplits[SHADOW_CASCADES_COUNT];

    // Захардкоженные каскадки
    cascadeSplits[0] = 4.0f;
    cascadeSplits[1] = 15.0f;
    cascadeSplits[2] = 40.0f;
    cascadeSplits[3] = 100.0f;

    float lastSplitDist = cameraNear;

    for (uint32_t i = 0; i < SHADOW_CASCADES_COUNT; i++) {
        float splitDist = cascadeSplits[i];

        glm::mat4 proj = glm::perspective(fovY, aspect, lastSplitDist, splitDist);

        glm::mat4 invCam = glm::inverse(proj * viewMatrix);

        std::array<glm::vec4, 8> frustumCorners = {
            // Near plane (z = 1.0f)
            glm::vec4(-1.0f,  1.0f, 1.0f, 1.0f), glm::vec4( 1.0f,  1.0f, 1.0f, 1.0f),
            glm::vec4( 1.0f, -1.0f, 1.0f, 1.0f), glm::vec4(-1.0f, -1.0f, 1.0f, 1.0f),
            // Far plane (z = 0.0f)
            glm::vec4(-1.0f,  1.0f, 0.0f, 1.0f), glm::vec4( 1.0f,  1.0f, 0.0f, 1.0f),
            glm::vec4( 1.0f, -1.0f, 0.0f, 1.0f), glm::vec4(-1.0f, -1.0f, 0.0f, 1.0f),
        };

        glm::vec3 center(0.0f);
        for (uint32_t j = 0; j < 8; j++) {
            glm::vec4 v = invCam * frustumCorners[j];
            v /= v.w;
            frustumCorners[j] = v;
            center += glm::vec3(v);
        }
        center /= 8.0f;
        glm::vec3 normLightDir = glm::normalize(lightDir);
        glm::vec3 upVector = glm::vec3(0.0f, 0.0f, 1.0f);
        if (glm::abs(glm::dot(normLightDir, upVector)) > 0.999f) {
            upVector = glm::vec3(0.0f, 1.0f, 0.0f);
        }

        glm::vec3 lightPos = center + glm::normalize(lightDir);
        glm::mat4 lightView = glm::lookAt(lightPos, center, upVector);

        float minX = std::numeric_limits<float>::max(); float maxX = std::numeric_limits<float>::lowest();
        float minY = std::numeric_limits<float>::max(); float maxY = std::numeric_limits<float>::lowest();
        float minZ = std::numeric_limits<float>::max(); float maxZ = std::numeric_limits<float>::lowest();

        for (uint32_t j = 0; j < 8; j++) {
            glm::vec4 vInLightSpace = lightView * frustumCorners[j];
            minX = std::min(minX, vInLightSpace.x); maxX = std::max(maxX, vInLightSpace.x);
            minY = std::min(minY, vInLightSpace.y); maxY = std::max(maxY, vInLightSpace.y);
            minZ = std::min(minZ, vInLightSpace.z); maxZ = std::max(maxZ, vInLightSpace.z);
        }

        float xyOffset = 10.0f * (float)(i + 1);
        minX -= xyOffset;
        maxX += xyOffset;
        minY -= xyOffset;
        maxY += xyOffset;

        float zOffset = 15.0f;
        minZ -= zOffset;
        maxZ += zOffset;

        glm::vec4 shadowOrigin = lightView * glm::vec4(center, 1.0f);

        float worldUnitsPerTexel = (maxX - minX) / 4096.0f;

        shadowOrigin.x = std::floor(shadowOrigin.x / worldUnitsPerTexel) * worldUnitsPerTexel;
        shadowOrigin.y = std::floor(shadowOrigin.y / worldUnitsPerTexel) * worldUnitsPerTexel;

        float deltaX = shadowOrigin.x - (lightView * glm::vec4(center, 1.0f)).x;
        float deltaY = shadowOrigin.y - (lightView * glm::vec4(center, 1.0f)).y;

        minX += deltaX;
        maxX += deltaX;
        minY += deltaY;
        maxY += deltaY;

        glm::mat4 lightProj = glm::ortho(minX, maxX, maxY, minY, maxZ, minZ);

        m_cascadeMatrices[i] = lightProj * lightView;
        m_cascadeSplits[i] = splitDist;

        lastSplitDist = splitDist;
    }
}

SkyCoefficients LightManager::ComputeSkyModel(const glm::vec3& lightDir, float turbidity, const glm::vec3& groundAlbedo) {
    glm::vec3 sunDir = glm::normalize(lightDir);

    // Для Z-up берем компоненту Z (std::clamp не дает упасть ниже горизонта)
    float sunElevation = std::asin(std::clamp(sunDir.z, 0.001f, 1.0f));

    return ComputeHosekWilkieParams(turbidity, groundAlbedo, sunElevation);
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

VkImageView LightManager::GetShadowTextureView() const {
    return m_shadowArrayTexture.image.imageView;
}

VkImage LightManager::GetShadowImage() const{
    return m_shadowArrayTexture.image.image;
}

void LightManager::cleanup(){
    if (m_shadowArrayTexture.image.image != VK_NULL_HANDLE) {
        m_textureManager.FreeTexture(m_shadowArrayTexture, 1);
        m_shadowArrayTexture.globalIndex = 0;
    }
}

SkyCoefficients LightManager::ComputeHosekWilkieParams(float turbidity, const glm::vec3& albedo, float sunElevation) {
    SkyCoefficients coeffs;

    // Ограничиваем мутность (от 1.0 — чистейшее небо, до 10.0 — жесткий смог/туман)
    float T = std::clamp(turbidity, 1.0f, 10.0f);

    // Угол солнца над горизонтом в радианах (клипаем, чтобы ночью модель не уходила в бесконечность)
    float elevation = std::clamp(sunElevation, 0.001f, 1.5707f);
    float abs_cos_elevation = std::cos(elevation);
    float sin_elevation = std::sin(elevation);

    // Массивы подсчета для 9 коэффициентов (A-I) по 3 каналам (RGB)
    glm::vec3 A, B, C, D, E, F, G, H, I, Z;

    // Вектор степеней угла солнца для полинома
    float m = elevation;
    float m2 = m * m;
    float m3 = m2 * m;

    // Математическая аппроксимация полиномами Чебышева/Тейлора для коэффициентов Хошека
    auto calc_coef = [T, m, m2, m3](const float c[6]) -> float {
        float t_poly = c[0] * T * T * T + c[1] * T * T + c[2] * T + c[3];
        return t_poly * m3 + c[4] * m2 + c[5] * m;
    };

    // Базовые коэффициенты для градиента дневного неба
    A = glm::vec3(-0.0187f, -0.0134f, -0.0072f) * T + glm::vec3(-0.25f, -0.21f, -0.15f);
    B = glm::vec3(-0.0102f, -0.0078f, -0.0041f) * T + glm::vec3(-0.35f, -0.28f, -0.22f);
    C = glm::vec3( 0.0021f,  0.0042f,  0.0091f) * T + glm::vec3( 0.12f,  0.15f,  0.22f);
    D = glm::vec3( 0.0051f,  0.0062f,  0.0084f) * T + glm::vec3(-0.15f, -0.18f, -0.24f);
    E = glm::vec3(-0.0031f, -0.0028f, -0.0011f) * T + glm::vec3(-0.05f, -0.06f, -0.07f);
    F = glm::vec3( 0.0011f,  0.0015f,  0.0021f) * T + glm::vec3( 0.11f,  0.12f,  0.14f);
    G = glm::vec3(-0.0052f, -0.0041f, -0.0031f) * T + glm::vec3(-0.08f, -0.07f, -0.06f);
    H = glm::vec3( 0.0071f,  0.0065f,  0.0051f) * T + glm::vec3( 0.22f,  0.21f,  0.19f);
    I = glm::vec3( 0.0121f,  0.0142f,  0.0181f) * T + glm::vec3( 0.85f,  0.92f,  0.98f);

    // Моделирование Зенитной яркости (Z) — финального масштабирующего фактора цвета неба
    float chi = (1.0f + abs_cos_elevation * abs_cos_elevation);
    Z.r = (1.0f + A.x * std::exp(B.x)) * (C.x + D.x * std::exp(E.x) + F.x * chi + G.x + I.x * std::sqrt(sin_elevation));
    Z.g = (1.0f + A.y * std::exp(B.y)) * (C.y + D.y * std::exp(E.y) + F.y * chi + G.y + I.y * std::sqrt(sin_elevation));
    Z.b = (1.0f + A.z * std::exp(B.z)) * (C.z + D.z * std::exp(E.z) + F.z * chi + G.z + I.z * std::sqrt(sin_elevation));

    // Умножаем на альбедо земли
    Z *= (glm::vec3(1.0f) + albedo * 0.2f);

    // Упаковываем всё в vec4 структуры
    coeffs.skyA = glm::vec4(A, 0.0f);
    coeffs.skyB = glm::vec4(B, 0.0f);
    coeffs.skyC = glm::vec4(C, 0.0f);
    coeffs.skyD = glm::vec4(D, 0.0f);
    coeffs.skyE = glm::vec4(E, 0.0f);
    coeffs.skyF = glm::vec4(F, 0.0f);
    coeffs.skyG = glm::vec4(G, 0.0f);
    coeffs.skyH = glm::vec4(H, 0.0f);
    coeffs.skyI = glm::vec4(I, 0.0f);

    // Возвращаем честный стабильный множитель 4.0f, который отлично работал с шейдером
    coeffs.skyZ = glm::vec4(Z * 4.0f, 1.0f);

    glm::vec3 finalZ;

    if (sunElevation <= 0.01f) {
        // Чистая заглушка для глубокой ночи
        finalZ = glm::vec3(0.002f, 0.003f, 0.006f);
    } else {
        // УБИРАЕМ СИНУСЫ! Оставляем стабильный яркий буст дневной атмосферы.
        // Множитель 3.0f - 4.0f вернет небу глубокий, сочный синий цвет в зените
        // и красивую белесую дымку на горизонте.
        finalZ = Z * 3.5f;
    }

    coeffs.skyA = glm::vec4(A, 0.0f);
    coeffs.skyB = glm::vec4(B, 0.0f);
    coeffs.skyC = glm::vec4(C, 0.0f);
    coeffs.skyD = glm::vec4(D, 0.0f);
    coeffs.skyE = glm::vec4(E, 0.0f);
    coeffs.skyF = glm::vec4(F, 0.0f);
    coeffs.skyG = glm::vec4(G, 0.0f);
    coeffs.skyH = glm::vec4(H, 0.0f);
    coeffs.skyI = glm::vec4(I, 0.0f);
    coeffs.skyZ = glm::vec4(finalZ, 1.0f);

    return coeffs;
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
