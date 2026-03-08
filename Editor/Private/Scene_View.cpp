#include "pch.h"
#include "Scene_View.h"

#include "Action_Command.h"
#include "Camera.h"
#include "GameObject.h"
#include "Event_Manager.h"
#include "RenderTarget.h"
#include "GameInstance.h"
#include "EditorInstance.h"
#include "Input_Manager.h"
#include "Hierarchy.h"
#include "Asset_Manager.h"
#include "Camera_Free.h"
#include "Spawn_Helper.h"
#include "StaticMeshActor.h"

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

    Update_CameraLerp(timeDelta);

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

void Scene_View::Pre_Render()
{
    EditorWindow::Pre_Render();

    Render_Preview();
}

void Scene_View::Focus_OnPosition(const Vec3& targetPos)
{
    _isCameraLerping = true;
    _lerpTargetPos = targetPos;
}

void Scene_View::Update_CameraLerp(float timeDelta)
{
    if (!_isCameraLerping)
        return;

    auto camera = GAME->Get_ActiveCamera();
    if (!camera)
    {
        _isCameraLerping = false; 
        return;
    }

    auto camTransform = camera->Get_Component<Transform>();
    CHECK_NULL(camTransform);

    Vec3 currentPos = camTransform->Get_WorldPosition();

    // 타겟 뒤쪽 계산
    Vec3 look = camTransform->Get_WorldForward();
    Vec3 targetCamPos = _lerpTargetPos - look * _lerpDistance;
    targetCamPos.y += 4.f;

    // Lerp
    Vec3 newPos = Vec3::Lerp(currentPos, targetCamPos, timeDelta * 5.f);
    camTransform->Set_WorldPosition(newPos);

    // 도착하면 종료
    if ((newPos - targetCamPos).Length() < 0.1f)
        _isCameraLerping = false;
}

void Scene_View::Clear_Drag()
{
    _previewObject = nullptr;
    _isDraggingPrefab = false;
}

Shared<GameObject> Scene_View::Create_StaticMesh(const string& guid, const Vec3& position)
{
    wstring assetPath = GAME->Resolve_AssetPath(guid);
    if (assetPath.empty())
    {
        LOG_ERROR("Create_StaticMeshActor: GUID {} 를 찾을 수 없습니다.", guid);
        return nullptr;
    }

    StaticMeshActor::FStaticMeshDesc desc;
    desc.modelGuid = guid;
    desc.name = Utils::ToWString(fs::path(assetPath).stem().string());

    auto meshActor = StaticMeshActor::Create(GAME->Get_Device(), GAME->Get_Context());
    if (!meshActor)
        return nullptr;

    if (FAILED(meshActor->Initialize(&desc)))
        return nullptr;

    auto transform = meshActor->Get_Component<Transform>();
    if (transform)
        transform->Set_WorldPosition(position);

    return meshActor;
}

void Scene_View::Handle_DragDrop(Shared<GameObject> previewObj, const Vec3& worldPos, const ImGuiPayload* payload)
{
    if (!_previewObject)
    {
        _previewObject = previewObj;
        _isDraggingPrefab = true;
    }

    if (_previewObject)
    {
        auto transform = _previewObject->Get_Component<Transform>();
        if (transform) transform->Set_WorldPosition(worldPos);
    }

    if (payload->IsDelivery() && _previewObject)
    {
        GAME->Add_GameObject(GAME->Current_Level(),
            TEXT("Layer_GameObject"), _previewObject);

        auto hierarchy = dynamic_pointer_cast<Hierarchy>(
            EDITOR->Get_Window(TEXT("Hierarchy")));
        if (hierarchy) hierarchy->Select_Object(_previewObject, false);

        Clear_Drag();
    }

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
            ImVec2 mousePos = ImGui::GetMousePos();
            ImVec2 windowPos = ImGui::GetWindowPos();
            ImVec2 contentMin = ImGui::GetWindowContentRegionMin();

            Vec2 localPos(
                mousePos.x - windowPos.x - contentMin.x,
                mousePos.y - windowPos.y - contentMin.y
            );
            Vec3 worldPos = Screen_To_World(localPos);

            if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("CONTENT_PREFAB",
                ImGuiDragDropFlags_AcceptBeforeDelivery | ImGuiDragDropFlags_AcceptNoDrawDefaultRect))
            {
                string guid = (const char*)payload->Data;
                string prefabName = GUID_To_PrefabName(guid);

                auto obj = _previewObject ? nullptr : GAME->Instantiate_Prefab(prefabName);

                Handle_DragDrop(obj, worldPos, payload);
            }

            // Content Mesh
            if (auto* payload = ImGui::AcceptDragDropPayload("CONTENT_MESH",
                ImGuiDragDropFlags_AcceptBeforeDelivery | ImGuiDragDropFlags_AcceptNoDrawDefaultRect))
            {
                string guid = (const char*)payload->Data;
                auto obj = _previewObject ? nullptr : Create_StaticMesh(guid, worldPos);

                Handle_DragDrop(obj, worldPos, payload);
            }

            ImGui::EndDragDropTarget();
        }
        else
        {
            if (_previewObject)
            {
                Clear_Drag();
            }
        }
    }
    
}

void Scene_View::Update_WindowState()
{
    if (!_isFullScreen)
        _savedDockId = ImGui::GetWindowDockID();

    _isFocused = ImGui::IsWindowFocused();
    _isHovered = ImGui::IsWindowHovered();

    auto freeCam = GAME->Find_Camera(Protocol::OBJECT_TYPE_CAMERA_FREE);
    if (freeCam)
    {
        auto camFree = dynamic_pointer_cast<Camera_Free>(freeCam);
        if (camFree)
            camFree->Set_InputEnabled(true);

    }
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

    // 행렬
    const Matrix* pView = GAME->Get_Transform(ETransformState::View);
    const Matrix* pProj = GAME->Get_Transform(ETransformState::Proj);
    if (!pView || !pProj) return;

    Matrix world = transform->Get_WorldMatrix();
    Matrix view = *pView;
    Matrix proj = *pProj;

    Matrix worldForGizmo = world;

    ImGuizmo::Manipulate(&view.m[0][0], &proj.m[0][0],
        _gizmoOperation, _gizmoMode, &worldForGizmo.m[0][0]);

    if (ImGuizmo::IsUsing())
    {
        if (!_gizmoWasUsing)
        {
            _gizmoStartPos   = transform->Get_LocalPosition();
            _gizmoStartRot   = transform->Get_LocalRotation();
            _gizmoStartScale = transform->Get_LocalScale();
            _gizmoWasUsing = true;
        }

        if (ImGui::GetIO().KeyAlt && !_altDragDuplicated)
        {
            EVENT->Publish(FEvent_Object::Create(EEventType::Create_Object, targetObject));
            _altDragDuplicated = true;
        }

        Vec3 scale, translation;
        Quat rotation;

        worldForGizmo.Decompose(scale, rotation, translation);

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
    else
    {
        _altDragDuplicated = false;

        // 기즈모 조작 끝
        if (_gizmoWasUsing)
        {
            Vec3 newPos = transform->Get_LocalPosition();
            Quat newRot = transform->Get_LocalRotation();
            Vec3 newScale = transform->Get_LocalScale();

            auto cmd = Action_Command::Create(
                [=]()
                { // Undo
                    transform->Set_LocalPosition(_gizmoStartPos);
                    transform->Set_LocalRotation(_gizmoStartRot);
                    transform->Set_LocalScale(_gizmoStartScale);
                },
                [=]()// Redo
                {
                    transform->Set_LocalPosition(newPos);
                    transform->Set_LocalRotation(newRot);
                    transform->Set_LocalScale(newScale);
                },
                "Transform Gizmo Edit");

            EDITOR->ExecuteCommand(cmd);

            _gizmoWasUsing = false;
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
    // NDC로 변환
    float ndcX = (screenPos.x / _viewportSize.x) * 2.f - 1.f;
    float ndcY = 1.f - (screenPos.y / _viewportSize.y) * 2.f;

    // 역행렬 세팅
    const Matrix* invertView = GAME->Get_TransformInverse(ETransformState::View);
    const Matrix* invertProj = GAME->Get_TransformInverse(ETransformState::Proj);

    if (!invertView || !invertProj)
        return Vec3::Zero;

    // NDC -> View Space
    Vec3 nearNDC(ndcX, ndcY, 0.f);
    Vec3 farNDC(ndcX, ndcY, 1.f);

    Vec3 nearView = Vec3::Transform(nearNDC, *invertProj);
    Vec3 farView = Vec3::Transform(farNDC, *invertProj);

    // View -> World
    Vec3 nearWorld = Vec3::Transform(nearView, *invertView);
    Vec3 farWorld = Vec3::Transform(farView, *invertView);

    // 레이 방향
    Vec3 rayDir = farWorld - nearWorld;
    rayDir.Normalize();

    // 일단 임시값으로 y = 5 평면과의 교차점 계산으로 세팅
    float targetY = 5;
    if (fabsf(rayDir.y) < FLT_EPSILON)
        return Vec3(nearWorld.x, targetY, nearWorld.z);

    float t = (targetY - nearWorld.y) / rayDir.y;

    return nearWorld + rayDir * t;
}

string Scene_View::GUID_To_PrefabName(const string& guid)
{
    wstring assetPath = GAME->Resolve_AssetPath(guid);
    if (assetPath.empty())
    {
        LOG_ERROR("Unknown asset GUID: {}", guid);
        return "";
    }

    fs::path path(assetPath);
    string fileName = path.filename().string();
    size_t dotPos = fileName.find('.');

    return (dotPos != string::npos) ? fileName.substr(0, dotPos) : path.stem().string();
}

void Scene_View::Spawn_Prefab(const string& guid, const Vec3& worldPos)
{
#pragma region Legacy : 경로 기반 소환
    // 파일 경로에서 프리펩 이름 추출
    //fs::path path(prefabPath);
    //string fileName = path.filename().string();
    //size_t dotPos = fileName.find('.');
    //string prefabName = (dotPos != string::npos) ? fileName.substr(0, dotPos) : path.stem().string();
    //
    //auto newObj = GAME->Instantiate_Prefab(prefabName);
    //
    //if (!newObj)
    //{
    //    LOG_ERROR("Failed to instantiate prefab: {}", prefabName);
    //    return;
    //}
    //
    //// UUID 출력 로그 확인
    //LOG_INFO("Spawned '{}' UUID: {}",
    //    Utils::ToString(newObj->Get_Name()),
    //    newObj->Get_GUID());
    //
    //// 위치 세팅
    //auto transform = newObj->Get_Component<Transform>();
    //if (transform)
    //{
    //    transform->Set_WorldPosition(worldPos);
    //}
    //
    //// 스폰
    //GAME->Add_GameObject(ETOI(ELevelType::GamePlay), TEXT("Layer_GamePlay"), newObj);
    //
    //LOG_INFO("Position: ({:.2f}, {:.2f}, {:.2f})", worldPos.x, worldPos.y, worldPos.z);
    //
    //auto hierarchy = dynamic_pointer_cast<Hierarchy>(EDITOR->Get_Window(TEXT("Hierarchy")));
    //if (hierarchy)
    //{
    //    hierarchy->Select_Object(newObj, false);  // false = 단일 선택
    //}
#pragma endregion

    string prefabName = GUID_To_PrefabName(guid);
    if (prefabName.empty()) return;

    auto newObj = Spawn_Helper::Prefab(prefabName)
        .AtLevel(GAME->Current_Level())
        .InLayer(TEXT("Layer_GameObject"))
        .Position(worldPos)
        .Spawn();

    if (!newObj)
        return;

    auto hierarchy = dynamic_pointer_cast<Hierarchy>(
        EDITOR->Get_Window(TEXT("Hierarchy")));

    if (hierarchy)
        hierarchy->Select_Object(newObj, false);

}

void Scene_View::Render_Preview()
{
    if (!_previewObject)
        return;

    GAME->Add_RenderGroup(ERenderGroup::NonBlend, _previewObject);
}

shared_ptr<Scene_View> Scene_View::Create()
{
    return make_shared<Scene_View>();
}
