#include "pch.h"
#include "Camera_Target.h"

Camera_Target::Camera_Target(ComPtr<Device> device, ComPtr<DeviceContext> context)
    : Camera(device, context)
{
    Set_ObjectType(Protocol::OBJECT_TYPE_CAMERA_TARGET);
}

Camera_Target::Camera_Target(const Camera_Target& rhs)
    : Camera(rhs)
    , _offset(rhs._offset)
    , _followSpeed(rhs._followSpeed)
{
}

HRESULT Camera_Target::Initialize_Prototype()
{
    Camera::Initialize_Prototype();

    return S_OK;
}

HRESULT Camera_Target::Initialize(void* arg)
{
    if (arg == nullptr)
        return E_FAIL;

    FCameraTargetDesc* desc = static_cast<FCameraTargetDesc*>(arg);
    _offset = desc->offset;
    _followSpeed = desc->followSpeed;

    LOG_INFO("Camera_Target: before Camera::Initialize");
    CHECK_FAILED(Camera::Initialize(arg), E_FAIL);
    LOG_INFO("Camera_Target: Initialize OK");

    return S_OK;
}

void Camera_Target::BeginPlay()
{
    Camera::BeginPlay();

    GAME->Set_ActiveCamera(GetSharedPtr<Camera>());

    GAME->Get_DelegateHub().OnPlayerSpawned.Add(
        this, &Camera_Target::Set_TargetTransform);
}

void Camera_Target::Priority_Update(float timeDelta)
{
    Camera::Priority_Update(timeDelta);

    if (!GAME->IsPlaying())
        return;

    auto target = _targetTransform.lock();
    if (!target) return;

    // 타겟 위치 + 오프셋 = 최종 위치
    Vec3 targetPos = target->Get_WorldPosition();
    Vec3 desiredPos = targetPos + _offset;

    // 위치 보간
    Vec3 currentPos = _transformCom->Get_LocalPosition();
    Vec3 newPos = Vec3::Lerp(currentPos, desiredPos, _followSpeed * timeDelta);

    _transformCom->Set_LocalPosition(newPos);
    _transformCom->LookAt(targetPos);

    // View/Proj 세팅
    Update_TransformMatrices();
}

void Camera_Target::Update(float timeDelta)
{
    Camera::Update(timeDelta);
}

void Camera_Target::Late_Update(float timeDelta)
{
    Camera::Late_Update(timeDelta);
}

HRESULT Camera_Target::Render()
{
    Camera::Render();


    return S_OK;
}

Shared<Camera_Target> Camera_Target::Create(ComPtr<Device> device, ComPtr<DeviceContext> context)
{
    auto instance = make_shared<Camera_Target>(device, context);

    if (FAILED(instance->Initialize_Prototype()))
    {
        MSG_BOX("Failed to Create : Camera_Target");
        instance->Free();

        return nullptr;
    }

    return instance;
}

Shared<GameObject> Camera_Target::Clone(void* arg)
{
    auto clone = make_shared<Camera_Target>(*this);

    if (FAILED(clone->Initialize(arg)))
    {
        MSG_BOX("Failed to Clone : Camera_Target");
        clone->Free();

        return nullptr;
    }

    return clone;
}

void Camera_Target::Free()
{
    Camera::Free();
}
