#include "vk_imgui.h"

void VK_GUI::apply_theme(){
    ImGuiStyle& style = ImGui::GetStyle();
    ImVec4* colors = style.Colors;

    // --- 1. НАСТРОЙКА РАЗМЕРОВ И ФОРМ (Плоский и строгий стиль) ---
    style.WindowRounding    = 0.0f;  // Никаких круглых углов у окон (как в Blender)
    style.ChildRounding     = 0.0f;
    style.FrameRounding     = 3.0f;  // Легкое скругление полей ввода и кнопок
    style.PopupRounding     = 0.0f;
    style.ScrollbarRounding = 4.0f;
    style.GrabRounding      = 3.0f;
    style.TabRounding       = 2.0f;  // Едва заметное скругление вкладок сверху

    style.WindowBorderSize  = 1.0f;  // Тонкая дебаг-обводка между панелями
    style.FrameBorderSize   = 1.0f;
    style.PopupBorderSize   = 1.0f;

    style.WindowPadding     = ImVec2(8.0f, 8.0f);
    style.FramePadding      = ImVec2(6.0f, 4.0f); // Чуть просторнее поля ввода
    style.ItemSpacing       = ImVec2(6.0f, 4.0f);

    // --- 2. ЦВЕТОВАЯ ПАЛИТРА (Тёмно-серый уголь) ---
    // Текст
    colors[ImGuiCol_Text]                   = ImVec4(0.85f, 0.85f, 0.85f, 1.00f); // Светло-серый читаемый
    colors[ImGuiCol_TextDisabled]           = ImVec4(0.45f, 0.45f, 0.45f, 1.00f);

    // Фоны окон и панелей
    colors[ImGuiCol_WindowBg]               = ImVec4(0.18f, 0.18f, 0.18f, 1.00f); // Основной фон (Blender Gray)
    colors[ImGuiCol_ChildBg]                = ImVec4(0.14f, 0.14f, 0.14f, 1.00f); // Вложенные панели
    colors[ImGuiCol_PopupBg]                = ImVec4(0.12f, 0.12f, 0.12f, 0.95f); // Контекстные меню

    // Границы и обводки
    colors[ImGuiCol_Border]                 = ImVec4(0.11f, 0.11f, 0.11f, 1.00f); // Тёмные строгие стыки
    colors[ImGuiCol_BorderShadow]           = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);

    // Поля ввода, чекбоксы, слайдеры (Frames)
    colors[ImGuiCol_FrameBg]                = ImVec4(0.11f, 0.11f, 0.11f, 1.00f); // Очень тёмный фон инпутов
    colors[ImGuiCol_FrameBgHovered]         = ImVec4(0.24f, 0.24f, 0.24f, 1.00f);
    colors[ImGuiCol_FrameBgActive]          = ImVec4(0.28f, 0.28f, 0.28f, 1.00f);

    // Заголовки окон и панелей (Заголовки и Коллапсы)
    colors[ImGuiCol_TitleBg]                = ImVec4(0.14f, 0.14f, 0.14f, 1.00f); // Плоская верхняя плашка окна
    colors[ImGuiCol_TitleBgActive]          = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);
    colors[ImGuiCol_TitleBgCollapsed]       = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);

    // Скроллбары
    colors[ImGuiCol_ScrollbarBg]            = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);
    colors[ImGuiCol_ScrollbarGrab]          = ImVec4(0.31f, 0.31f, 0.31f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabHovered]   = ImVec4(0.40f, 0.40f, 0.40f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabActive]    = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);

    // Кнопки (Универсальные плоские кнопки)
    colors[ImGuiCol_Button]                 = ImVec4(0.25f, 0.25f, 0.25f, 1.00f); // Нейтрально-серый
    colors[ImGuiCol_ButtonHovered]          = ImVec4(0.33f, 0.33f, 0.33f, 1.00f); // Чуть светлее при наведении
    colors[ImGuiCol_ButtonActive]           = ImVec4(0.21f, 0.47f, 0.76f, 1.00f);  // Синий акцент Unity при клике

    // Заголовки CollapsingHeader и TreeNode
    colors[ImGuiCol_Header]                 = ImVec4(0.22f, 0.22f, 0.22f, 1.00f);
    colors[ImGuiCol_HeaderHovered]          = ImVec4(0.28f, 0.28f, 0.28f, 1.00f);
    colors[ImGuiCol_HeaderActive]           = ImVec4(0.21f, 0.47f, 0.76f, 1.00f);

    // Разделители (Separator)
    colors[ImGuiCol_Separator]              = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);
    colors[ImGuiCol_SeparatorHovered]       = ImVec4(0.21f, 0.47f, 0.76f, 1.00f);
    colors[ImGuiCol_SeparatorActive]        = ImVec4(0.21f, 0.47f, 0.76f, 1.00f);

    // Ползунки слайдеров (Grab)
    // СТАЛО (скомпилируется успешно):
    colors[ImGuiCol_SliderGrab]             = ImVec4(0.35f, 0.35f, 0.35f, 1.00f);
    colors[ImGuiCol_SliderGrabActive]       = ImVec4(0.21f, 0.47f, 0.76f, 1.00f);


    // Вкладки (Для Docking, если перейдешь, или для ручных табов)
    colors[ImGuiCol_Tab]                    = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);
    colors[ImGuiCol_TabHovered]             = ImVec4(0.24f, 0.24f, 0.24f, 1.00f);
    colors[ImGuiCol_TabActive]              = ImVec4(0.18f, 0.18f, 0.18f, 1.00f);
    colors[ImGuiCol_TabUnfocused]           = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);
    colors[ImGuiCol_TabUnfocusedActive]     = ImVec4(0.18f, 0.18f, 0.18f, 1.00f);

    // Линии графиков, подсветок текста и докинг-маркеров
    // colors[ImGuiCol_DockingPreview]         = ImVec4(0.21f, 0.47f, 0.76f, 0.70f);
    // colors[ImGuiCol_DockingEmptyBg]         = ImVec4(0.10f, 0.10f, 0.10f, 1.00f);
    colors[ImGuiCol_PlotLines]              = ImVec4(0.61f, 0.61f, 0.61f, 1.00f);
    colors[ImGuiCol_PlotLinesHovered]       = ImVec4(1.00f, 0.43f, 0.35f, 1.00f);
    colors[ImGuiCol_TextSelectedBg]         = ImVec4(0.21f, 0.47f, 0.76f, 0.35f);
    colors[ImGuiCol_NavHighlight]           = ImVec4(0.21f, 0.47f, 0.76f, 1.00f);

    // DND borders
    colors[ImGuiCol_DragDropTarget]         = ImVec4(1.0f, 0.0f, 0.3f, 1.00f);
}

void VK_GUI::GUI::draw_model_list_overlay(VK_INIT_ENGINE::_inited_engine& _init, ModelManager& _modelManager,
    std::unique_ptr<Scene>& _scene, const GPUSceneData& sceneData, PipelineManager& pipelineManager){
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Drag and drop
    static bool shouldSpawnDroppedEntity = false;
    static uint32_t droppedModelIdToSpawn = 0;
    static int entityCounter = 0;

    // Сюда будем сохранять координаты мыши ДО того, как они сбросятся в 0
    static ImVec2 dropMousePos = ImVec2(0.0f, 0.0f);

    ImGuiViewport* ImViewport = ImGui::GetMainViewport();
    ImGuiWindowFlags bgFlags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
                               ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoSavedSettings |
                               ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoBringToFrontOnFocus;

    if (ImGui::GetDragDropPayload() != nullptr) {
        // Если что-то тащат, окно активно для мыши и мы постоянно записываем позицию
        bgFlags &= ~ImGuiWindowFlags_NoInputs; // На всякий случай явно убираем блокировку ввода
        dropMousePos = ImGui::GetMousePos();
    } else {
        bgFlags |= ImGuiWindowFlags_NoInputs;
    }

    ImGui::SetNextWindowPos(ImViewport->WorkPos);
    ImGui::SetNextWindowSize(ImViewport->WorkSize);

    ImGui::Begin("BackgroundDropZone", nullptr, bgFlags);

    ImGui::Dummy(ImGui::GetContentRegionAvail());

    if (ImGui::BeginDragDropTarget()) {
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("DND_MODEL_ID")) {
            droppedModelIdToSpawn = *(const uint32_t*)payload->Data;
            shouldSpawnDroppedEntity = true;

            // Если в этот кадр GetMousePos() уже вернул 0, мы оставляем dropMousePos нетронутым (из предыдущего кадра)
            if (ImGui::GetMousePos().x != 0.0f || ImGui::GetMousePos().y != 0.0f) {
                dropMousePos = ImGui::GetMousePos();
            }
        }
        ImGui::EndDragDropTarget();
    }
    ImGui::End();
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Window manager logic

    ImGuiViewport* viewport = ImGui::GetMainViewport();
    float screenWidth = viewport->WorkSize.x;

    float menuBarHeight = ImGui::GetFrameHeight();

    float windowWidth = 320.0f;

    ImVec2 finalPos = ImVec2(screenWidth - windowWidth, menuBarHeight);

    ImGui::SetNextWindowPos(finalPos, ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(windowWidth, 0.0f), ImGuiCond_Always);
    ImGui::SetNextWindowBgAlpha(1.0f);

    ImGuiWindowFlags flags = ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoCollapse;

    bool triggerFileDialog = false;
    static int contextMenuModelId = -1;
    bool openModelPopup = false;

    if (ImGui::Begin("ModelManager", nullptr, flags)) {

        ImGui::Text("All models in memory: %u", _modelManager.CountOfModels());

        float spacing = ImGui::GetStyle().ItemSpacing.x;
        float halfButtonWidth = (300.0f - spacing) / 2.0f;

        if (ImGui::Button("Destroy DM", ImVec2(halfButtonWidth, 0.0f))) {
            _scene->DestroyAllDynamicEntites();
            _modelManager.destroy_dynamic_models();
        }
        ImGui::SameLine();
        if (ImGui::Button("Destroy AM", ImVec2(halfButtonWidth, 0.0f))) {
            _scene->DestroyAllEntites();
            _modelManager.destroy_all();
        }

        if (ImGui::Button("Recompile shaders", ImVec2(300.0f, 0.0f))) {
            pipelineManager.ReloadAllPipelines();
        }

        ImGui::Separator();
        ImGui::TextDisabled("Loaded models (Drag to Viewport / Right-Click):");

        ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.11f, 0.11f, 0.11f, 1.00f));

        if (ImGui::BeginChild("ModelListArea", ImVec2(300.0f, 350.0f), ImGuiChildFlags_Borders))  {

            if (_modelManager.empty()) {
                ImGui::SetCursorPos(ImVec2(10, 10));
                ImGui::TextDisabled("No loaded models");
            } else {
                auto& models = _modelManager.GetModels();

                for (uint32_t i = 0; i < models.size(); ++i) {
                    Model& model = models[i];
                    if (!model.bIsValid) continue;

                    ImGui::PushID(i);

                    std::string lifetimeStr = (model.lifetime == ModelLifetime::Static) ? "Static" : "Dynamic";
                    std::string label = "Model [" + std::to_string(i) + "] (" + lifetimeStr + ")";

                    bool isSelected = (selectedModelId == static_cast<int>(i));
                    if (ImGui::Selectable(label.c_str(), isSelected, ImGuiSelectableFlags_AllowOverlap)) {
                        selectedModelId = i;
                    }

                    if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None)) {
                        uint32_t modelIdPayload = i;

                        ImGui::SetDragDropPayload("DND_MODEL_ID", &modelIdPayload, sizeof(uint32_t));
                        ImGui::Text("Spawning: Model [%u]", i);

                        ImGui::EndDragDropSource();
                    }

                    if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
                        contextMenuModelId = static_cast<int>(i);
                        openModelPopup = true;
                    }

                    ImGui::PopID();
                }
            }

            if (openModelPopup) {
                ImGui::OpenPopup("ModelContextMenu");
            }

            // ПРАВИЛО: EndPopup() вызывается ТОЛЬКО если BeginPopup вернул true
            if (ImGui::BeginPopup("ModelContextMenu")) {
                auto& models = _modelManager.GetModels();

                if (contextMenuModelId >= 0 && contextMenuModelId < static_cast<int>(models.size())) {
                    Model& model = models[contextMenuModelId];

                    if (ImGui::MenuItem("Inspector")) {
                        selectedModelId = contextMenuModelId;
                        showInspector = true;
                        contextMenuModelId = -1;
                    }

                    ImGui::Separator();

                    bool isStatic = (model.lifetime == ModelLifetime::Static);
                    if (isStatic) ImGui::BeginDisabled();

                    if (ImGui::MenuItem("Delete")) {
                        _scene->DestroyEntitiesByModel(contextMenuModelId);
                        _modelManager.destroy_model(contextMenuModelId);

                        if (selectedModelId == contextMenuModelId) {
                            selectedModelId = -1;
                        }
                        contextMenuModelId = -1;
                    }

                    if (isStatic) ImGui::EndDisabled();
                }
                ImGui::EndPopup();
            }

            if (ImGui::IsWindowHovered(ImGuiHoveredFlags_ChildWindows) &&
                ImGui::IsMouseClicked(ImGuiMouseButton_Right) &&
                !ImGui::IsAnyItemHovered())
            {
                ImGui::OpenPopup("EmptySpaceContextMenu");
            }

            if (ImGui::BeginPopup("EmptySpaceContextMenu")) {
                if (ImGui::MenuItem("Load model")) {
                    triggerFileDialog = true;
                }
                ImGui::EndPopup();
            }

            ImGui::EndChild(); // Закрываем ChildArea, так как мы вошли в условие BeginChild
        }
        else {
        }

        ImGui::PopStyleColor();
    }
    ImGui::End();

    static bool delayedTrigger = false;
    if (triggerFileDialog) {
        delayedTrigger = true;
    }
    else if (delayedTrigger) {
        delayedTrigger = false;

        std::string path = UTILS::OpenModelDialog();
        if (!path.empty()) {
            _modelManager.LoadModel(path, _confDynamic.lifetime, _confDynamic.useArena);
        }
    }

    // DND Ray casting
   if (shouldSpawnDroppedEntity) {
        shouldSpawnDroppedEntity = false;

        ImVec2 viewportSize = ImViewport->WorkSize;

        Ray ray = Ray::FromScreen(
            dropMousePos.x,
            dropMousePos.y,
            viewportSize.x,
            viewportSize.y,
            sceneData
        );

        RaycastHit hitResult = _scene->Raycast(ray);

        glm::vec3 spawnPosition = glm::vec3(0.0f);

        if (hitResult.hit) {
            spawnPosition = ray.GetPoint(hitResult.distance);
        } else {
            const glm::vec3& dir = ray.GetDirection();
            const glm::vec3& orig = ray.GetOrigin();

            if (glm::abs(dir.z) > 0.0001f) {
                float t = (0.0f - orig.z) / dir.z;

                if (t >= 0.0f && !std::isnan(t) && !std::isinf(t)) {
                    spawnPosition = ray.GetPoint(t);
                } else {
                    spawnPosition = orig + dir * 10.0f;
                }
            } else {
                spawnPosition = orig + dir * 10.0f;
            }
        }

        if (std::isnan(spawnPosition.x) || std::isnan(spawnPosition.y) || std::isnan(spawnPosition.z)) {
            spawnPosition = glm::vec3(0.0f, 0.0f, 0.0f);
        }

        // Создаем сущность
        std::string entityName = "Entity_Model_" + std::to_string(droppedModelIdToSpawn) + "_" + std::to_string(entityCounter++);
        GameEntity* newEntity = _scene->CreateEntity(entityName, droppedModelIdToSpawn);

        if (newEntity) {
            fmt::print("[Safe Spawn] Success! Created entity: {} at pos ({:.2f}, {:.2f}, {:.2f})\n",
                entityName, spawnPosition.x, spawnPosition.y, spawnPosition.z);

            newEntity->position = spawnPosition;
            newEntity->rotation = glm::vec3(0.0f, 0.0f, 0.0f);
            newEntity->scale    = glm::vec3(1.0f, 1.0f, 1.0f);
        }
    }
}

void VK_GUI::GUI::draw_fps_overlay(VK_INIT_ENGINE::_inited_engine& _init, CONTROLLER::Delta _delta){
    float fps = (_delta.delta > 0.00001f && std::isfinite(_delta.delta)) ? (1.0f / _delta.delta) : 0.0f;
    static float smoothedFps = 60.0f;

    if (std::isfinite(fps) && fps > 0.0f) {
        smoothedFps = glm::mix(smoothedFps, fps, 0.05f);
    }

    // Флаги полной невидимости окна
    ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoDecoration |
                                   ImGuiWindowFlags_AlwaysAutoResize |
                                   ImGuiWindowFlags_NoSavedSettings |
                                   ImGuiWindowFlags_NoFocusOnAppearing |
                                   ImGuiWindowFlags_NoNav |
                                   ImGuiWindowFlags_NoMove |
                                   ImGuiWindowFlags_NoBackground |
                                   ImGuiWindowFlags_NoInputs;

    ImGuiViewport* viewport = ImGui::GetMainViewport();
    float screenWidth = viewport->WorkSize.x;
    float menuBarHeight = ImGui::GetFrameHeight();

    float paddingX = 10.0f;
    float paddingY = 5.0f;

    float posX = screenWidth - paddingX;
    float posY = menuBarHeight + paddingY;

    ImGui::SetNextWindowPos(ImVec2(posX, posY), ImGuiCond_Always, ImVec2(1.0f, 0.0f));

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));

    if (ImGui::Begin("##FPS_Overlay", nullptr, windowFlags)) {
        ImGui::TextColored(ImVec4(0.2f, 0.8f, 0.2f, 1.0f), "%.0f FPS", smoothedFps);
    }
    ImGui::End();

    ImGui::PopStyleVar(); // Возвращаем отступы назад
}

void VK_GUI::GUI::draw_context_menu_trs(VK_INIT_ENGINE::_inited_engine& _init, std::unique_ptr<Scene>& _scene,
    const GPUSceneData& sceneData){
    if (ImGui::IsMouseClicked(ImGuiMouseButton_Right) &&
    !ImGui::GetIO().WantCaptureMouse &&
    !ImGuizmo::IsOver())  {

        ImVec2 mousePos = ImGui::GetMousePos();

        float screenWidth  = static_cast<float>(_init._windowExtent.width);
        float screenHeight = static_cast<float>(_init._windowExtent.height);

        // Строим луч и пускаем в сцену
        Ray ray = Ray::FromScreen(mousePos.x, mousePos.y, screenWidth, screenHeight, sceneData);

        RaycastHit hit = _scene->Raycast(ray);

        if (hit.hit && hit.entity != nullptr) {
            selectedEntityId = static_cast<int>(hit.entity->id);
            mouseClickPos = mousePos; // Запоминаем координаты курсора
            showContextMenu = true;

            ImGui::OpenPopup("ModelContextMenu");
        } else {
            selectedEntityId = -1;
            showContextMenu = false;
        }
    }

    static bool showTrsWindow = false;

    ImGui::SetNextWindowPos(mouseClickPos, ImGuiCond_Appearing);

    if (ImGui::BeginPopup("ModelContextMenu")) {
        GameEntity* entity = _scene->GetEntity(static_cast<uint32_t>(selectedEntityId));

        if (entity) {
            ImGui::TextDisabled("Entity ID: %d", selectedEntityId);
            ImGui::Separator();

            if (ImGui::MenuItem("TRS")) {
                showTrsWindow = true;
                showContextMenu = false;
                ImGui::CloseCurrentPopup();
            }

            if (ImGui::MenuItem("Delete")) {
                _scene->DestroyEntity(selectedEntityId);
                selectedEntityId = -1;
                ImGui::CloseCurrentPopup();
            }
        }
        ImGui::EndPopup();
    }

    if (showTrsWindow) {
        ImGui::SetNextWindowSize(ImVec2(350, 400), ImGuiCond_FirstUseEver);

        if (ImGui::Begin("TRS Properties", &showTrsWindow)) {
            GameEntity* entity = _scene->GetEntity(static_cast<uint32_t>(selectedEntityId));

            if (entity) {
                ImGui::Text("Editing Entity ID: %d", selectedEntityId);
                ImGui::Separator();

                auto& position = entity->position;
                auto& rotation = entity->rotation;
                auto& scale    = entity->scale;
                
                float itemWidth = ImGui::GetContentRegionAvail().x * 0.6f;

                // Position
                if (ImGui::TreeNodeEx("Transform", ImGuiTreeNodeFlags_DefaultOpen)) {

                    ImGui::PushItemWidth(itemWidth);

                    // Location X, Y, Z
                    ImGui::Text("Location X"); ImGui::SameLine(120);
                    ImGui::DragFloat("##LocX", &position.x, 0.05f, 0.0f, 0.0f, "%.3f m");

                    ImGui::Text("         Y"); ImGui::SameLine(120);
                    ImGui::DragFloat("##LocY", &position.y, 0.05f, 0.0f, 0.0f, "%.3f m");

                    ImGui::Text("         Z"); ImGui::SameLine(120);
                    ImGui::DragFloat("##LocZ", &position.z, 0.05f, 0.0f, 0.0f, "%.3f m");

                    ImGui::Spacing();

                    // Rotation X, Y, Z (Euler angles)
                    static glm::vec3 cachedUIAngles(0.0f);
                    static uint32_t lastEntityId = -1;

                    // Схемка такая в интерфейсе юзаем углы эйлера а когда они начнут клинить бахаем квантерионы
                    if (selectedEntityId != lastEntityId || (!ImGui::IsItemActive() && !ImGui::IsAnyItemActive())) {
                        cachedUIAngles = glm::degrees(glm::eulerAngles(rotation));
                        lastEntityId = selectedEntityId;
                    }

                    bool isRotationChanged = false;

                    ImGui::Text("Rotation X"); ImGui::SameLine(120);
                    if (ImGui::DragFloat("##RotX", &cachedUIAngles.x, 0.5f, -360.0f, 360.0f, "%.1f°")) {
                        isRotationChanged = true;
                    }

                    ImGui::Text("         Y"); ImGui::SameLine(120);
                    if (ImGui::DragFloat("##RotY", &cachedUIAngles.y, 0.5f, -360.0f, 360.0f, "%.1f°")) {
                        isRotationChanged = true;
                    }

                    ImGui::Text("         Z"); ImGui::SameLine(120);
                    if (ImGui::DragFloat("##RotZ", &cachedUIAngles.z, 0.5f, -360.0f, 360.0f, "%.1f°")) {
                        isRotationChanged = true;
                    }

                    if (isRotationChanged) {
                        rotation = glm::quat(glm::radians(cachedUIAngles));
                    }

                    ImGui::Spacing();

                    // Scale X, Y, Z
                    // Для масштаба ставим минимальный лимит 0.001f, чтобы объект не схлопнулся
                    ImGui::Text("Scale    X"); ImGui::SameLine(120);
                    ImGui::DragFloat("##ScaleX", &scale.x, 0.01f, 0.001f, 1000.0f, "%.3f");

                    ImGui::Text("         Y"); ImGui::SameLine(120);
                    ImGui::DragFloat("##ScaleY", &scale.y, 0.01f, 0.001f, 1000.0f, "%.3f");

                    ImGui::Text("         Z"); ImGui::SameLine(120);
                    ImGui::DragFloat("##ScaleZ", &scale.z, 0.01f, 0.001f, 1000.0f, "%.3f");

                    ImGui::PopItemWidth();
                    ImGui::TreePop();
                }

                // General scale
                ImGui::Spacing();
                ImGui::Separator();
                ImGui::Spacing();

                ImGui::Text("Uniform Scale");
                ImGui::SameLine(120);
                ImGui::PushItemWidth(itemWidth);

                float uniformScale = scale.x;
                if (ImGui::DragFloat("##UniformScale", &uniformScale, 0.01f, 0.001f, 1000.0f, "Multiplier: %.3f")) {
                    scale.x = uniformScale;
                    scale.y = uniformScale;
                    scale.z = uniformScale;
                }
                ImGui::PopItemWidth();

            } else {
                ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Error: Entity not found!");
            }
        }
        ImGui::End();
    }
}

void VK_GUI::GUI::draw_gizmo(VK_INIT_ENGINE::_inited_engine& _init, std::unique_ptr<Scene>& _scene,
    const GPUSceneData& sceneData, ModelManager& _modelManager)
{
    if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) &&
        !ImGui::GetIO().WantCaptureMouse &&
        !ImGuizmo::IsOver())
    {
        ImVec2 mousePos = ImGui::GetMousePos();
        float screenWidth  = static_cast<float>(_init._windowExtent.width);
        float screenHeight = static_cast<float>(_init._windowExtent.height);

        Ray ray = Ray::FromScreen(mousePos.x, mousePos.y, screenWidth, screenHeight, sceneData);
        RaycastHit hit = _scene->Raycast(ray);

        if (hit.hit && hit.entity != nullptr) {
            selectedEntityId = static_cast<int>(hit.entity->id);
            showTrsWindow = true;
        } else {
            selectedEntityId = -1;
            showTrsWindow = false;
        }
    }

    if (selectedEntityId == -1 || !showTrsWindow) return;

    GameEntity* entity = _scene->GetEntity(static_cast<uint32_t>(selectedEntityId));
    if (entity == nullptr) return;

    ImGuiIO& io = ImGui::GetIO();
    ImGuizmo::SetOrthographic(false);
    ImGuizmo::SetRect(0.0f, 0.0f, io.DisplaySize.x, io.DisplaySize.y);

    glm::mat4 viewMatrix = sceneData.view;
    glm::mat4 projMatrix = sceneData.proj;

    glm::vec3 pivotOffset = glm::vec3(0.0f);

    if (projMatrix[1][1] < 0.0f) {
        projMatrix[1][1] *= -1.0f;
    }

    auto box = entity->GetWorldAABB(_modelManager);
    glm::vec3 aabbSize = box.max - box.min;
    float objectSizeFactor = (aabbSize.x + aabbSize.y + aabbSize.z) / 3.0f;
    glm::vec3 aabbCenter = (box.min + box.max) * 0.5f;
    pivotOffset = aabbCenter - entity->position;
    if (objectSizeFactor < 0.001f) { objectSizeFactor = 1.0f; }
    if (currentGizmoOperation == ImGuizmo::ROTATE){
        IMGUIZMO_FIX::SetCustomAABBSize(objectSizeFactor/1.5f);
    }
    else{
        IMGUIZMO_FIX::SetCustomAABBSize(objectSizeFactor);
    }

    // Блокировка Gizmo от большого расстояния
    glm::mat4 invView = glm::inverse(sceneData.view);
    glm::vec3 cameraPos = glm::vec3(invView[3]);

    glm::vec3 finalPos = entity->position + pivotOffset;
    float distanceToTarget = glm::distance(cameraPos, finalPos);

    float fovY = glm::radians(70.0f);
    float screenHeight = static_cast<float>(_init._windowExtent.height);

    float gizmoPixelSize = (objectSizeFactor / (distanceToTarget * glm::tan(fovY * 0.5f))) * screenHeight;

    bool canInteract = (gizmoPixelSize > 100.0f);

    if (ImGuizmo::IsUsing())
    {
        canInteract = true;
    }

    IMGUIZMO_FIX::SetAllowInteraction(canInteract);

    glm::mat4 modelMatrix = glm::mat4(1.0f);
    modelMatrix = glm::translate(modelMatrix, finalPos);
    glm::mat4 rotationMatrix = glm::mat4_cast(entity->rotation);
    modelMatrix = modelMatrix * rotationMatrix;
    modelMatrix = glm::scale(modelMatrix, entity->scale);

    ImGuizmo::GetStyle().RotationLineThickness = 3.0f;

    // Манипуляция ImGuizmo
    if (canInteract)
    {
        // Возвращаем нормальный размер окна ImGuizmo
        ImGuizmo::SetRect(0.0f, 0.0f, io.DisplaySize.x, io.DisplaySize.y);

        // Вызываем манипулятор ТОЛЬКО когда объект достаточно близко
        ImGuizmo::Manipulate(
            glm::value_ptr(viewMatrix),
            glm::value_ptr(projMatrix),
            currentGizmoOperation,
            currentGizmoMode,
            glm::value_ptr(modelMatrix)
        );
    }
    else
    {
        // Прячем гизмо от мышки на всякий случай
        ImGuizmo::SetRect(0.0f, 0.0f, 0.0f, 0.0f);
    }

    static bool wasGizmoUsingLastFrame = false;
    static float lastMouseAngle = 0.0f;

    static glm::vec3 startEntityPos{0.0f};
    static ImVec2 startMousePos = {0.0f, 0.0f};

    if (ImGuizmo::IsUsing())
    {
        if (!wasGizmoUsingLastFrame)
        {
            wasGizmoUsingLastFrame = true;

            ImVec2 mousePos = ImGui::GetMousePos();
            startMousePos = mousePos;
            startEntityPos = entity->position;

            if (currentGizmoOperation == ImGuizmo::ROTATE) {
                ImVec2 gizmoScreenPos = ImVec2(io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.5f);
                lastMouseAngle = atan2f(mousePos.y - gizmoScreenPos.y, mousePos.x - gizmoScreenPos.x);
            }
            return;
        }

        // ЕСЛИ МЫ КРУТИМ: включаем бережный режим Blender
        if (currentGizmoOperation == ImGuizmo::ROTATE)
        {
            ImVec2 mousePos = ImGui::GetMousePos();
            ImVec2 gizmoScreenPos = ImVec2(io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.5f);

            float currentMouseAngle = atan2f(mousePos.y - gizmoScreenPos.y, mousePos.x - gizmoScreenPos.x);
            float deltaAngle = currentMouseAngle - lastMouseAngle;

            // Нормализуем дельту, чтобы не было прыжков при переходе через Pi / -Pi
            if (deltaAngle > 3.14159f) { deltaAngle -= 2.0f * 3.14159f; }
            if (deltaAngle < -3.14159f) { deltaAngle += 2.0f * 3.14159f; }

            float distToCenter = sqrtf(powf(mousePos.x - gizmoScreenPos.x, 2) + powf(mousePos.y - gizmoScreenPos.y, 2));

            if (distToCenter < 100.0f) { deltaAngle *= (distToCenter / 100.0f); }
            int activeAxis = IMGUIZMO_FIX::GetCurrentRotateAxis();
            glm::vec3 targetAxis = glm::vec3(0.0f, 0.0f, 1.0f);

            if (activeAxis == 0) { targetAxis = glm::vec3(1.0f, 0.0f, 0.0f); }
            if (activeAxis == 1) { targetAxis = glm::vec3(0.0f, 1.0f, 0.0f); }
            if (activeAxis == 2) { targetAxis = glm::vec3(0.0f, 0.0f, 1.0f); }
            if (activeAxis == 3) { targetAxis = -glm::normalize(glm::vec3(invView[2])); }

            if (currentGizmoMode == ImGuizmo::LOCAL && activeAxis != 3){
                targetAxis = glm::normalize(glm::vec3(glm::toMat4(entity->rotation) * glm::vec4(targetAxis, 0.0f)));
            }

            float axisSign = 1.0f;
            if (activeAxis == 0) { axisSign = 1.0f; }
            if (activeAxis == 1) { axisSign = 1.0f; }
            if (activeAxis == 2) { axisSign = 1.0f; }
            if (activeAxis == 3) { axisSign = 1.0f; }

            if (activeAxis != 3)
            {
                glm::vec3 cameraLookDir = -glm::normalize(glm::vec3(invView[2]));
                glm::vec3 worldBaseAxis = glm::vec3(0.0f, 0.0f, 1.0f);
                if (activeAxis == 0) { worldBaseAxis = glm::vec3(1.0f, 0.0f, 0.0f); }
                if (activeAxis == 1) { worldBaseAxis = glm::vec3(0.0f, 1.0f, 0.0f); }
                if (activeAxis == 2) { worldBaseAxis = glm::vec3(0.0f, 0.0f, 1.0f); }

                float dotResult = glm::dot(worldBaseAxis, cameraLookDir);
                if (dotResult < 0.0f) { axisSign = -axisSign; }
            }
            glm::quat deltaRotation = glm::angleAxis(deltaAngle * axisSign, targetAxis);
            entity->rotation = deltaRotation * entity->rotation;
            entity->rotation = glm::normalize(entity->rotation);
            lastMouseAngle = currentMouseAngle;
        }
        else
        {
            glm::vec3 skew; glm::vec4 perspective; glm::vec3 translation; glm::quat rotationResult; glm::vec3 scale;
            glm::decompose(modelMatrix, scale, rotationResult, translation, skew, perspective);

            // Возвращаем позицию, просто вычитая pivotOffset
            entity->position.x = translation.x - pivotOffset.x;
            entity->position.y = translation.y - pivotOffset.y;
            entity->position.z = translation.z - pivotOffset.z;

            entity->scale = scale;
        }
    }
    else
    {
        wasGizmoUsingLastFrame = false;
    }
}

void VK_GUI::GUI::draw_view_navigation_widget(const GPUSceneData& sceneData, CONTROLLER::Camera& _camera){
    ImGuiIO& io = ImGui::GetIO();

    glm::mat4 viewMatrix = sceneData.view;

    float widgetWidth = 100.0f;
    float widgetHeight = 100.0f;

    float posX = io.DisplaySize.x - 320.0f - widgetWidth - 20.0f;
    float posY = 40.0f; // Ниже главного меню

    ImGuizmo::SetRect(0.0f, 0.0f, io.DisplaySize.x, io.DisplaySize.y);

    float length = 4.0f;
    ImGuizmo::ViewManipulate(
        glm::value_ptr(viewMatrix),
        length,
        ImVec2(posX, posY),
        ImVec2(widgetWidth, widgetHeight),
        0x00000000
    );

    if (viewMatrix != sceneData.view)
    {
        glm::mat4 invView = glm::inverse(viewMatrix);
        glm::quat newCameraRotation = glm::quat_cast(invView);
    }
}

void VK_GUI::GUI::gizmo_mode(){
    if (!ImGui::GetIO().WantTextInput) {
        if (ImGui::IsKeyPressed(ImGuiKey_F)) {
            if (currentGizmoOperation == ImGuizmo::TRANSLATE) {
                currentGizmoOperation = ImGuizmo::ROTATE;
            }
            else if (currentGizmoOperation == ImGuizmo::ROTATE) {
                currentGizmoOperation = ImGuizmo::SCALE;
            }
            else if (currentGizmoOperation == ImGuizmo::SCALE) {
                currentGizmoOperation = ImGuizmo::TRANSLATE;
            }
        }
    }
}

void VK_GUI::GUI::draw_main_menu_bar(CONTROLLER::Delta& _delta){
    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("New Scene", "Ctrl+N")) {}
            if (ImGui::MenuItem("Open...", "Ctrl+O")) {}
            ImGui::Separator();
            if (ImGui::MenuItem("Exit", "Alt+F4")) {}
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Setting")) {
            static bool vsync = true;
            if (ImGui::Checkbox("VSync", &vsync)) {}
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Tools")) {
            if (ImGui::MenuItem("Metrics/Debugger")) {}
            ImGui::EndMenu();
        }

        float fps = (_delta.delta > 0.00001f && std::isfinite(_delta.delta)) ? (1.0f / _delta.delta) : 0.0f;
        static float smoothedFps = 60.0f;
        if (std::isfinite(fps) && fps > 0.0f) {
            smoothedFps = glm::mix(smoothedFps, fps, 0.05f);
        }

        char fpsBuffer[32];
        snprintf(fpsBuffer, sizeof(fpsBuffer), "%.0f FPS", smoothedFps);

        float rightPadding = 15.0f;
        float posX = ImGui::GetWindowWidth() - ImGui::CalcTextSize(fpsBuffer).x - rightPadding;

        ImGui::SameLine(posX);

        ImGui::TextColored(ImVec4(0.4f, 0.8f, 0.4f, 1.0f), "%s", fpsBuffer);

        ImGui::EndMainMenuBar();
    }
}

void VK_GUI::GUI::draw_imgui(VK_INIT_ENGINE::_inited_engine& _init, VkCommandBuffer cmd, VkExtent2D _drawExtent){
    VkRenderingAttachmentInfo colorAttachment = vkinit::attachment_info(_init._drawImage.imageView, nullptr, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;

    VkRenderingInfo renderInfo = vkinit::rendering_info(_drawExtent, &colorAttachment, nullptr);
    vkCmdBeginRendering(cmd, &renderInfo);

    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cmd);

    vkCmdEndRendering(cmd);
}

void VK_GUI::GUI::update_imgui(VK_INIT_ENGINE::_inited_engine& _init, CONTROLLER::Delta& _delta, CONTROLLER::Camera _camera, ModelManager& _modelManager,
    std::unique_ptr<Scene>& _scene, const GPUSceneData& sceneData, PipelineManager& pipelineManager){
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplSDL3_NewFrame();
    ImGui::NewFrame();
    ImGuizmo::BeginFrame();

    draw_main_menu_bar(_delta);

    gizmo_mode();

    draw_model_list_overlay(_init, _modelManager, _scene, sceneData, pipelineManager);

    draw_inspector_window(_init, _modelManager);

    draw_context_menu_trs(_init, _scene, sceneData);

    draw_gizmo(_init, _scene, sceneData, _modelManager);

    draw_view_navigation_widget(sceneData, _camera);

    ImGui::Render();
}

void VK_GUI::GUI::draw_inspector_window(VK_INIT_ENGINE::_inited_engine& _init, ModelManager& modelManager){
    if (!showInspector || selectedModelId == -1) {
        return;
    }

    if (!modelManager.has_model(selectedModelId) || !modelManager.GetModel(selectedModelId).bIsValid) {
        selectedModelId = -1;
        showInspector = false;
        return;
    }

    Model& model = modelManager.GetModel(selectedModelId);

    ImGuiWindowFlags flags = ImGuiWindowFlags_NoSavedSettings |
                             ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize;

    // TODO: Настройка изначальной позиции панели
    float posX = 600;
    float posY = 400;
    float initialWidth = 320.0f;
    float initialHeight = 300.0f;

    ImGui::SetNextWindowPos(ImVec2(posX, posY), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(initialWidth, initialHeight), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowBgAlpha(1.0f);

    std::string title = "Inspector: Model [" + std::to_string(selectedModelId) + "]";

    if (ImGui::Begin(title.c_str(), &showInspector, flags)) {

        ImGui::TextDisabled("Asset Info:");

        if (ImGui::BeginTable("AssetProperties", 2, ImGuiTableFlags_BordersInnerH)) {
            ImGui::TableSetupColumn("Property", ImGuiTableColumnFlags_WidthFixed, 100.0f);
            ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch);

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0); ImGui::Text("Lifetime");
            ImGui::TableSetColumnIndex(1); ImGui::Text("%s", (model.lifetime == ModelLifetime::Static) ? "Static" : "Dynamic");

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0); ImGui::Text("Status");
            ImGui::TableSetColumnIndex(1); ImGui::Text(model.bIsValid ? "Valid / Loaded" : "Invalid");

            ImGui::EndTable();
        }

        ImGui::Dummy(ImVec2(0.0f, 6.0f));
        ImGui::Separator();
        ImGui::Dummy(ImVec2(0.0f, 6.0f));

        if (ImGui::CollapsingHeader("Meshes & Nodes")) {
            ImGui::Text("Total Meshes: %zu", model.Meshes.size());
            ImGui::Text("Mesh Nodes: %zu", model.meshNodes.size());

            ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.11f, 0.11f, 0.11f, 1.00f));
            if (ImGui::BeginChild("MeshListSubArea", ImVec2(0.0f, 80.0f), true, 0)) {
                for (size_t m = 0; m < model.Meshes.size(); ++m) {
                    ImGui::Text("  • Mesh Sub-Asset ID: %zu", m);
                }
            }
            ImGui::EndChild();
            ImGui::PopStyleColor();
        }

        ImGui::Dummy(ImVec2(0.0f, 4.0f));

        if (ImGui::CollapsingHeader("GPU Textures")) {
            ImGui::Text("Loaded Textures: %zu", model.loadedTextures.size());

            ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.11f, 0.11f, 0.11f, 1.00f));

            if (ImGui::BeginChild("TextureListSubArea", ImVec2(0.0f, 200.0f), true, 0)) {
                for (size_t t = 0; t < model.loadedTextures.size(); ++t) {

                    ImGui::PushID(static_cast<int>(t));
                    VkDescriptorSet textureSet = model.loadedTextures[t].imguiDescriptorSet;

                    if (textureSet != VK_NULL_HANDLE) {
                        ImGui::Image((ImTextureID)textureSet, ImVec2(64.0f, 64.0f));
                        ImGui::SameLine();
                    }

                    ImGui::BeginGroup();
                    ImGui::Text("Bindless Global ID: %u", model.loadedTextures[t].globalIndex);
                    ImGui::TextDisabled("VkImageView: 0x%p", (void*)model.loadedTextures[t].image.imageView);
                    ImGui::EndGroup();

                    ImGui::Dummy(ImVec2(0.0f, 4.0f));
                    ImGui::Separator();

                    ImGui::PopID();
                }
            }
            ImGui::EndChild();
            ImGui::PopStyleColor();
        }
    }
    ImGui::End();

    if (!showInspector) {
        selectedModelId = -1;
    }
}
