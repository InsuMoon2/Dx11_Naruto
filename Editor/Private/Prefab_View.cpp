#include "pch.h"
#include "Prefab_View.h"

#include "Content_Browser.h"
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

    ImGuiWindowFlags flags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoDocking;

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

                if (_prevRT)
                {
                    _previewScreenPos = ImGui::GetCursorScreenPos();

                    ImGui::Image(_prevRT->GetSRV(), _previewImGuiSize);

                    if (ImGui::IsItemHovered())
                    {
                        if (ImGui::IsMouseDragging(ImGuiMouseButton_Right))
                        {
                            _previewYaw += ImGui::GetIO().MouseDelta.x * 0.01f;
                        }

                        // 스크롤로 줌
                        float wheel = ImGui::GetIO().MouseWheel;
                        if (wheel != 0.f)
                        {
                            _previewDistnace -= wheel * 0.5f;
                            _previewDistnace = max(1.f, min(_previewDistnace, 30.f));
                        }
                    }
                }
                else
                {
                    ImGui::Button("##ModelPreview", _previewImGuiSize);
                }


                ImGui::Separator();
                static int currentState = 0;
                const char* states[] = { "Idle", "Run", "Attack", "Hit", "Dead" };

                ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
                ImGui::Combo("##StateCombo", &currentState, states, IM_ARRAYSIZE(states));

                ImGui::Spacing();
                ImGui::Spacing();
                ImGui::Spacing();

                float remainingSpace = ImGui::GetContentRegionAvail().y - 30.f;
                ImGui::Dummy(ImVec2(0, remainingSpace));  
            }

            // [우측] 컴포넌트 리스트
            ImGui::TableSetColumnIndex(1);
            {
                Draw_ComponentList();
            }
            ImGui::EndTable();
        }
        ImGui::Separator();
        //Draw_Buttons();

        Update_ImGuizmo();
    }
    ImGui::End();


    //ImGui::Begin("Test");
    //ImGui::DragFloat("OffsetX", &_previewCenter.x, 0.1f, 1.f, 30.f);
    //ImGui::DragFloat("OffsetY", &_previewCenter.y, 0.1f, 1.f, 30.f);
    //ImGui::DragFloat("OffsetZ", &_previewCenter.z, 0.1f, 1.f, 30.f);
    //ImGui::DragFloat("Yaw", &_previewYaw, 0.01f);
    //ImGui::DragFloat("Pitch", &_previewPitch, 0.01f);
    //ImGui::End();

}

bool Prefab_View::CanSave() const
{
    return _targetObject != nullptr && !_prefabPath.empty();
}

void Prefab_View::Save()
{
    GAME->Save_Prefab(_prefabPath, _targetObject);
    ClearDirty();
    EDITOR->Get_Notification()->Add_Notification("Prefab Saved: {}", _prefabName);
}

void Prefab_View::Open_Prefab(const string& prefabName, const string& prefabPath)
{
    _prefabName = prefabName;
    _prefabPath = prefabPath;

    _targetObject = nullptr;

    // 인스턴스화
    _targetObject = GAME->Instantiate_Prefab(prefabName, {});

    if (_targetObject)
    {
        _isOpen = true;

        if (_prevRT == nullptr)
        {
            auto device = GAME->Get_Device();
            _prevRT = RenderTarget::Create(device, 600.f, 300.f);
        }

        // 프리뷰 카메라 초기값
        _previewYaw = -2.21f;
        _previewDistnace = 16.5f;
    }
        

}

void Prefab_View::Close_Prefab()
{
    _isOpen = false;
    _targetObject = nullptr;
}

void Prefab_View::Pre_Render()
{
    EditorWindow::Pre_Render();

    if (!_isOpen || !_targetObject || !_prevRT)
        return;

    // 기존 View/Proj 백업
    Matrix savedView = *GAME->Get_Transform(ETransformState::View);
    Matrix savedProj = *GAME->Get_Transform(ETransformState::Proj);

    _previewDistnace = max(_previewDistnace, 0.5f);

    Vec3 target = Vec3(0.f, 0.f, 0.f);

    float camX = _previewDistnace * sinf(_previewPitch) * sinf(_previewYaw);
    float camY = _previewDistnace * cosf(_previewPitch);
    float camZ = _previewDistnace * sinf(_previewPitch) * cosf(_previewYaw);

    Vec3 center = _targetObject->Get_Component<Transform>()->Get_WorldPosition();
    center += _previewCenter;

    _previewCamPos = center + Vec3(camX, camY + 3.f, camZ);

    _previewView = XMMatrixLookAtLH(_previewCamPos, center, Vec3::Up);;

    float aspect = static_cast<float>(_prevRT->GetWidth()) / _prevRT->GetHeight();
    _previewProj = XMMatrixPerspectiveFovLH(XM_PIDIV4, aspect, 0.1f, 100.f);

    GAME->Set_Transform(ETransformState::View, _previewView);
    GAME->Set_Transform(ETransformState::Proj, _previewProj);

    _prevRT->Clear(Color(0.15f, 0.15f, 0.15f, 1.f));
    _prevRT->BindAsTarget();

    _targetObject->Render();

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
    CHECK_NULL(_targetObject);

#pragma region Legacy : Inspector 방식으로 보여주기
    //Inspector::Draw_Components(_targetObject);
#pragma endregion

    ImGui::Text("Components Inspector");
    ImGui::Separator();

    auto& components = _targetObject->Get_Components();
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
        _targetObject->Remove_Component(deleteTargetID);
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
                    _targetObject->Add_Component(typeId, newComp);
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
        if (_targetObject)
        {
            string filePath = fs::path(_prefabPath).stem().string();

            LOG_INFO("Saving to: '{}'", filePath);

            GAME->Save_Prefab(_prefabPath, _targetObject);
            ClearDirty();
            EDITOR->Get_Notification()->Add_Notification("Prefab Saved: {}", _prefabName);
        }
    }

    ImGui::SameLine();

    if (ImGui::Button(ICON_FA_XMARK " Close", ImVec2(100, 0)))
    {
        _isOpen = false;
        _targetObject = nullptr;
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
        }
    }
}

void Prefab_View::Update_ImGuizmo()
{
    auto transform = _targetObject->Get_Component<Transform>();
    if (
        transform && _gizmoOperation != (ImGuizmo::OPERATION)0)
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
