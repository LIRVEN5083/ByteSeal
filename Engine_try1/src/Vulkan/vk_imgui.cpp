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
    //colors[ImGuiCol_DockingPreview]         = ImVec4(0.21f, 0.47f, 0.76f, 0.70f);
    //colors[ImGuiCol_DockingEmptyBg]         = ImVec4(0.10f, 0.10f, 0.10f, 1.00f);
    colors[ImGuiCol_PlotLines]              = ImVec4(0.61f, 0.61f, 0.61f, 1.00f);
    colors[ImGuiCol_PlotLinesHovered]       = ImVec4(1.00f, 0.43f, 0.35f, 1.00f);
    colors[ImGuiCol_TextSelectedBg]         = ImVec4(0.21f, 0.47f, 0.76f, 0.35f);
    colors[ImGuiCol_NavHighlight]           = ImVec4(0.21f, 0.47f, 0.76f, 1.00f);
}

void VK_GUI::GUI::draw_model_list_overlay(VK_INIT_ENGINE::_inited_engine& _init, ModelManager& _modelManager, std::unique_ptr<Scene>& _scene){
    ImGuiWindowFlags flags = ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoCollapse;


    ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f), ImGuiCond_Always);
    ImGui::SetNextWindowBgAlpha(1.0f);

    if (ImGui::Begin("ModelManager", nullptr, flags)) {

        ImGui::Text("All models in memory: %u", _modelManager.CountOfModels());

        if (ImGui::Button("Destroy dynamic models")) {
            _scene->DestroyAllDynamicEntites();
            _modelManager.destroy_dynamic_models();
        }
        ImGui::SameLine();
        if (ImGui::Button("Destroy all models")) {
            _scene->DestroyAllEntites();
            _modelManager.destroy_all();
        }

        ImGui::Separator();
        ImGui::TextDisabled("Loaded models (Right-Click for options):");

        ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.11f, 0.11f, 0.11f, 1.00f));

        if (ImGui::BeginChild("ModelListArea", ImVec2(300.0f, 350.0f), true, 0)) {

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

                    // Selectable теперь занимает всю ширину строки, кнопка больше не мешает
                    bool isSelected = (selectedModelId == static_cast<int>(i));
                    if (ImGui::Selectable(label.c_str(), isSelected)) {
                        selectedModelId = i;
                    }

                    // ЕДИНЫЙ КОНТЕКСТНЫЙ МЕНЮ ДЛЯ МОДЕЛИ ПО ПКМ
                    bool shouldBreak = false;
                    if (ImGui::BeginPopupContextItem("ModelContextMenu")) {

                        if (ImGui::MenuItem("Inspector")) {
                            selectedModelId = i;
                            showInspector = true;
                        }

                        ImGui::Separator(); // Небольшая черта между опциями

                        // Защита удаления статической модели прямо в контекстном меню
                        bool isStatic = (model.lifetime == ModelLifetime::Static);
                        if (isStatic) ImGui::BeginDisabled();

                        if (ImGui::MenuItem("Delete")) {
                            _scene->DestroyEntitiesByModel(i);
                            _modelManager.destroy_model(i);

                            if (selectedModelId == static_cast<int>(i)) {
                                selectedModelId = -1;
                            }
                            shouldBreak = true; // Помечаем, что нужно прервать цикл
                        }

                        if (isStatic) ImGui::EndDisabled();

                        ImGui::EndPopup();
                    }

                    ImGui::PopID();

                    if (shouldBreak) {
                        break; // Выходим из цикла отрисовки моделей, так как вектор изменился
                    }
                }
            }
        }
        ImGui::EndChild();
        ImGui::PopStyleColor();
    }
    ImGui::End();
}

void VK_GUI::GUI::draw_fps_overlay(VK_INIT_ENGINE::_inited_engine& _init, CONTROLLER::Delta _delta){
    float fps = (_delta.delta > 0.00001f && std::isfinite(_delta.delta)) ? (1.0f / _delta.delta) : 0.0f;

    static float smoothedFps = 60.0f;

    if (std::isfinite(fps) && fps > 0.0f) {
        smoothedFps = glm::mix(smoothedFps, fps, 0.05f);
    }

    ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoDecoration |
                                   ImGuiWindowFlags_AlwaysAutoResize |
                                   ImGuiWindowFlags_NoSavedSettings |
                                   ImGuiWindowFlags_NoFocusOnAppearing |
                                   ImGuiWindowFlags_NoNav |
                                   ImGuiWindowFlags_NoMove;

    int windowWidth = 0;
    int windowHeight = 0;
    SDL_GetWindowSize(_init._window, &windowWidth, &windowHeight);

    float padding = 10.0f;
    float posX = static_cast<float>(windowWidth) - padding;
    float posY = padding;

    ImGui::SetNextWindowPos(ImVec2(posX, posY), ImGuiCond_Always, ImVec2(1.0f, 0.0f));
    ImGui::SetNextWindowBgAlpha(0.35f);

    if (ImGui::Begin("##FPS_Overlay", nullptr, windowFlags)) {
        ImGui::Text("FPS: %.1f", smoothedFps);
    }
    ImGui::End();
}

void VK_GUI::GUI::draw_imgui(VK_INIT_ENGINE::_inited_engine& _init, VkCommandBuffer cmd, VkExtent2D _drawExtent){
    VkRenderingAttachmentInfo colorAttachment = vkinit::attachment_info(_init._drawImage.imageView, nullptr, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;

    VkRenderingAttachmentInfo depthAttachment = vkinit::depth_attachment_info(_init._depthImage.imageView, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL);
    depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
    depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;

    VkRenderingInfo renderInfo = vkinit::rendering_info(_drawExtent, &colorAttachment, nullptr);
    vkCmdBeginRendering(cmd, &renderInfo);

    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cmd);

    vkCmdEndRendering(cmd);
}

void VK_GUI::GUI::update_imgui(VK_INIT_ENGINE::_inited_engine& _init, CONTROLLER::Delta& _delta, ModelManager& _modelManager, std::unique_ptr<Scene>& _scene){
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplSDL3_NewFrame();
    ImGui::NewFrame();

    draw_fps_overlay(_init, _delta);

    draw_model_list_overlay(_init, _modelManager, _scene);

    draw_inspector_window(_init, _modelManager);

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
