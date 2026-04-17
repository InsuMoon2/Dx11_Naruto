#include "pch.h"
#include "Scene_View.h"
#include "Mesh.h"
#include "Action_Command.h"
#include "Camera.h"
#include "GameObject.h"
#include "Event_Manager.h"
#include "RenderTarget.h"
#include "GameInstance.h"
#include "EditorInstance.h"
#include "Input_Manager.h"
#include "Hierarchy.h"
#include "Camera_Free.h"
#include "CollisionProxyActor.h"
#include "Spawn_Helper.h"
#include "StaticMeshActor.h"
#include "Model.h"
#include "Notification_Manager.h"

static bool Try_BuildPickingLocalBounds(Shared<Model> model, BoundingBox& outBounds)
{
    if (!model)
        return false;

    Vec3 minPos = Vec3(FLT_MAX, FLT_MAX, FLT_MAX);
    Vec3 maxPos = Vec3(-FLT_MAX, -FLT_MAX, -FLT_MAX);
    bool hasAnyVertex = false;

    for (const auto& mesh : model->Get_Meshes())
    {
        if (!mesh)
            continue;

        const auto& positions = mesh->Get_CPUPositions();
        if (positions.empty())
            continue;

        for (const auto& localPos : positions)
        {
            minPos.x = min(minPos.x, localPos.x);
            minPos.y = min(minPos.y, localPos.y);
            minPos.z = min(minPos.z, localPos.z);

            maxPos.x = max(maxPos.x, localPos.x);
            maxPos.y = max(maxPos.y, localPos.y);
            maxPos.z = max(maxPos.z, localPos.z);

            hasAnyVertex = true;
        }
    }

    if (!hasAnyVertex)
        return false;

    outBounds.Center = (minPos + maxPos) * 0.5f;
    outBounds.Extents = (maxPos - minPos) * 0.5f;

    return true;
}

static wstring Get_CollisionUnitPlanePath()
{
    return fs::absolute(
        L"../../Client/Bin/Resources/StaticMesh/CollisionProxy/Meshes/SM_Collision_UnitPlane.meshbin").wstring();
}

static string Resolve_CollisionUnitPlaneGuid()
{
    const wstring unitPlanePath = Get_CollisionUnitPlanePath();

    if (!fs::exists(unitPlanePath))
    {
        LOG_ERROR("Collision unit plane missing: {}", Utils::ToString(unitPlanePath));
        NOTIFY("Collision Unit Plane Missing");
        return "";
    }

    string guid = GAME->Find_AssetGUID(unitPlanePath);
    if (!guid.empty())
        return guid;

    guid = GAME->Register_Asset(unitPlanePath, "model");
    if (guid.empty())
    {
        LOG_ERROR("Collision unit plane register failed: {}", Utils::ToString(unitPlanePath));
        NOTIFY("Collision Unit Plane Register Failed");
    }

    return guid;
}

static bool Try_BuildProxyPlaneBaseSize(const string& modelGuid, Vec2& outPlaneSize)
{
    const wstring resolvedPath = GAME->Resolve_AssetPath(modelGuid);
    if (resolvedPath.empty())
        return false;

    const Matrix preTransform =
        Matrix::CreateScale(1.f) *
        Matrix::CreateRotationY(XMConvertToRadians(180.f));

    auto model = Model::Create(
        GAME->Get_Device(),
        GAME->Get_Context(),
        EMeshVertexType::StaticMesh,
        Utils::ToString(resolvedPath),
        preTransform,
        true);

    if (!model)
        return false;

    BoundingBox localBounds{};
    if (!Try_BuildPickingLocalBounds(model, localBounds))
        return false;

    const Vec3 size = Vec3(localBounds.Extents.x * 2.f, localBounds.Extents.y * 2.f, localBounds.Extents.z * 2.f);
    outPlaneSize.x = max(size.x, 0.0001f);
    outPlaneSize.y = max(size.z, 0.0001f);

    return true;
}

static void Notify_CollisionProxyCreateResult(int32 createdCount, int32 skippedCount)
{
    if (createdCount <= 0)
    {
        NOTIFY("Collision Proxy Set Failed");
        return;
    }

    if (skippedCount > 0)
    {
        NOTIFY("Collision Proxy Set Created (Some faces skipped)");
        return;
    }

    NOTIFY("Collision Proxy Set Created");
}

static bool Try_BuildPickingWorldBounds(
    Shared<Model> model,
    const Matrix& worldMatrix,
    BoundingBox& outBounds)
{
    if (!model)
        return false;

    Vec3 minPos = Vec3(FLT_MAX, FLT_MAX, FLT_MAX);
    Vec3 maxPos = Vec3(-FLT_MAX, -FLT_MAX, -FLT_MAX);
    bool hasAnyVertex = false;

    for (const auto& mesh : model->Get_Meshes())
    {
        if (!mesh)
            continue;

        const auto& positions = mesh->Get_CPUPositions();
        if (positions.empty())
            continue;

        for (const auto& localPos : positions)
        {
            const Vec3 worldPos = Vec3::Transform(localPos, worldMatrix);

            minPos.x = min(minPos.x, worldPos.x);
            minPos.y = min(minPos.y, worldPos.y);
            minPos.z = min(minPos.z, worldPos.z);

            maxPos.x = max(maxPos.x, worldPos.x);
            maxPos.y = max(maxPos.y, worldPos.y);
            maxPos.z = max(maxPos.z, worldPos.z);

            hasAnyVertex = true;
        }
    }

    if (!hasAnyVertex)
        return false;

    outBounds.Center = (minPos + maxPos) * 0.5f;
    outBounds.Extents = (maxPos - minPos) * 0.5f;

    return true;
}

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
    _displayRenderTarget = RenderTarget::Create(GAME->Get_Device(), GAME->Get_ViewportWidth(), GAME->Get_ViewportHeight());
}

void Scene_View::Update(float timeDelta)
{
    EditorWindow::Update(timeDelta);

    Update_CameraLerp(timeDelta);

    //Handle_Guizmo_Shotcut();
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

        Handle_Guizmo_Shotcut();

        Render_Viewport();

        ImGui::SetCursorPos(ImVec2(12.f, 30.f));
        ImGui::BeginChild("##SceneViewOverlay", ImVec2(170.f, 34.f), false, ImGuiWindowFlags_NoScrollbar);
        ImGui::Checkbox("RT Debug", &_showRenderTargetDebug);
        ImGui::EndChild();

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

Ray Scene_View::Build_PickingRay(Vec2 localMousePos) const
{
    float ndcX = (localMousePos.x / _viewportSize.x) * 2.f - 1.f;
    float ndcY = 1.f - (localMousePos.y / _viewportSize.y) * 2.f;

    const Matrix* invertView = GAME->Get_TransformInverse(ETransformState::View);
    const Matrix* invertProj = GAME->Get_TransformInverse(ETransformState::Proj);

    if (!invertView || !invertProj)
    {
        return Ray(Vec3::Zero, Vec3::Forward);
    }

    Vec3 nearNdc(ndcX, ndcY, 0.f);
    Vec3 farNdc(ndcX, ndcY, 1.f);

    Vec3 nearView = Vec3::Transform(nearNdc, *invertProj);
    Vec3 farView = Vec3::Transform(farNdc, *invertProj);

    Vec3 nearWorld = Vec3::Transform(nearView, *invertView);
    Vec3 farWorld = Vec3::Transform(farView, *invertView);

    Vec3 rayDir = farWorld - nearWorld;
    rayDir = Utils::Safe_Normalize(rayDir, Vec3::Forward);

    return Ray(nearWorld, rayDir);
}

Shared<GameObject> Scene_View::Pick_GameObject(const Ray& ray) const
{
    Shared<GameObject> pickedObject = nullptr;
    float closestDist = FLT_MAX;

    const auto gameObjects = GAME->Get_GameObjects(GAME->Current_Level());

    for (const auto& obj : gameObjects)
    {
        if (!obj)
            continue;

        auto transform = obj->Get_Component<Transform>();
        if (!transform)
            continue;

        Shared<Model> model = nullptr;

        if (auto staticMesh = dynamic_pointer_cast<StaticMeshActor>(obj))
        {
            model = staticMesh->Get_Model();
        }
        else if (auto proxyActor = dynamic_pointer_cast<CollisionProxyActor>(obj))
        {
            model = proxyActor->Get_Model();
        }
        else
        {
            continue;
        }

        if (!model)
            continue;

        BoundingBox worldBounds{};
        if (!Try_BuildPickingWorldBounds(model, transform->Get_WorldMatrix(), worldBounds))
            continue;

        float boundsHitDist = 0.f;
        if (!ray.Intersects(worldBounds, boundsHitDist))
            continue;

        if (boundsHitDist > closestDist)
            continue;

        float hitDist = 0.f;
        Vec3 hitPoint = Vec3::Zero;
        Vec3 hitNormal = Vec3::Up;

        if (!model->Raycast(ray, transform->Get_WorldMatrix(), hitDist, hitPoint, hitNormal))
            continue;

        if (hitDist < closestDist)
        {
            closestDist = hitDist;
            pickedObject = obj;
        }
    }

    return pickedObject;
}

void Scene_View::Handle_MousePicking()
{
    if (!_isHovered || !_isFocused)
        return;

    if (ImGuizmo::IsUsing())
        return;

    if (_previewObject)
        return;

    if (ImGui::IsMouseDown(ImGuiMouseButton_Right))
        return;

    if (!ImGui::IsMouseClicked(ImGuiMouseButton_Left))
        return;

    ImVec2 mousePos = ImGui::GetMousePos();

    if (mousePos.x < _viewportTopLeft.x || mousePos.x > _viewportBottomRight.x ||
        mousePos.y < _viewportTopLeft.y || mousePos.y > _viewportBottomRight.y)
    {
        return;
    }

    Vec2 localMousePos(
        mousePos.x - _viewportTopLeft.x,
        mousePos.y - _viewportTopLeft.y
    );

    Ray pickingRay = Build_PickingRay(localMousePos);
    auto pickedObject = Pick_GameObject(pickingRay);
    if (!pickedObject)
        return;

    auto hierarchy = dynamic_pointer_cast<Hierarchy>(EDITOR->Get_Window(TEXT("Hierarchy")));
    if (!hierarchy)
        return;

    const bool isMultiSelect = ImGui::GetIO().KeyCtrl;
    hierarchy->Select_Object(pickedObject, isMultiSelect);
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

Shared<GameObject> Scene_View::Create_CollisionProxy(
    const CollisionProxyActor::FCollisionProxyDesc& desc)
{
    auto proxyActor = CollisionProxyActor::Create(GAME->Get_Device(), GAME->Get_Context());
    if (!proxyActor)
        return nullptr;

    auto proxyDesc = desc;

    if (FAILED(proxyActor->Initialize(&proxyDesc)))
        return nullptr;

    return proxyActor;
}

void Scene_View::Create_CollisionProxySetFromStaticMesh(Shared<GameObject> sourceObj)
{
    auto staticMesh = dynamic_pointer_cast<StaticMeshActor>(sourceObj);
    CHECK_NULL(staticMesh);

    auto sourceTransform = staticMesh->Get_Component<Transform>();
    if (!sourceTransform)
        return;

    auto sourceModel = staticMesh->Get_Model();
    if (!sourceModel)
        return;

    const string unitPlaneGuid = Resolve_CollisionUnitPlaneGuid();
    if (unitPlaneGuid.empty())
        return;

    Vec2 planeBaseSize(1.f, 1.f);
    if (!Try_BuildProxyPlaneBaseSize(unitPlaneGuid, planeBaseSize))
    {
        LOG_ERROR("Collision unit plane base size build failed");
        NOTIFY("Collision Unit Plane Invalid");
        return;
    }

    BoundingBox localBounds{};
    if (!Try_BuildPickingLocalBounds(sourceModel, localBounds))
    {
        LOG_ERROR(
            "Create_CollisionProxySetFromStaticMesh: local bounds build failed for '{}'",
            Utils::ToString(staticMesh->Get_Name()));
        NOTIFY("Collision Proxy Bounds Failed");
        return;
    }

    const Vec3 localMin = Vec3(localBounds.Center.x, localBounds.Center.y, localBounds.Center.z)
                    - Vec3(localBounds.Extents.x, localBounds.Extents.y, localBounds.Extents.z);

    const Vec3 localMax = Vec3(localBounds.Center.x, localBounds.Center.y, localBounds.Center.z)
                    + Vec3(localBounds.Extents.x, localBounds.Extents.y, localBounds.Extents.z);

    const Vec3 localCenter = localBounds.Center;

    const Vec3 sourceWorldScaleRaw = sourceTransform->Get_WorldScale();
    const Vec3 sourceWorldScale(
        max(fabsf(sourceWorldScaleRaw.x), 0.0001f),
        max(fabsf(sourceWorldScaleRaw.y), 0.0001f),
        max(fabsf(sourceWorldScaleRaw.z), 0.0001f));

    const float width = max((localMax.x - localMin.x) * sourceWorldScale.x, 0.f);
    const float height = max((localMax.y - localMin.y) * sourceWorldScale.y, 0.f);
    const float depth = max((localMax.z - localMin.z) * sourceWorldScale.z, 0.f);

    const float outwardOffset = 0.05f;
    const float roofOffset = 0.03f;
    const float minWallWidth = 0.5f;
    const float minWallHeight = 1.0f;
    const float minRoofSize = 0.5f;

    const Matrix sourceWorldMatrix = sourceTransform->Get_WorldMatrix();
    const Quat sourceWorldRotation = sourceTransform->Get_WorldRotation();
    const wstring baseName = staticMesh->Get_Name();

    int32 createdCount = 0;
    int32 skippedCount = 0;

    auto createFaceProxy =
        [&](const wstring& proxyName,
            ECollisionProxyType proxyType,
            const Vec3& localFaceCenter,
            const Quat& localFaceRotation,
            const Vec3& proxyScale,
            bool shouldCreate)
        {
            if (!shouldCreate)
            {
                ++skippedCount;
                return;
            }

            CollisionProxyActor::FCollisionProxyDesc desc{};
            desc.name = proxyName;
            desc.modelGuid = unitPlaneGuid;
            desc.proxyType = proxyType;

            auto proxyObj = Create_CollisionProxy(desc);
            if (!proxyObj)
            {
                ++skippedCount;
                return;
            }

            auto proxyTransform = proxyObj->Get_Component<Transform>();
            if (!proxyTransform)
            {
                ++skippedCount;
                return;
            }

            const Vec3 worldPos = Vec3::Transform(localFaceCenter, sourceWorldMatrix);
            const Quat worldRot = sourceWorldRotation * localFaceRotation;

            proxyTransform->Set_WorldPosition(worldPos);
            proxyTransform->Set_WorldRotation(worldRot);
            proxyTransform->Set_LocalScale(proxyScale);

            GAME->Add_GameObject(GAME->Current_Level(), TEXT("Layer_CollisionProxy"), proxyObj);
            ++createdCount;
        };

    createFaceProxy(
        baseName + L"_WallFrontProxy",
        ECollisionProxyType::WallRun,
        Vec3(localCenter.x, localCenter.y, localMax.z + outwardOffset),
        Quat::CreateFromYawPitchRoll(XMConvertToRadians(0.f), XMConvertToRadians(90.f), XMConvertToRadians(0.f)),
        Vec3(width / planeBaseSize.x, 1.f, height / planeBaseSize.y),
        width >= minWallWidth && height >= minWallHeight);

    createFaceProxy(
        baseName + L"_WallBackProxy",
        ECollisionProxyType::WallRun,
        Vec3(localCenter.x, localCenter.y, localMin.z - outwardOffset),
        Quat::CreateFromYawPitchRoll(XMConvertToRadians(180.f), XMConvertToRadians(90.f), XMConvertToRadians(0.f)),
        Vec3(width / planeBaseSize.x, 1.f, height / planeBaseSize.y),
        width >= minWallWidth && height >= minWallHeight);

    createFaceProxy(
        baseName + L"_WallLeftProxy",
        ECollisionProxyType::WallRun,
        Vec3(localMin.x - outwardOffset, localCenter.y, localCenter.z),
        Quat::CreateFromYawPitchRoll(XMConvertToRadians(-90.f), XMConvertToRadians(90.f), XMConvertToRadians(0.f)),
        Vec3(depth / planeBaseSize.x, 1.f, height / planeBaseSize.y),
        depth >= minWallWidth && height >= minWallHeight);

    createFaceProxy(
        baseName + L"_WallRightProxy",
        ECollisionProxyType::WallRun,
        Vec3(localMax.x + outwardOffset, localCenter.y, localCenter.z),
        Quat::CreateFromYawPitchRoll(XMConvertToRadians(90.f), XMConvertToRadians(90.f), XMConvertToRadians(0.f)),
        Vec3(depth / planeBaseSize.x, 1.f, height / planeBaseSize.y),
        depth >= minWallWidth && height >= minWallHeight);

    createFaceProxy(
        baseName + L"_WalkableProxy",
        ECollisionProxyType::Walkable,
        Vec3(localCenter.x, localMax.y + roofOffset, localCenter.z),
        Quat::Identity,
        Vec3(width / planeBaseSize.x, 1.f, depth / planeBaseSize.y),
        width >= minRoofSize && depth >= minRoofSize);

    LOG_INFO(
        "Collision proxy set created: actor='{}', created={}, skipped={}",
        Utils::ToString(baseName),
        createdCount,
        skippedCount);

    Notify_CollisionProxyCreateResult(createdCount, skippedCount);
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

        if (_displayRenderTarget)
        {
            _displayRenderTarget->Resize(static_cast<uint32>(panelSize.x),
                static_cast<uint32>(panelSize.y));
        }

        auto srv = _displayRenderTarget ? _displayRenderTarget->Get_SRV() : _renderTarget->Get_SRV();

        ImVec2 imageTopLeft = ImGui::GetCursorScreenPos();
        ImGui::Image((ImTextureID)srv, panelSize);

        _viewportTopLeft = imageTopLeft;
        _viewportBottomRight = ImVec2(imageTopLeft.x + panelSize.x, imageTopLeft.y + panelSize.y);

        if (ImGui::BeginDragDropTarget())
        {
            ImVec2 mousePos = ImGui::GetMousePos();

            Vec2 localPos(
                mousePos.x - _viewportTopLeft.x,
                mousePos.y - _viewportTopLeft.y
            );

            Vec3 worldPos = Screen_To_World(localPos);

            if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(
                "CONTENT_PREFAB",
                ImGuiDragDropFlags_AcceptBeforeDelivery | ImGuiDragDropFlags_AcceptNoDrawDefaultRect))
            {
                string guid = (const char*)payload->Data;
                string prefabName = GUID_To_PrefabName(guid);

                auto obj = _previewObject ? nullptr : GAME->Instantiate_Prefab(prefabName);
                Handle_DragDrop(obj, worldPos, payload);
            }

            if (auto* payload = ImGui::AcceptDragDropPayload(
                "CONTENT_MESH",
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

        Handle_MousePicking();
    }
}

void Scene_View::Update_WindowState()
{
    if (!_isFullScreen)
        _savedDockId = ImGui::GetWindowDockID();

    _isFocused = ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows);
    _isHovered = ImGui::IsWindowHovered(ImGuiHoveredFlags_RootAndChildWindows);

    const bool canControlSceneCamera = _isFocused && _isHovered;

    auto freeCam = GAME->Find_Camera(Protocol::OBJECT_TYPE_CAMERA_FREE);
    if (freeCam)
    {
        auto camFree = dynamic_pointer_cast<Camera_Free>(freeCam);
        if (camFree)
            camFree->Set_InputEnabled(canControlSceneCamera);

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
    //if (hierarchy)
    //{
    //    bool isMultiSelect = ImGui::GetIO().KeyCtrl;
    //    hierarchy->Select_Object(pickedObject, isMultiSelect);
    //}
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

    // 일단 임시값으로 y = 5 추후에, 네비메시 이후 평면과의 교차점 계산으로 세팅
    float targetY = 0.f;
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
