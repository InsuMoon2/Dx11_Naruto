#include "pch.h"
#include "Prefab_View.h"
#include "Model.h"
#include "Content_Browser.h"
#include "Editor_Camera_Free.h"
#include "GameInstance.h"
#include "GameObject.h"
#include "Inspector.h"
#include "Notification_Manager.h"
#include "RenderTarget.h"
#include "Reflection_Inspector.h"
#include "DebugDraw.h"
#include "Prefab_PreviewCameraSettings.h"
#include "Animation_View.h"
#include "ContainerObject.h"
#include "PartObject.h"

Prefab_View::Prefab_View()
    : EditorWindow(TEXT("Prefab"))
{
}

Prefab_View::~Prefab_View()
{
    Close_Prefab();
}

void Prefab_View::Initialize()
{
    EditorWindow::Initialize();

    if (!_previewCameraSettings)
        _previewCameraSettings = Prefab_PreviewCameraSettings::Create();

    _previewCameraSettings->Load_Settings();
}

void Prefab_View::Update(float timeDelta)
{
    EditorWindow::Update(timeDelta);

    Handle_Guizmo_Shotcut();
    
}

void Prefab_View::OnGui()
{
    if (!_isOpen)
        return;

    ImVec2 windowSize(1600, 1000);
    ImVec2 viewportSize = ImGui::GetMainViewport()->Size;

    ImVec2 windowPos(
        (viewportSize.x - windowSize.x) * 0.5f,
        (viewportSize.y - windowSize.y) * 0.5f
    );

    ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(windowSize, ImGuiCond_Appearing);

    ImGuiWindowFlags flags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse;

    string title = "Prefab View - " + _prefabName;
    if (_isDirty)
        title += " *";

    title += "###PrefabView";

    if (ImGui::Begin(title.c_str(), &_isOpen, flags))
    {
        _isFocused = ImGui::IsWindowFocused(ImGuiFocusedFlags_ChildWindows);

        Draw_Buttons();
        ImGui::SameLine();
        if (_isDirty)
        {
            ImGui::TextColored(ImVec4(1, 0.5f, 0, 1), ICON_FA_TRIANGLE_EXCLAMATION " Unsaved Changes");
        }
        ImGui::Dummy(ImVec2(0, 0));
        ImGui::Separator();

        if (ImGui::BeginTable("PrefabLayout", 3, ImGuiTableFlags_Resizable| ImGuiTableFlags_BordersInnerV))
        {
            ImGui::TableSetupColumn("Component List", ImGuiTableColumnFlags_WidthStretch, 1.f);
            ImGui::TableSetupColumn("ModelPreview", ImGuiTableColumnFlags_WidthStretch, 2.5f);
            ImGui::TableSetupColumn("Inspector", ImGuiTableColumnFlags_WidthStretch, 1.5f);

            ImGui::TableNextRow();

            // [좌측] 컴포넌트 리스트
            ImGui::TableSetColumnIndex(0);
            {
                ImVec2 listSize = ImVec2(0.f, 0.f);
                ImGui::BeginChild("ComponentListChild", listSize, false);
                {
                    Draw_ComponentList();
                }
                ImGui::EndChild();
            }

            // [중간] 모델 프리뷰
            ImGui::TableSetColumnIndex(1);
            {
                ImGui::Text("모델 프리뷰");
                _previewImGuiSize = ImVec2(ImGui::GetContentRegionAvail().x, 450);

                ImGui::BeginChild("ModelPreviewChild", _previewImGuiSize, false,
                    ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
                {
                    if (_prevRT)
                    {
                        _previewScreenPos = ImGui::GetCursorScreenPos();

                        ImVec2 childSize = ImGui::GetContentRegionAvail();
                        ImGui::Image((ImTextureID)_prevRT->Get_SRV(), childSize);

                        if (ImGui::IsWindowHovered())
                            _previewCamera->Priority_Update(ImGui::GetIO().DeltaTime);

                        Update_ImGuizmo();
                    }
                    else
                    {
                        ImGui::Button("##ModelPreview", _previewImGuiSize);
                    }
                }
                ImGui::EndChild();
            }
            
            // [우측] 컴포넌트 인스팩터
            ImGui::TableSetColumnIndex(2);
            {
                ImGui::BeginChild("InspectorChild", ImVec2(0, 0), false);
                Draw_ComponentInspector();
                ImGui::EndChild();
            }
            ImGui::EndTable();
        }
        ImGui::Separator();

    }
    ImGui::End();

}

bool Prefab_View::CanSave() const
{
    return _previewObject != nullptr && !_prefabPath.empty();
}

void Prefab_View::Save()
{
    GAME->Save_Prefab(_prefabPath, _previewObject);
    ClearDirty();
    EDITOR->Get_Notification()->Add_Notification("Prefab Saved: {}", _prefabName);
}

void Prefab_View::Open_Prefab(const string& prefabName, const string& prefabPath)
{
    _prefabName = prefabName;
    _prefabPath = prefabPath;
    _previewObject = nullptr;
    _previewHasBegunPlay = false;

    _selectionType = ESelectionType::Component;
    _selectedComponentId = Transform::StaticTypeID();
    _selectedPartSlot = 0;

    // 인스턴스화
    _previewObject = GAME->Instantiate_Prefab(prefabName, {});

    if (_previewObject)
    {
        _isOpen = true;

        if (_prevRT == nullptr)
        {
            auto device = GAME->Get_Device();
            _prevRT = RenderTarget::Create(device, 600.f, 300.f);
        }

        if (!_previewCamera)
        {
            _previewCamera = Editor_Camera_Free::Create(GAME->Get_Device(), GAME->Get_Context());

            auto desc = _previewCameraSettings->Build_Desc();
            _previewCamera->Initialize(&desc);
        }

        Apply_PreviewCameraSettings();

        Preview_BeginPlay();
        //Tick_PreviewAnimation(0.f);
    }

}

void Prefab_View::Close_Prefab()
{
    _isOpen = false;
    _previewHasBegunPlay = false;

    _previewCamera.reset();
    _previewObject.reset();
    _prevRT.reset();
}

void Prefab_View::Pre_Render()
{
    EditorWindow::Pre_Render();

    if (!_isOpen || !_previewObject || !_prevRT)
        return;

    // 기존 View/Proj 백업
    Matrix savedView = *GAME->Get_Transform(ETransformState::View);
    Matrix savedProj = *GAME->Get_Transform(ETransformState::Proj);

    if (!_previewCamera)
        return;

    _previewView = _previewCamera->Get_ViewMatrix();

    float aspect = static_cast<float>(_prevRT->GetWidth()) / _prevRT->GetHeight();
    _previewProj = XMMatrixPerspectiveFovLH(XM_PIDIV4, aspect, 0.1f, 100.f);

    GAME->Set_Transform(ETransformState::View, _previewView);
    GAME->Set_Transform(ETransformState::Proj, _previewProj);

    FLightDesc savedLight{};
    bool hasSavedLight = false;

    if (const FLightDesc* lightDesc = GAME->Get_LightDesc(0))
    {
        savedLight = *lightDesc;
        hasSavedLight = true;
    }

    FLightDesc previewLight{};
    previewLight.direction = Vec4(0.f, -1.f, 0.f, 0.f);
    previewLight.diffuse = Vec4(1.f, 1.f, 1.f, 1.f);
    previewLight.ambient = Vec4(1.f, 1.f, 1.f, 1.f);
    previewLight.specular = Vec4(0.f, 0.f, 0.f, 1.f);

    GAME->Clear_Lights();
    GAME->Add_Light(previewLight);

    float dt = ImGui::GetIO().DeltaTime;

    bool wasEnableInput = GAME->Is_GameInputEnabled();
    GAME->Set_GameInputEnabled(false);

    _previewObject->Priority_Update(dt);
    _previewObject->Update(dt);
    _previewObject->Late_Update(dt);

    GAME->Set_GameInputEnabled(wasEnableInput);

    _prevRT->Clear(Color(0.15f, 0.15f, 0.15f, 1.f));
    _prevRT->BindAsTarget();

    GAME->Draw();
    //_previewObject->Render();

    GAME->BindBackBuffer();

    GAME->Clear_Lights();

    if (hasSavedLight)
        GAME->Add_Light(savedLight);

    // 기존 View/Proj 복원
    GAME->Set_Transform(ETransformState::View, savedView);
    GAME->Set_Transform(ETransformState::Proj, savedProj);
}

void Prefab_View::Draw_PreviewCameraInspector()
{
    if (!_previewCamera)
        return;

    ImGui::TextColored(ImVec4(0.7f, 0.9f, 1.f, 1.f), "[ Preview Camera ]");
    ImGui::Separator();
    ImGui::Spacing();

    auto cameraTransform = _previewCamera->Get_Component<Transform>();
    if (cameraTransform)
    {
        ImGui::PushID("PreviewCameraTransform");
        Inspector::Draw_Component(Transform::StaticTypeID(), cameraTransform);
        ImGui::PopID();
    }
    else
    {
        ImGui::TextDisabled("Preview camera transform not found.");
        ImGui::Spacing();
    }

    auto& cameraRefInfo = _previewCamera->Get_ReflectionInfo();
    if (!cameraRefInfo.properties.empty())
    {
        static Reflection_Inspector autoInspector;
        autoInspector.Draw_FromReflection(_previewCamera.get(), cameraRefInfo);

        ImGui::Spacing();
    }

    if (ImGui::Button("Save Current View", ImVec2(-1.f, 28.f)))
    {
        _previewCameraSettings->Capture_FromCamera(_previewCamera);
        _previewCameraSettings->Save_Settings();
    }

    if (ImGui::Button("Reset To Saved", ImVec2(-1.f, 28.f)))
    {
        Apply_PreviewCameraSettings();
    }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();
}

void Prefab_View::Apply_PreviewCameraSettings()
{
    if (!_previewCamera || !_previewCameraSettings)
        return;

    const auto desc = _previewCameraSettings->Build_Desc();
    _previewCamera->Apply_EditorDesc(desc);
}

void Prefab_View::Draw_Header()
{
    ImGui::TextColored(ImVec4(1, 1, 0, 1), "Prefab: %s", _prefabName.c_str());
    ImGui::TextDisabled("Path: %s", _prefabPath.c_str());
    ImGui::Separator();
    ImGui::Spacing();

    if (_previewObject)
    {
        ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.2f, 1.f), "[ Class Type Override ]");

        // 검색 필터용 InputText
        static char filterBuffer[64] = "";
        ImGui::InputText("Search Type", filterBuffer, IM_ARRAYSIZE(filterBuffer));

        Protocol::OBJECT_TYPE currentType = _previewObject->Get_ObjectType();
        string currentTypeName = string(magic_enum::enum_name(currentType));

        // 콤보 박스 렌더링
        if (ImGui::BeginCombo("Type", currentTypeName.c_str()))
        {
            string filterStr = filterBuffer;
            std::transform(filterStr.begin(), filterStr.end(), filterStr.begin(), ::toupper);

            for (auto& enumVal : magic_enum::enum_values<Protocol::OBJECT_TYPE>())
            {
                string typeName = string(magic_enum::enum_name(enumVal));

                if (!filterStr.empty() && typeName.find(filterStr) == string::npos)
                    continue;

                bool isSelected = (currentType == enumVal);
                if (ImGui::Selectable(typeName.c_str(), isSelected))
                {
                    _previewObject->Set_ObjectType(enumVal);
                    MarkDirty();
                }

                if (isSelected)
                    ImGui::SetItemDefaultFocus();
            }
            ImGui::EndCombo();
        }
    }
}

void Prefab_View::Draw_ComponentList()
{
    if (!_previewObject) return;

    auto& components = _previewObject->Get_Components();
    ImGui::Text(ICON_FA_LIST " Components");
    ImGui::Separator();
    ImGui::Spacing();

    ImGui::BeginChild("##CompSelectArea", ImVec2(0, -40.f), false);
    for (auto& [id, comp] : components)
    {
        if (!comp) continue;

        bool hasInspector = false;

        if (Inspector_Factory::GetInstance()->Get_Inspector(id) ||
            Inspector_Factory::GetInstance()->Get_Inspector_ByType(comp))
        {
            hasInspector = true;
        }
        else if (!comp->Get_ReflectionInfo().properties.empty())
        {
            hasInspector = true;
        }

        if (!hasInspector)
            continue;

        string name = Utils::ToString(comp->Get_Name());
        bool isSelected = (_selectionType == ESelectionType::Component && _selectedComponentId == id);

        if (ImGui::Selectable(name.c_str(), isSelected))
        {
            _selectionType = ESelectionType::Component;
            _selectedComponentId = id; 
            MarkDirty();

        }
        if (ImGui::BeginPopupContextItem())
        {
            if (ImGui::MenuItem("Remove"))
            {
                _previewObject->Remove_Component(id);

                if (_selectedComponentId == id)
                    _selectedComponentId = 0;
                MarkDirty();
            }
            ImGui::EndPopup();
        }
    }

    Draw_PartObjectList();

    ImGui::EndChild();

    if (ImGui::Button(ICON_FA_PLUS " Add", ImVec2(-1, 30)))
        ImGui::OpenPopup("AddComponentPopup");

    if (ImGui::BeginPopup("AddComponentPopup"))
    {
        for (const auto& [typeId, name] : GAME->Get_RegisteredComponents())
        {
            if (ImGui::MenuItem(Utils::ToString(name).c_str()))
            {
                auto newComp = GAME->Instantiate_FromFactory(typeId);
                if (newComp)
                {
                    _previewObject->Add_Component(typeId, newComp);
                    _selectedComponentId = typeId; 
                    MarkDirty();
                }
            }
        }
        ImGui::EndPopup();
    }
}

void Prefab_View::Draw_ComponentInspector()
{
    if (!_previewObject)
        return;

    Draw_Header();
    ImGui::Separator();

    Draw_PreviewCameraInspector();
    //Draw_AnimationControls(); 굳이 여기서 안해도 될듯

    auto& refInfo = _previewObject->Get_ReflectionInfo();
    if (!refInfo.properties.empty())
    {
        string objName = Utils::ToString(_previewObject->Get_Name());
        ImGui::TextColored(ImVec4(1.f, 0.8f, 0.3f, 1.f), "[ %s ] Properties", objName.c_str());
        ImGui::Separator();
        ImGui::Spacing();

        static Reflection_Inspector autoInspector;
        autoInspector.Draw_FromReflection(_previewObject.get(), refInfo);

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();
    }

    if (_selectionType == ESelectionType::PartObject)
    {
        Draw_PartObjectInspector();
        return;
    }

    if (_selectedComponentId == 0)
        return;

    auto& components = _previewObject->Get_Components();
    auto it = components.find(_selectedComponentId);
    if (it == components.end() || !it->second)
        return;

    Inspector::Draw_Component(it->first, it->second);
}


void Prefab_View::Draw_Buttons()
{
    if (ImGui::Button(ICON_FA_FLOPPY_DISK "  Save", ImVec2(100.f, 0)))
    {
        if (_previewObject)
        {
            string filePath = fs::path(_prefabPath).stem().string();

            LOG_INFO("Saving to: '{}'", filePath);

            GAME->Save_Prefab(_prefabPath, _previewObject);
            ClearDirty();
            EDITOR->Get_Notification()->Add_Notification("Prefab Saved: {}", _prefabName);
        }
    }

    ImGui::SameLine();

    if (ImGui::Button(ICON_FA_XMARK " Close", ImVec2(100, 0)))
    {
        _isOpen = false;
        _previewObject = nullptr;
    }

    ImGui::SameLine();

    if (ImGui::Button(ICON_FA_CAMERA " Capture Thumbnail", ImVec2(150.f, 0)))
    {
        if (_prevRT)
        {
            string guid = GAME->Find_AssetGUID(Utils::ToWString(_prefabPath));
            if (!guid.empty())
            {
                fs::create_directories("../../Client/Bin/Resources/Thumbnails");

                wstring thumbPath = L"../../Client/Bin/Resources/Thumbnails/" + Utils::ToWString(guid) + L".png";

                if (SUCCEEDED(_prevRT->Save_To_File(thumbPath)))
                {
                    LOG_INFO("Thumbnail saved: {}", guid);

                    auto browser = dynamic_pointer_cast<Content_Browser>(
                        EDITOR->Get_Window(TEXT("Content Browser")));

                    if (browser)
                        browser->Load_Thumbnail(guid);
                }
                
            }

            EDITOR->Get_Notification()->Add_Notification("Capture Thumbnail: {}", _prefabName);
        }
    }
}

void Prefab_View::Update_ImGuizmo()
{
    if (!_previewObject)
        return;

    auto transform = _previewObject->Get_Component<Transform>();
    if (transform && _gizmoOperation != (ImGuizmo::OPERATION)0)
    {
        ImGuizmo::SetOrthographic(false);
        ImGuizmo::SetDrawlist();
         
        ImGuizmo::SetRect(_previewScreenPos.x, _previewScreenPos.y,
            _previewImGuiSize.x, _previewImGuiSize.y);

        Matrix world = transform->Get_WorldMatrix();
        ImGuizmo::Manipulate(
            &_previewView.m[0][0],   
            &_previewProj.m[0][0],
            _gizmoOperation, ImGuizmo::LOCAL,
            &world.m[0][0]
        );
        if (ImGuizmo::IsUsing())
        {
            Vec3 scale, translation;
            Quat rotation;
            world.Decompose(scale, rotation, translation);
            transform->Set_WorldPosition(translation);
            transform->Set_WorldRotation(rotation);
            transform->Set_LocalScale(scale);
        }
    }
}

void Prefab_View::Handle_Guizmo_Shotcut()
{
    if (!ImGuizmo::IsUsing())
    {
        if (ImGui::IsKeyPressed(ImGuiKey_W)) _gizmoOperation = ImGuizmo::TRANSLATE;
        if (ImGui::IsKeyPressed(ImGuiKey_E)) _gizmoOperation = ImGuizmo::ROTATE;
        if (ImGui::IsKeyPressed(ImGuiKey_R)) _gizmoOperation = ImGuizmo::SCALE;
        if (ImGui::IsKeyPressed(ImGuiKey_Q)) _gizmoOperation = (ImGuizmo::OPERATION)0;
    }
}

void Prefab_View::Preview_BeginPlay()
{
    if (!_previewObject || _previewHasBegunPlay)
        return;

    _previewObject->BeginPlay();
    _previewHasBegunPlay = true;
}

Shared<Model> Prefab_View::Find_PreviewModel() const
{
    if (!_previewObject)
        return nullptr;

    auto component = _previewObject->Find_Component_ByStaticType(Model::StaticTypeID());
    if (!component)
        return nullptr;

    return dynamic_pointer_cast<Model>(component);
}

void Prefab_View::Draw_AnimationControls()
{
    auto model = Find_PreviewModel();
    if (!model || !model->Has_Animations())
        return;

    ImGui::TextColored(ImVec4(0.7f, 0.9f, 1.f, 1.f), "[ Animation ]");
    ImGui::Separator();

    vector<string> names;
    vector<const char*> items;

    const uint32 count = model->Get_AnimationCount();
    names.reserve(count);
    items.reserve(count);

    for (uint32 i = 0; i < count; ++i)
        names.push_back(model->Get_AnimationName(i));

    for (auto& name : names)
        items.push_back(name.c_str());

    if (!items.empty())
    {
        _previewSelectedAnimIndex = std::clamp(
            _previewSelectedAnimIndex,
            0,
            static_cast<int32>(items.size()) - 1);

        ImGui::Combo("Preview Clip", &_previewSelectedAnimIndex, items.data(), static_cast<int32>(items.size()));
    }

    ImGui::Checkbox("Loop", &_previewAnimLoop);

    if (ImGui::Button("Apply Preview Animation"))
    {
        model->Set_Animation(static_cast<uint32>(_previewSelectedAnimIndex), _previewAnimLoop);
    }

    if (ImGui::Button("Open Animation View"))
    {
        Open_AnimationView();
    }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();
}

void Prefab_View::Open_AnimationView()
{
    auto model = Find_PreviewModel();
    if (!model)
        return;

    auto animView = dynamic_pointer_cast<Animation_View>(EDITOR->Get_Window(TEXT("Animation View")));
    if (!animView)
        return;

    // 현재 preview 중인 model을 Animation View에 넘기기
    animView->Open_Model(model);

    // combo에서 선택한 애니메이션으로 바로 보여주게
    if (model->Get_AnimationCount() > 0 &&
        _previewSelectedAnimIndex >= 0 &&
        _previewSelectedAnimIndex < static_cast<int32>(model->Get_AnimationCount()))
    {
        animView->Focus_Clip(model->Get_AnimationName(static_cast<uint32>(_previewSelectedAnimIndex)));
    }
}

void Prefab_View::Draw_PartObjectList()
{
    auto container = Get_PreviewContainer();
    CHECK_NULL(container);

    ImGui::Spacing(); ImGui::Separator(); ImGui::Spacing();
    ImGui::Text(ICON_FA_CUBES " Part Objects");
    ImGui::Spacing();

    for (auto slot : magic_enum::enum_values<ContainerObject::EPartSlot>())
    {
        if (slot == ContainerObject::EPartSlot::END) continue;

        auto part = container->Get_PartObject(slot);
        const bool exists = (part != nullptr);

        if (!exists)
            ImGui::BeginDisabled();

        string slotName = ContainerObject::Get_PartSlotName(slot);
        bool isSelected = (_selectionType == ESelectionType::PartObject
            && _selectedPartSlot == static_cast<uint32>(slot));

        if (ImGui::Selectable(slotName.c_str(), isSelected))
        {
            _selectionType = ESelectionType::PartObject;
            _selectedPartSlot = static_cast<uint32>(slot);
        }

        if (!exists) ImGui::EndDisabled();
    }

}

void Prefab_View::Draw_PartObjectInspector()
{
    auto container = Get_PreviewContainer();
    if (!container) return;

    auto slot = static_cast<ContainerObject::EPartSlot>(_selectedPartSlot);
    auto part = container->Get_PartObject(slot);

    if (!part) return;

    auto transform = part->Get_Component<Transform>();
    if (!transform) return;

    ImGui::TextColored(ImVec4(0.3f, 0.8f, 1.f, 1.f), "[ Part Object ]");
    ImGui::Separator();
    ImGui::Text("Slot : %s", ContainerObject::Get_PartSlotName(slot).c_str());
    ImGui::Text("Class : %s", Utils::ToString(part->Get_Name()).c_str());
    ImGui::Spacing();

    Inspector::Draw_Component(Transform::StaticTypeID(), transform);

    MarkDirty();

    ImGui::Spacing();

    // 위치 초기화 버튼
    if (ImGui::Button("Reset Local Transform", ImVec2(-1.f, 28.f)))
    {
        transform->Set_LocalPosition(Vec3::Zero);
        transform->Set_LocalEulerAngles(0.f, 0.f, 0.f);
        transform->Set_LocalScale(1.f, 1.f, 1.f);
        MarkDirty();
    }
}

Shared<ContainerObject> Prefab_View::Get_PreviewContainer() const
{
    if (!_previewObject) return nullptr;

    return dynamic_pointer_cast<ContainerObject>(_previewObject);
}

shared_ptr<Prefab_View> Prefab_View::Create()
{
    return make_shared<Prefab_View>();
}
