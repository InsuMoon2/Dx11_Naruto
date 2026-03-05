#include "pch.h"
#include "Prefab_View.h"

#include "Content_Browser.h"
#include "Editor_Camera_Free.h"
#include "GameInstance.h"
#include "GameObject.h"
#include "Inspector.h"
#include "Notification_Manager.h"
#include "RenderTarget.h"

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

    ImVec2 windowSize(1200, 800);
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
        ImGui::NewLine();
        ImGui::Separator();

        if (ImGui::BeginTable("PrefabLayout", 2, ImGuiTableFlags_Resizable| ImGuiTableFlags_BordersInnerV))
        {
            ImGui::TableSetupColumn("ModelPreview", ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableSetupColumn("Inspector", ImGuiTableColumnFlags_WidthStretch);

            ImGui::TableNextRow();

            // [좌측] 모델 프리뷰
            ImGui::TableSetColumnIndex(0);
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
                        ImGui::Image(_prevRT->GetSRV(), childSize);

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

            // [우측] 컴포넌트 리스트
            ImGui::TableSetColumnIndex(1);
            {
                ImVec2 listSize = ImVec2(0.f, 0.f); 
                ImGui::BeginChild("ComponentListChild", listSize, false); 
                {
                    Draw_ComponentList();
                }
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

            Editor_Camera_Free::FEditorCameraDesc desc;

            desc.speedPerSec = 10.f;   
            desc.rotationPerSec = 90.f;

            desc.eye = Vec3(-7.f, 3.f, -10.f);
            desc.at = Vec3(6.f, 0.f, 0.f);
            desc.fovY = XM_PIDIV4;
            desc.nearZ = 0.1f;
            desc.farZ = 1000.f;

            desc.mouseSensor = 0.15f;

            _previewCamera->Initialize(&desc);
        }
    }

}

void Prefab_View::Close_Prefab()
{
    _isOpen = false;

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

    _prevRT->Clear(Color(0.15f, 0.15f, 0.15f, 1.f));
    _prevRT->BindAsTarget();

    _previewObject->Render();

    GAME->BindBackBuffer();

    // 기존 View/Proj 복원
    GAME->Set_Transform(ETransformState::View, savedView);
    GAME->Set_Transform(ETransformState::Proj, savedProj);
}

void Prefab_View::Draw_Header()
{
    ImGui::TextColored(ImVec4(1, 1, 0, 1), "Prefab: %s", _prefabName.c_str());
    ImGui::TextDisabled("Path: %s", _prefabPath.c_str());
}

void Prefab_View::Draw_ComponentList()
{
    if (!_previewObject)
        return;

#pragma region Legacy : Inspector 방식으로 보여주기
    //Inspector::Draw_Components(_targetObject);
#pragma endregion

    ImGui::Text("Components Inspector");
    ImGui::Separator();

    auto& components = _previewObject->Get_Components();
    uint32 deleteTargetID = 0; // 삭제할 컴포넌트 ID

    for (auto& pair : components)
    {
        uint32 id = pair.first;
        auto& component = pair.second;

        if (!component)
            continue;

        ImGui::PushID(id);

        ImGui::BeginGroup();

        Inspector::Draw_Component(id, component);

        ImGui::EndGroup();

        if (ImGui::IsItemHovered() && ImGui::IsMouseReleased(ImGuiMouseButton_Right))
        {
            ImGui::OpenPopup("ComponentContextMenu");
        }
        // 4. 팝업 메뉴 열기
        if (ImGui::BeginPopup("ComponentContextMenu"))
        {
            if (ImGui::MenuItem("Remove Component"))
            {
                deleteTargetID = id;
            }
            ImGui::EndPopup();
        }
        ImGui::PopID();
    }

    if (deleteTargetID != 0)
    {
        _previewObject->Remove_Component(deleteTargetID);
        MarkDirty();
    }
        
    ImGui::Separator();
    ImGui::Spacing();

    // 컴포넌트 추가 버튼
    if (ImGui::Button("Add Component", ImVec2(ImGui::GetContentRegionAvail().x, 30)))
    {
        ImGui::OpenPopup("AddComponentPopup");
    }

    if (ImGui::BeginPopup("AddComponentPopup"))
    {
        auto registeredComponents = GAME->Get_RegisteredComponents();

        for (const auto& pair : registeredComponents)
        {
            uint32 typeId = pair.first;
            string name = Utils::ToString(pair.second);

            if (ImGui::MenuItem(name.c_str()))
            {
                auto newComp = GAME->Instantiate_FromFactory(typeId);

                if (newComp)
                {
                    _previewObject->Add_Component(typeId, newComp);
                    MarkDirty();
                }
            }
        }
        ImGui::EndPopup();
    }
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

shared_ptr<Prefab_View> Prefab_View::Create()
{
    return make_shared<Prefab_View>();
}
