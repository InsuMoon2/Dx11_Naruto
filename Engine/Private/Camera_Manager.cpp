#include "pch.h"
#include "Camera_Manager.h"

#include "Camera.h"
#include "Input_Manager.h"

#include "CameraTrack_Player.h"
#include "CameraTrack_Serializer.h"
#include "Camera_Cinematic.h"
#include "GameInstance.h"
#include "GameObject.h"

// target_tag 문자열로 현재 레벨에서 실제 타겟 오브젝트를 찾을 때 호출한다.
// 현재 시네마틱 타겟 문자열은 GameObject 이름 문자열 기준으로 사용한다.
static Shared<GameObject> Find_CinematicTargetObject(const string& targetTag)
{
    if (targetTag.empty())
        return nullptr;

    const auto objects = GAME->Get_GameObjects(GAME->Current_Level());
    for (const auto& obj : objects)
    {
        if (!obj)
            continue;

        if (Utils::ToString(obj->Get_Name()) == targetTag)
            return obj;
    }

    return nullptr;
}

// 현재 프레임 카메라 키 설정을 Runtime 시네마틱 카메라에 반영할 때 호출한다.
// Target / LookAt 모드의 distance, pitch, yaw, targetOffset, targetTransform을 같이 전달한다.
static void Apply_TrackPlayerState_ToRuntimeCamera(
    const Shared<CameraTrack_Player>& player,
    const Shared<Camera_Cinematic>& cineCamera,
    const Shared<Transform>& anchorTransform)
{
    if (!player || !cineCamera)
        return;

    const FCameraKey& currentKey = player->Get_CurrentKey();

    cineCamera->Set_Mode(currentKey.cameraMode);
    cineCamera->Set_Distance(currentKey.distance);
    cineCamera->Set_TargetOffset(currentKey.targetOffset);
    cineCamera->Set_PitchYaw(currentKey.pitch, currentKey.yaw);

    Shared<Transform> targetTransform = nullptr;

    if (currentKey.cameraMode == ECineCameraMode::Target ||
        currentKey.cameraMode == ECineCameraMode::LookAt)
    {
        if (!currentKey.targetTag.empty())
        {
            auto targetObject = Find_CinematicTargetObject(currentKey.targetTag);
            if (targetObject)
                targetTransform = targetObject->Get_Transform();
        }

        // target_tag가 비어 있으면 anchor를 기본 타겟으로 사용한다.
        // OwnerRelative 카메라가 플레이어에 장착된 연출에서 실제 플레이 카메라와 동기화하기 위한 기본 동작이다.
        if (!targetTransform)
            targetTransform = anchorTransform;
    }

    cineCamera->Set_TargetTransform(targetTransform);
}

void Camera_Manager::Update(float timeDelta)
{
    Clear_InvalidCameras();

    // 자유카메라 -> 타겟 카메라 전환용 -> 근데 이거 에디터용으로 옮겨야할듯
    if (INPUT->KeyDown(KEY_TYPE::F8))
        Toggle_Camera();

    // 런타임 시네마틱 재생
    if (_isCinematicPlaying && _cinePlayer)
    {
        if (auto anchor = _cineAnchorTransform.lock())
            _cinePlayer->Set_AnchorTransform(anchor);
        else
            _cinePlayer->Clear_AnchorTransform();

        _cinePlayer->Tick(timeDelta);

        if (_cineCamera && _currentAsset)
        {
            Apply_TrackPlayerState_ToRuntimeCamera(
                _cinePlayer,
                _cineCamera,
                _cineAnchorTransform.lock());

            _cineCamera->Apply_CinematicState(
                _cinePlayer->Get_Position(),
                _cinePlayer->Get_Rotation(),
                _cinePlayer->Get_FovY()
            );

            _cineCamera->Priority_Update(timeDelta);
            _cineCamera->Update(timeDelta);
            _cineCamera->Late_Update(timeDelta);

            GAME->Set_Transform(ETransformState::View, _cineCamera->Get_ViewMatrix());
            GAME->Set_Transform(ETransformState::Proj, _cineCamera->Get_ProjMatrix());

            if (_cinePlayer->IsFinished())
            {
                Stop_Cinematic();
            }
        }
    }
}

void Camera_Manager::Set_ActiveCamera(Shared<Camera> camera)
{
    if (!camera)
    {
        LOG_WARN("Camera_Manager::Set_ActiveCamera - target camera is null");
        _activeCamera.reset();
        INPUT->UnlockMouse();
        return;
    }

    _activeCamera = camera;

    if (camera->Get_ObjectType() == Protocol::OBJECT_TYPE_CAMERA_TARGET)
        INPUT->LockMouse();
    else
        INPUT->UnlockMouse();

    LOG_INFO("Active Camera Changed");
}

bool Camera_Manager::Is_ActiveCamera(const Shared<Camera> camera) const
{
    return _activeCamera.lock() == camera;
}

void Camera_Manager::Toggle_Camera()
{
    Clear_InvalidCameras();

    auto current = _activeCamera.lock();
    auto nextCam = Find_NextValidCamera(current);

    if (!nextCam)
        return;

    if (current && current != nextCam)
    {
        auto srcT = current->Get_Component<Transform>();
        auto destT = nextCam->Get_Component<Transform>();

        if (srcT && destT)
        {
            destT->Set_LocalPosition(srcT->Get_WorldPosition());
            destT->Set_LocalRotation(srcT->Get_WorldRotation());
        }
    }

    Set_ActiveCamera(nextCam);
}

void Camera_Manager::Register_Camera(Shared<Camera> camera)
{
    if (!camera)
        return;

    Clear_InvalidCameras();

    for (auto& weakCamera : _cameras)
    {
        if (weakCamera.lock() == camera)
            return;
    }

    _cameras.push_back(camera);

    // 첫번째 카메라 액티브로 세팅
    if (!_activeCamera.lock())
        _activeCamera = camera;
}

Shared<Camera> Camera_Manager::Find_Camera(Protocol::OBJECT_TYPE type)
{
    Clear_InvalidCameras();

    for (auto& weak : _cameras)
    {
        auto camera = weak.lock();

        if (camera && camera->Get_ObjectType() == type)
        {
            return camera;
        }
    }

    return nullptr;
}

void Camera_Manager::Clear_InvalidCameras()
{
    _cameras.erase(remove_if(_cameras.begin(), _cameras.end(),
        [](const Weak<Camera>& weakCamera)
        {
            return weakCamera.expired();
        }),
        _cameras.end());

    if (_activeCamera.expired())
    {
        _activeCamera.reset();
        INPUT->UnlockMouse();
    }

    if (_originCamera.expired())
    {
        _originCamera.reset();
    }

}

Shared<Camera> Camera_Manager::Find_NextValidCamera(const Shared<Camera>& current)
{
    Clear_InvalidCameras();

    if (_cameras.empty())
        return nullptr;

    int32 currentIndex = -1;

    for (int32 i = 0; i < static_cast<int32>(_cameras.size()); ++i)
    {
        auto camera = _cameras[i].lock();
        if (camera == current)
        {
            currentIndex = i;
            break;
        }
    }

    // 현재 활성 카메라가 목록에 없으면 첫 번째 유효 카메라 반환
    if (currentIndex < 0)
    {
        for (auto& weakCamera : _cameras)
        {
            auto camera = weakCamera.lock();
            if (camera)
                return camera;
        }

        return nullptr;
    }

    const int32 cameraCount = static_cast<int32>(_cameras.size());

    for (int32 offset = 1; offset <= cameraCount; ++offset)
    {
        const int32 nextIndex = (currentIndex + offset) % cameraCount;
        auto nextCamera = _cameras[nextIndex].lock();

        if (nextCamera)
            return nextCamera;
    }

    return nullptr;
}

bool Camera_Manager::Play_Cinematic(const wstring& sequenceName)
{
    return Play_Cinematic(sequenceName, nullptr, true);
}

bool Camera_Manager::Play_Cinematic(const wstring& sequenceName, Shared<Transform> anchorTransform, bool blockGameInput)
{
    // 최초 재생
    if (!_cineCamera)
    {
        Camera_Cinematic::FCinematicDesc desc;
        desc.mode = ECineCameraMode::Free;
        desc.mouseSensor = 10.f;

        auto clone = GAME->Clone_GameObject(0, Protocol::OBJECT_TYPE_CAMERA_CINEMATIC, &desc);
        if (clone)
            _cineCamera = dynamic_pointer_cast<Camera_Cinematic>(clone);

        _cinePlayer = CameraTrack_Player::Create();
    }

    // 로드 실패
    auto nextAsset = make_unique<FCameraSequenceAsset>();
    const wstring filePath = CameraTrack_Serializer::Get_BaseFolderPath() / (sequenceName + L".json");

    if (!CameraTrack_Serializer::Load_FromFile(filePath, *nextAsset))
    {
        LOG_WARN("Play_Cinematic failed. sequence='{}'", Utils::ToString(sequenceName));
        return false;
    }

    if (_isCinematicPlaying)
        Stop_Cinematic();

    _currentAsset = std::move(nextAsset);

    _cineAnchorTransform = anchorTransform;
    _blockGameInputOnCinematic = blockGameInput;

    _cinePlayer->Bind(&_currentAsset->track);

    if (anchorTransform)
        _cinePlayer->Set_AnchorTransform(anchorTransform);
    else
        _cinePlayer->Clear_AnchorTransform();

    _cinePlayer->Play();
    _isCinematicPlaying = true;

    // 복귀할 원래 카메라
    _originCamera = _activeCamera;

    if (_blockGameInputOnCinematic)
        GAME->Set_GameInputEnabled(false);

    return true;
}

void Camera_Manager::Stop_Cinematic()
{
    if (!_isCinematicPlaying)
        return;

    _isCinematicPlaying = false;

    if (_cinePlayer)
        _cinePlayer->Stop();

    _currentAsset.reset();
    _cineAnchorTransform.reset();

    if (auto origin = _originCamera.lock())
        Set_ActiveCamera(origin);

    // 이번 시네마틱이 입력을 막았던 경우에만 복구
    if (_blockGameInputOnCinematic)
        GAME->Set_GameInputEnabled(true);

    _blockGameInputOnCinematic = false;
}

Unique<Camera_Manager> Camera_Manager::Create()
{
    auto instance = make_unique<Camera_Manager>();

    return instance;
}

void Camera_Manager::Free()
{
    if (_cinePlayer)
        _cinePlayer->Stop();

    _currentAsset.reset();
    _cineAnchorTransform.reset();
    _originCamera.reset();
    _blockGameInputOnCinematic = false;
    _isCinematicPlaying = false;

    Base::Free();
}
