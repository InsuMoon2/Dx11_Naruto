#pragma once

#include "Camera.h"

NS_BEGIN(Client)

class Camera_Target : public Camera
{
    GENERATED_BODY(Camera_Target)

public:
    struct FCameraTargetDesc : public Camera::FCameraDesc
    {
        Vec3    offset = { 0.f, 10.f, -10.f };
        float   followSpeed = 5.f;

        bool    enableMouseRotation = true;
        bool    bindOnPlayerSpawned = true;
    };

private:
    struct FActiveCameraShake
    {
        FCameraShakeDesc request{};
        float   elapsedSec = 0.f;
        Vec3    posPhase = Vec3::Zero;
        Vec3    rotPhase = Vec3::Zero;
    };

public:
    explicit Camera_Target(ComPtr<Device> device, ComPtr<DeviceContext> context);
    explicit Camera_Target(const Camera_Target& rhs);
    virtual ~Camera_Target() = default;

public:
    HRESULT Initialize_Prototype() override;
    HRESULT Initialize(void* arg) override;
    void    BeginPlay() override;

    void    Priority_Update(float timeDelta) override;
    void    Update(float timeDelta) override;
    void    Late_Update(float timeDelta) override;
    HRESULT Render() override;

public:
    void    Set_TargetTransform(Shared<Transform> target) { _targetTransform = target; }
    float   Get_Yaw() const { return _yaw; }

public:
    void    Request_CameraShake(const FCameraShakeDesc& request) override;
    void    Stop_CameraShake(const string& tag = "") override;
    void    Clear_CameraShake() override;

private:
    void    Update_CameraShake(float timeDelta, Vec3& outLocalPosOffset, Vec3& outLocalRotOffsetDeg);
    void    Push_CameraShake(const FCameraShakeDesc& request);
    Matrix  Build_ShakenViewMatrix(const Vec3& localPosOffset, const Vec3& localRotOffsetDeg) const;

    // 데미지받아서 처리될 때
    void    On_Damaged(Shared<Character> damagedCharacter, float damage);

    static float Compute_ShakeEnvelope(const FActiveCameraShake& shake);
    static float Sample_ShakeAxis(float elapsedSec, float frequency, float phaseRad);

private:
    Weak<Transform> _targetTransform;

    Vec3            _offset = { 0.f, 10.f, -10.f };
    float           _followSpeed = 5.f;

    float           _pitch = 0.f;
    float           _yaw = 0.f;
    float           _heightOffset = 2.f;
    float           _mouseSensor = 0.5f;

    float           _pitchMin = -30.f;
    float           _pitchMax = 60.f;

    float           _distance = 10.f;

    float           _targetDistance = 10.f;

    float           _distanceMin = 3.f;
    float           _distanceMax = 15.f;

    float           _zoomSpeed = 2.f;
    float           _zoomLerpSpeed = 10.f;

private:
    bool            _enableMouseRotation = true;
    bool            _bindOnPlayerSpawned = true;

private:
    vector<FActiveCameraShake> _activeCameraShakes; 
    FDelegateHandle            _damagedHandle = {}; 

    static constexpr size_t MAX_ACTIVE_CAMERA_SHAKES = 4; // 동시에 유지할 최대 쉐이크 개수.
    static constexpr float MAX_SHAKE_POS_X = 0.25f;
    static constexpr float MAX_SHAKE_POS_Y = 0.25f;
    static constexpr float MAX_SHAKE_POS_Z = 0.25f;
    static constexpr float MAX_SHAKE_ROT_PITCH = 3.f;
    static constexpr float MAX_SHAKE_ROT_YAW = 3.f;
    static constexpr float MAX_SHAKE_ROT_ROLL = 3.f;

public:
    static Shared<Camera_Target> Create(ComPtr<Device> device, ComPtr<DeviceContext> context);
    Shared<GameObject> Clone(void* arg) override;
    void Free() override;

};

NS_END
