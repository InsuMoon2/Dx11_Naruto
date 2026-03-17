#pragma once

#include "Camera.h"

NS_BEGIN(Client)

class Camera_Free : public Camera
{
    GENERATED_BODY(Camera_Free)

public:
    struct FCameraFreeDesc : public Camera::FCameraDesc
    {
        float mouseSensor = 0.1f;
    };

public:
    explicit Camera_Free(ComPtr<Device> device, ComPtr<DeviceContext> context);
    explicit Camera_Free(const Camera_Free& rhs);
    virtual ~Camera_Free() = default;

public:
    HRESULT Initialize_Prototype() override;
    HRESULT Initialize(void* arg) override;
    void    BeginPlay() override;

    void    Priority_Update(float timeDelta) override;
    void    Update(float timeDelta) override;
    void    Late_Update(float timeDelta) override;
    HRESULT Render() override;

public:
    float Get_MouseSensor() const { return _mouseSensor; }
    float Get_CameraSpeed() const { return _cameraSpeed; }

protected:
    float _mouseSensor = {};
    float _cameraSpeed = 10.f;

public:
    static Shared<Camera_Free> Create(ComPtr<Device> device, ComPtr<DeviceContext> context);
    Shared<GameObject> Clone(void* arg) override;
    void Free() override;

};

NS_END
