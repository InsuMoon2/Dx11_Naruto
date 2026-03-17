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
    float           _distanceMin = 3.f;
    float           _distanceMax = 15.f;
    float           _zoomSpeed = 2.f;


public:
    static Shared<Camera_Target> Create(ComPtr<Device> device, ComPtr<DeviceContext> context);
    Shared<GameObject> Clone(void* arg) override;
    void Free() override;

};

NS_END
