#pragma once

#include "Camera.h"

NS_BEGIN(Engine)

class ENGINE_DLL Camera_Cinematic : public Camera
{
    GENERATED_BODY(Camera_Cinematic)

public:
    struct FCinematicDesc : public FCameraDesc
    {
        ECineCameraMode mode = ECineCameraMode::Free;

        float mouseSensor = 0.1f;
        float cameraSpeed = 10.f;
    };

public:
    explicit Camera_Cinematic(ComPtr<Device> device, ComPtr<DeviceContext> context);
    explicit Camera_Cinematic(const Camera_Cinematic& rhs);
    virtual ~Camera_Cinematic() = default;

public:
    HRESULT Initialize_Prototype() override;
    HRESULT Initialize(void* arg) override;

    void    BeginPlay() override;
    void    Priority_Update(float timeDelta) override;
    void    Update(float timeDelta) override;
    void    Late_Update(float timeDelta) override;

    HRESULT Render() override;

public:
    void    Set_Mode(ECineCameraMode mode) { _mode = mode; }
    ECineCameraMode Get_Mode() const { return _mode; }

    void Set_TargetTransform(Shared<Transform> target) { _targetTransform = target; }
    void Set_Distance(float distance) { _distance = distance; }
    void Set_TargetOffset(const Vec3& vec) { _targetOffset = vec; }
    void Set_PitchYaw(float pitch, float yaw) { _pitch = pitch; _yaw = yaw; }

    // 외부에서 보간 결과 강제 적용 (시퀀스에서 재생하게)
    void Apply_CinematicState(const Vec3& pos, const Quat& rot, float fovY);

    Matrix Get_ViewMatrix() const;
    Matrix Get_ProjMatrix() const;

private:
    // 모드별 입력처리
    void Update_FreeMode(float timeDelta);
    void Update_TargetMode(float timeDelta);
    void Update_LookAtMode(float timeDelta);

private:
    ECineCameraMode _mode = ECineCameraMode::Free;

    float           _mouseSensor = 0.1f;
    float           _cameraSpeed = 10.f;

    // Target / LookAt 전용
    Weak<Transform> _targetTransform;
    float           _distance = 10.f;
    Vec3            _targetOffset = Vec3(0.f, 2.f, 0.f);
    float           _pitch = 0.f;
    float           _yaw = 0.f;

public:
    static Shared<Camera_Cinematic> Create(ComPtr<Device> device, ComPtr<DeviceContext> context);
    Shared<GameObject> Clone(void* arg) override;
    void Free() override;

};

NS_END
