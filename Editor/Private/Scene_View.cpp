#include "pch.h"
#include "Scene_View.h"

#include <GameObject.h>

#include "RenderTarget.h"
#include "GameInstance.h"
#include "EditorInstance.h"
#include "Input_Manager.h"
#include "Hierarchy.h"

Scene_View::Scene_View()
    : EditorWindow(TEXT("Scene"))
{
}

Scene_View::~Scene_View()
{
}

void Scene_View::Initialize()
{
    EditorWindow::Initialize();

    // RenderTarget 초기화
    _renderTarget = RenderTarget::Create(GAME->Get_Device(), GAME->Get_ViewportWidth(), GAME->Get_ViewportHeight());
}

void Scene_View::Update(float timeDelta)
{
    EditorWindow::Update(timeDelta);

    // TODO : 카메라 추가 후, 이동 세팅

    if (INPUT->KeyDown(KEY_TYPE::F11))
    {
        ToggleFullScreen();
    }

    Handle_Guizmo_Shotcut();

}

void Scene_View::OnGui()
{
    Prepare_Window();

    ImGuiWindowFlags flags = Get_WindowFlags();

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));

    string str = Utils::ToString(Get_Name());
    ImGui::Begin(str.c_str(), nullptr, flags);
    {
        Update_WindowState();

        Render_Viewport();

        // Scene View에서만, Edit 모드일 때만 보여주기
        //if (GAME->Get_GameState() == EGameState::Edit)
        {
            Update_ImGuizmo();
        }

    }
    ImGui::End();
    ImGui::PopStyleVar();
}

ImGuiWindowFlags Scene_View::Get_WindowFlags() const
{
    ImGuiWindowFlags flags = ImGuiWindowFlags_None;

    if (_isFullScreen)
    {
        flags |= ImGuiWindowFlags_NoDecoration;
        flags |= ImGuiWindowFlags_NoMove;
        flags |= ImGuiWindowFlags_NoResize;
    }

    return flags;
}

void Scene_View::Prepare_Window()
{
    // DockID 복원
    if (_shouldRestoreWindow && _savedDockId != 0)
    {
        ImGui::SetNextWindowDockID(_savedDockId, ImGuiCond_Always);
        _shouldRestoreWindow = false;
    }

    // 전체화면 크기 설정
    if (_isFullScreen)
    {
        ImGui::SetNextWindowPos(ImVec2(0, 0));
        ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);
    }
}

void Scene_View::Render_Viewport()
{
    ImVec2 panelSize = ImGui::GetContentRegionAvail();
    _viewportSize = Vec2(panelSize.x, panelSize.y);

    if (_renderTarget && panelSize.x > 0 && panelSize.y > 0)
    {
        _renderTarget->Resize(static_cast<uint32>(panelSize.x),
            static_cast<uint32>(panelSize.y));

        ImGui::Image(_renderTarget->GetSRV(), panelSize);

        // 드래그 드롭 타겟
        if (ImGui::BeginDragDropTarget())
        {
            if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("CONTENT_PREFAB"))
            {

                wstring prefabPath = (wchar_t*)payload->Data;

                ImVec2 mousePos = ImGui::GetMousePos();
                ImVec2 windowPos = ImGui::GetWindowPos();
                ImVec2 contentMin = ImGui::GetWindowContentRegionMin();
                Vec2 localPos(
                    mousePos.x - windowPos.x - contentMin.x,
                    mousePos.y - windowPos.y - contentMin.y
                );

                Vec3 worldPos = Screen_To_World(localPos);

                Spawn_Prefab(prefabPath, worldPos);
            }
            ImGui::EndDragDropTarget();
        }
    }
}

void Scene_View::Update_WindowState()
{
    if (!_isFullScreen)
        _savedDockId = ImGui::GetWindowDockID();

    _isFocused = ImGui::IsWindowFocused();
    _isHovered = ImGui::IsWindowHovered();
}

void Scene_View::ToggleFullScreen()
{
    _isFullScreen = !_isFullScreen;

    HWND hWnd = EDITOR->Get_WindowHandle();

    if (_isFullScreen)
    {
        // 현재 윈도우 크기/스타일 저장
        GetWindowRect(hWnd, &_windowedRect);
        _savedStyle = GetWindowLongPtr(hWnd, GWL_STYLE);

        int screenWidth = GetSystemMetrics(SM_CXSCREEN);
        int screenHeight = GetSystemMetrics(SM_CYSCREEN);

        SetWindowLongPtr(hWnd, GWL_STYLE, WS_POPUP | WS_VISIBLE);
        SetWindowPos(hWnd, HWND_TOP, 0, 0, screenWidth, screenHeight,
            SWP_FRAMECHANGED);

        GAME->Resize_BackBuffer(screenWidth, screenHeight);
    }
    else
    {
        // 원래 윈도우 스타일/크기 복원
        SetWindowLongPtr(hWnd, GWL_STYLE, _savedStyle);
        SetWindowPos(hWnd, HWND_TOP,
            _windowedRect.left, _windowedRect.top,
            _windowedRect.right - _windowedRect.left,
            _windowedRect.bottom - _windowedRect.top,
            SWP_FRAMECHANGED);

        // Graphics 리사이즈
        RECT clientRect;
        GetClientRect(hWnd, &clientRect);
        GAME->Resize_BackBuffer(clientRect.right, clientRect.bottom);

        _shouldRestoreWindow = true; 
    }
}

void Scene_View::Update_ImGuizmo()
{
    if (_gizmoOperation == (ImGuizmo::OPERATION)0)
        return;

    ImGuizmo::SetOrthographic(false);
    ImGuizmo::SetDrawlist();

    ImVec2 vMin = ImGui::GetWindowContentRegionMin();
    ImVec2 wPos = ImGui::GetWindowPos();

    float x = vMin.x + wPos.x;
    float y = vMin.y + wPos.y;

    ImGuizmo::SetRect(x, y, _viewportSize.x, _viewportSize.y);

    // 선택된 오브젝트
    auto hierarchy = dynamic_pointer_cast<Hierarchy>(EDITOR->Get_Window(TEXT("Hierarchy")));
    CHECK_NULL(hierarchy);

    const auto& selectedObjects = hierarchy->Get_SelectedObject();
    if (selectedObjects.empty())
        return;

    // 이건 선택
    auto targetObject = selectedObjects[0]; // 첫 번째 객체만 조작
    CHECK_NULL(targetObject);

    auto transform = targetObject->Get_Component<Transform>();
    CHECK_NULL(transform);

    // 카메라 행렬은 추후 변경 예정
    //Matrix view = Matrix::Identity;
    Matrix view = Matrix::CreateLookAt(
        Vec3(0, 0, -10.f), 
        Vec3(0, 0, 0),     
        Vec3(0, 1, 0)      
    );

    Matrix proj = Matrix::CreatePerspectiveFieldOfView(
        XMConvertToRadians(60.f),
        _viewportSize.x / _viewportSize.y,
        0.1f,
        1000.f
    );

    // 오브젝트 월드 행렬
    Matrix world = transform->Get_WorldMatrix();

    Matrix worldForGizmo = world;
    worldForGizmo._41 = -worldForGizmo._41;

    ImGuizmo::Manipulate(
        &view.m[0][0],
        &proj.m[0][0],
        _gizmoOperation,
        _gizmoMode,
        &worldForGizmo.m[0][0]
    );

    if (ImGuizmo::IsUsing())
    {
        Vec3 scale, translation;
        Quat rotation;

        worldForGizmo.Decompose(scale, rotation, translation);

        translation.x = -translation.x;

        rotation.y = -rotation.y;
        rotation.z = -rotation.z;

        transform->Set_WorldPosition(translation);
        transform->Set_WorldRotation(rotation);

        // 스케일 (부모가 있을 경우 보정 필요)
        if (transform->Get_Parent())
        {
            Vec3 parentScale = transform->Get_Parent()->Get_WorldScale();
            // 0 나누기 방지
            if (parentScale.LengthSquared() > 0.0001f)
            {
                transform->Set_LocalScale(scale / parentScale);
            }
        }

        else
        {
            transform->Set_LocalScale(scale);
        }
    }

}

void Scene_View::Handle_Guizmo_Shotcut()
{
    if (!ImGuizmo::IsUsing())
    {
        if (ImGui::IsMouseDown(ImGuiMouseButton_Right))
            return;

        if (ImGui::IsKeyPressed(ImGuiKey_Q))
            _gizmoOperation = (ImGuizmo::OPERATION)0;

        if (ImGui::IsKeyPressed(ImGuiKey_W))
            _gizmoOperation = ImGuizmo::TRANSLATE;

        if (ImGui::IsKeyPressed(ImGuiKey_E))
            _gizmoOperation = ImGuizmo::ROTATE;

        if (ImGui::IsKeyPressed(ImGuiKey_R))
            _gizmoOperation = ImGuizmo::SCALE;

    }
    
}

Vec3 Scene_View::Screen_To_World(Vec2 screenPos)
{
    float normalizeX = (screenPos.x / _viewportSize.x) * 2.f - 1.f;
    float normalizeY = 1.f - (screenPos.y / _viewportSize.y) * 2.f;

    return Vec3(normalizeX * 5.f, normalizeY * 5.f, -5.f);
}

void Scene_View::Spawn_Prefab(const wstring& prefabPath, const Vec3& worldPos)
{
    // 파일 경로에서 프리펩 이름 추출
    fs::path path(prefabPath);
    wstring nameW = path.stem().wstring();
    string prefabName = Utils::ToString(nameW);

    // 프리펩 인스턴스 생성 (SR 때처럼 월드에 스폰 전에는 반투명하게 해줄지?)
    auto newObj = GAME->Instantiate_Prefab(prefabName);

    if (!newObj)
    {
        LOG_ERROR("Failed to instantiate prefab: {}", prefabName);
        return;
    }

    // UUID 출력 로그 확인
    LOG_INFO("Spawned '{}' UUID: {}",
        Utils::ToString(newObj->Get_Name()),
        newObj->Get_GUID());

    // 위치 세팅
    auto transform = newObj->Get_Component<Transform>();
    if (transform)
    {
        transform->Set_WorldPosition(worldPos);
    }

    // 스폰
    GAME->Add_GameObject(ETOI(ELevelType::GamePlay), TEXT("Layer_GamePlay"), newObj);

    LOG_INFO("Position: ({:.2f}, {:.2f}, {:.2f})", worldPos.x, worldPos.y, worldPos.z);

    auto hierarchy = dynamic_pointer_cast<Hierarchy>(EDITOR->Get_Window(TEXT("Hierarchy")));
    if (hierarchy)
    {
        hierarchy->Select_Object(newObj, false);  // false = 단일 선택
    }
}

shared_ptr<Scene_View> Scene_View::Create()
{
    return make_shared<Scene_View>();
}
