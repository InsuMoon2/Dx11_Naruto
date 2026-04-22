#include "pch.h"
#include "Camera_Target.h"

#include "GameObject_Factory.h"
#include "MyPlayer.h"
#include "Utils.h"

REGISTER_GAMEOBJECT(Camera_Target, Protocol::OBJECT_TYPE_CAMERA_TARGET)
IMPLEMENT_REFLECTION(Camera_Target);

bool Camera_Target::Register_Properties()
{
    auto& info = GetStaticReflectionInfo();
    info.className = "Camera_Target";

    PROPERTY_FLOAT("Distance", _distance, 1.f, 50.f);
    PROPERTY_FLOAT("Target Distance", _targetDistance, 1.f, 50.f);
    PROPERTY_FLOAT("Follow Speed", _followSpeed, 0.f, 20.f);
    PROPERTY_FLOAT("Zoom Lerp Speed", _zoomLerpSpeed, 0.f, 30.f);

    return true;
}

Camera_Target::Camera_Target(ComPtr<Device> device, ComPtr<DeviceContext> context)
    : Camera(device, context)
{
}

Camera_Target::Camera_Target(const Camera_Target& rhs)
    : Camera(rhs)
    , _offset(rhs._offset)
    , _followSpeed(rhs._followSpeed)
    , _pitch(rhs._pitch)
    , _yaw(rhs._yaw)
    , _heightOffset(rhs._heightOffset)
    , _mouseSensor(rhs._mouseSensor)
    , _pitchMin(rhs._pitchMin)
    , _pitchMax(rhs._pitchMax)
    , _distance(rhs._distance)
    , _targetDistance(rhs._targetDistance)
    , _distanceMin(rhs._distanceMin)
    , _distanceMax(rhs._distanceMax)
    , _zoomSpeed(rhs._zoomSpeed)
    , _zoomLerpSpeed(rhs._zoomLerpSpeed)
    , _enableMouseRotation(rhs._enableMouseRotation)
    , _bindOnPlayerSpawned(rhs._bindOnPlayerSpawned)
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
    _enableMouseRotation = desc->enableMouseRotation;
    _bindOnPlayerSpawned = desc->bindOnPlayerSpawned;

    _distance = 3.f;
    _targetDistance = _distance;

    CHECK_FAILED(Camera::Initialize(arg), E_FAIL);
    return S_OK;
}

void Camera_Target::BeginPlay()
{
    Camera::BeginPlay();

    GAME->Set_ActiveCamera(GetSharedPtr<Camera>());

    if (_bindOnPlayerSpawned)
    {
        GAME->Get_DelegateHub().OnPlayerSpawned.Add(
            this, &Camera_Target::Set_TargetTransform);
    }

    if (_damagedHandle.IsValid())
    {
        GAME->Get_DelegateHub().OnDamaged.Remove(_damagedHandle);
        _damagedHandle.Reset();
    }

    _damagedHandle = GAME->Get_DelegateHub().OnDamaged.Add(
        this, &Camera_Target::On_Damaged);
}

void Camera_Target::Priority_Update(float timeDelta)
{
    Camera::Priority_Update(timeDelta);

    if (!GAME->IsPlaying())
        return;

    auto target = _targetTransform.lock();
    if (!target)
        return;

    if (_enableMouseRotation)
    {
        const float dx = INPUT->GetMouseDelta().x;
        const float dy = INPUT->GetMouseDelta().y;

        _yaw += dx * _mouseSensor;
        _pitch += dy * _mouseSensor;
        _pitch = ::clamp(_pitch, _pitchMin, _pitchMax);

        const float wheel = INPUT->GetMouseWheel();
        _targetDistance -= wheel * _zoomSpeed;
        _targetDistance = ::clamp(_targetDistance, _distanceMin, _distanceMax);
    }

    const float zoomAlpha = std::clamp(_zoomLerpSpeed * timeDelta, 0.f, 1.f);
    _distance = std::lerp(_distance, _targetDistance, zoomAlpha);
    _distance = ::clamp(_distance, _distanceMin, _distanceMax);

    const float pitchRad = XMConvertToRadians(_pitch);
    const float yawRad = XMConvertToRadians(_yaw);

    Vec3 targetPos = target->Get_WorldPosition();
    targetPos.y += _heightOffset;

    Vec3 camOffset;
    camOffset.x = -cosf(pitchRad) * sinf(yawRad) * _distance;
    camOffset.y = sinf(pitchRad) * _distance;
    camOffset.z = -cosf(pitchRad) * cosf(yawRad) * _distance;

    const Vec3 desiredPos = targetPos + camOffset;
    const Vec3 currentPos = _transformCom->Get_LocalPosition();
    const float followAlpha = std::clamp(_followSpeed * timeDelta, 0.f, 1.f);
    const Vec3 newPos = Vec3::Lerp(currentPos, desiredPos, followAlpha);

    _transformCom->Set_LocalPosition(newPos);
    _transformCom->LookAt(targetPos);

    Vec3 localPosOffset = Vec3::Zero;
    Vec3 localRotOffsetDeg = Vec3::Zero;
    Update_CameraShake(timeDelta, localPosOffset, localRotOffsetDeg);

    const Matrix shakenView = Build_ShakenViewMatrix(localPosOffset, localRotOffsetDeg);
    const Matrix projMatrix = XMMatrixPerspectiveFovLH(_fovY, _aspect, _nearZ, _farZ);

    GAME->Set_Transform(ETransformState::View, shakenView);
    GAME->Set_Transform(ETransformState::Proj, projMatrix);
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

void Camera_Target::Request_CameraShake(const FCameraShakeDesc& request)
{
    if (request.durationSec <= 0.f)
        return;

    Push_CameraShake(request);
}

void Camera_Target::Stop_CameraShake(const string& tag)
{
    if (_activeCameraShakes.empty())
        return;

    if (tag.empty())
    {
        _activeCameraShakes.pop_back();
        return;
    }

    _activeCameraShakes.erase(
        remove_if(_activeCameraShakes.begin(), _activeCameraShakes.end(),
            [&tag](const FActiveCameraShake& shake)
            {
                return shake.request.tag == tag;
            }),
        _activeCameraShakes.end());
}

void Camera_Target::Clear_CameraShake()
{
    _activeCameraShakes.clear();
}

void Camera_Target::Push_CameraShake(const FCameraShakeDesc& request)
{
    FActiveCameraShake newShake{};
    newShake.request = request;
    newShake.elapsedSec = 0.f;
    newShake.posPhase = Vec3(
        Utils::RandomRange(0.f, XM_2PI),
        Utils::RandomRange(0.f, XM_2PI),
        Utils::RandomRange(0.f, XM_2PI));
    newShake.rotPhase = Vec3(
        Utils::RandomRange(0.f, XM_2PI),
        Utils::RandomRange(0.f, XM_2PI),
        Utils::RandomRange(0.f, XM_2PI));

    if (!request.tag.empty())
    {
        for (auto& activeShake : _activeCameraShakes)
        {
            if (activeShake.request.tag == request.tag)
            {
                activeShake = newShake;
                return;
            }
        }
    }

    if (_activeCameraShakes.size() >= MAX_ACTIVE_CAMERA_SHAKES)
    {
        _activeCameraShakes.erase(_activeCameraShakes.begin());
    }

    _activeCameraShakes.push_back(newShake);
}

void Camera_Target::Update_CameraShake(float timeDelta, Vec3& outLocalPosOffset, Vec3& outLocalRotOffsetDeg)
{
    outLocalPosOffset = Vec3::Zero;
    outLocalRotOffsetDeg = Vec3::Zero;

    for (auto& activeShake : _activeCameraShakes)
    {
        activeShake.elapsedSec += timeDelta;

        if (activeShake.elapsedSec >= activeShake.request.durationSec)
            continue;

        const float envelope = Compute_ShakeEnvelope(activeShake);
        const float frequency = max(0.f, activeShake.request.frequency);

        outLocalPosOffset.x += activeShake.request.localPosAmplitude.x
            * envelope * Sample_ShakeAxis(activeShake.elapsedSec, frequency, activeShake.posPhase.x);
        outLocalPosOffset.y += activeShake.request.localPosAmplitude.y
            * envelope * Sample_ShakeAxis(activeShake.elapsedSec, frequency, activeShake.posPhase.y);
        outLocalPosOffset.z += activeShake.request.localPosAmplitude.z
            * envelope * Sample_ShakeAxis(activeShake.elapsedSec, frequency, activeShake.posPhase.z);

        outLocalRotOffsetDeg.x += activeShake.request.localRotAmplitudeDeg.x
            * envelope * Sample_ShakeAxis(activeShake.elapsedSec, frequency, activeShake.rotPhase.x);
        outLocalRotOffsetDeg.y += activeShake.request.localRotAmplitudeDeg.y
            * envelope * Sample_ShakeAxis(activeShake.elapsedSec, frequency, activeShake.rotPhase.y);
        outLocalRotOffsetDeg.z += activeShake.request.localRotAmplitudeDeg.z
            * envelope * Sample_ShakeAxis(activeShake.elapsedSec, frequency, activeShake.rotPhase.z);
    }

    _activeCameraShakes.erase(
        remove_if(_activeCameraShakes.begin(), _activeCameraShakes.end(),
            [](const FActiveCameraShake& shake)
            {
                return shake.elapsedSec >= shake.request.durationSec;
            }),
        _activeCameraShakes.end());

    outLocalPosOffset.x = ::clamp(outLocalPosOffset.x, -MAX_SHAKE_POS_X, MAX_SHAKE_POS_X);
    outLocalPosOffset.y = ::clamp(outLocalPosOffset.y, -MAX_SHAKE_POS_Y, MAX_SHAKE_POS_Y);
    outLocalPosOffset.z = ::clamp(outLocalPosOffset.z, -MAX_SHAKE_POS_Z, MAX_SHAKE_POS_Z);

    outLocalRotOffsetDeg.x = ::clamp(outLocalRotOffsetDeg.x, -MAX_SHAKE_ROT_PITCH, MAX_SHAKE_ROT_PITCH);
    outLocalRotOffsetDeg.y = ::clamp(outLocalRotOffsetDeg.y, -MAX_SHAKE_ROT_YAW, MAX_SHAKE_ROT_YAW);
    outLocalRotOffsetDeg.z = ::clamp(outLocalRotOffsetDeg.z, -MAX_SHAKE_ROT_ROLL, MAX_SHAKE_ROT_ROLL);
}

Matrix Camera_Target::Build_ShakenViewMatrix(const Vec3& localPosOffset, const Vec3& localRotOffsetDeg) const
{
    const Matrix baseWorldMatrix = _transformCom->Get_WorldMatrix();

    // 쉐이크가 없을 때는 기존 카메라와 완전히 동일한 뷰 행렬을 사용한다.
    if (localPosOffset.LengthSquared() <= FLT_EPSILON &&
        localRotOffsetDeg.LengthSquared() <= FLT_EPSILON)
    {
        return baseWorldMatrix.Invert();
    }

    const Vec3 basePos = _transformCom->Get_WorldPosition();
    const Vec3 baseForward = _transformCom->Get_WorldForward();
    const Vec3 baseRight = _transformCom->Get_WorldRight();
    const Vec3 baseUp = _transformCom->Get_WorldUp();

    const Vec3 shakenPos =
        basePos +
        (baseRight * localPosOffset.x) +
        (baseUp * localPosOffset.y) +
        (baseForward * localPosOffset.z);

    const Quat baseRotation = _transformCom->Get_WorldRotation();
    const Quat shakeRotation = Quat::CreateFromYawPitchRoll(
        XMConvertToRadians(localRotOffsetDeg.y),
        XMConvertToRadians(localRotOffsetDeg.x),
        XMConvertToRadians(localRotOffsetDeg.z));

    const Quat finalRotation = baseRotation * shakeRotation;
    const Matrix finalRotationMatrix = Matrix::CreateFromQuaternion(finalRotation);

    // 이 엔진의 카메라 전방은 -Z 규약이다.
    const Vec3 finalForward = Utils::Safe_Normalize(
        -Vec3::TransformNormal(Vec3::Forward, finalRotationMatrix),
        baseForward);
    const Vec3 finalUp = Utils::Safe_Normalize(
        Vec3::TransformNormal(Vec3::Up, finalRotationMatrix),
        baseUp);

    return XMMatrixLookAtLH(shakenPos, shakenPos + finalForward, finalUp);
}

void Camera_Target::On_Damaged(Shared<Engine::Character> damagedCharacter, float damage)
{
    if (damage <= 0.f)
        return;

    auto myPlayer = dynamic_pointer_cast<MyPlayer>(damagedCharacter);
    if (!myPlayer)
        return;

    FCameraShakeDesc request{};
    request.tag = "player_damage";
    request.durationSec = 0.16f;
    request.frequency = 26.f;
    request.blendInSec = 0.01f;
    request.blendOutSec = 0.10f;
    request.localPosAmplitude = Vec3(0.05f, 0.04f, 0.02f);
    request.localRotAmplitudeDeg = Vec3(1.2f, 0.8f, 0.4f);

    GAME->Request_CameraShake(request);
}

float Camera_Target::Compute_ShakeEnvelope(const FActiveCameraShake& shake)
{
    float weight = 1.f;

    if (shake.request.blendInSec > 0.f)
    {
        weight = min(weight, std::clamp(
            shake.elapsedSec / shake.request.blendInSec, 0.f, 1.f));
    }

    if (shake.request.blendOutSec > 0.f)
    {
        const float remainSec = max(0.f, shake.request.durationSec - shake.elapsedSec);
        weight = min(weight, std::clamp(
            remainSec / shake.request.blendOutSec, 0.f, 1.f));
    }

    return weight;
}

float Camera_Target::Sample_ShakeAxis(float elapsedSec, float frequency, float phaseRad)
{
    return sinf((elapsedSec * frequency * XM_2PI) + phaseRad);
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
    if (_damagedHandle.IsValid())
    {
        GAME->Get_DelegateHub().OnDamaged.Remove(_damagedHandle);
        _damagedHandle.Reset();
    }

    _activeCameraShakes.clear();
    Camera::Free();
}
