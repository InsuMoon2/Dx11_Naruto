#include "pch.h"
#include "Camera_Manager.h"

#include "Camera.h"
#include "Input_Manager.h"

#include "CameraTrack_Player.h"
#include "CameraTrack_Serializer.h"
#include "Camera_Cinematic.h"
#include "GameInstance.h"

void Camera_Manager::Update(float timeDelta)
{
    // 자유카메라 -> 타겟 카메라 전환용 -> 근데 이거 에디터용으로 옮겨야할듯
    if (INPUT->KeyDown(KEY_TYPE::F8))
        Toggle_Camera();

    // 런타임 시네마틱 재생
    if (_isCinematicPlaying && _cinePlayer)
    {
        _cinePlayer->Tick(timeDelta);

        if (_cineCamera && _currentAsset)
        {
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
    auto current = _activeCamera.lock();

    for (size_t i = 0; i < _cameras.size(); i++)
    {
        if (_cameras[i].lock() == current)
        {
            size_t next = (i + 1) % _cameras.size();
            auto nextCam = _cameras[next].lock();

            if (current && nextCam)
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

            LOG_INFO("Camera Toggled");
            return;
        }
    }
}

void Camera_Manager::Register_Camera(Shared<Camera> camera)
{
    _cameras.push_back(camera);

    // 첫번째 카메라 액티브로 세팅
    if (!_activeCamera.lock())
        _activeCamera = camera;
}

Shared<Camera> Camera_Manager::Find_Camera(Protocol::OBJECT_TYPE type)
{
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

bool Camera_Manager::Play_Cinematic(const wstring& sequenceName)
{
    // 최초 재생 시 한번만 생성
    if (!_cineCamera)
    {
        Camera_Cinematic::FCinematicDesc desc;
        desc.mode = ECineCameraMode::Free;
        desc.mouseSensor = 10.f;

        auto clone = GAME->Clone_GameObject(0, Protocol::OBJECT_TYPE_CAMERA_CINEMATIC, &desc);
        if (clone)
        {
            _cineCamera = dynamic_pointer_cast<Camera_Cinematic>(clone);
        }
        _cinePlayer = CameraTrack_Player::Create();
    }

    wstring filePath = CameraTrack_Serializer::Get_BaseFolderPath() / (sequenceName + L".json");

    _currentAsset = make_unique<FCameraSequenceAsset>();

    if (!CameraTrack_Serializer::Load_FromFile(filePath, *_currentAsset))
    {
        _currentAsset.reset();

        return false;
    }

    // 바인딩 후 재생 시작
    _cinePlayer->Bind(&_currentAsset->track);
    _cinePlayer->Play();
    _isCinematicPlaying = true;

    // 카메라 임시 백업
    _originCamera = _activeCamera;

    // 컷신 중간에 조작 하지 못하게. 그런데 스킵을 만들지?에 대한 고민
    GAME->Set_GameInputEnabled(false);

    return true;
}

void Camera_Manager::Stop_Cinematic()
{
    if (!_isCinematicPlaying) return;

    _isCinematicPlaying = false;
    if (!_cinePlayer)
        _cinePlayer->Stop();

    _currentAsset.reset();

    // 원래 카메라로 원상복구
    if (auto origin = _originCamera.lock())
        Set_ActiveCamera(origin);

    GAME->Set_GameInputEnabled(true);
}

Unique<Camera_Manager> Camera_Manager::Create()
{
    auto instance = make_unique<Camera_Manager>();

    return instance;
}

void Camera_Manager::Free()
{
    Base::Free();
}
