#pragma once

#include "GameObject.h"

NS_BEGIN(Engine)

class ENGINE_DLL Camera abstract : public GameObject
{
    GENERATED_BODY(Camera)

public:
    struct FCameraDesc : public FGameObjectDesc
    {
        Vec3    eye = { 0.f, 5.f, -10.f };
        Vec3    at = { 0.f, 0.f, 0.f };
        float   fovY = XM_PIDIV4;       // 45도
        float   nearZ = 0.1f;
        float   farZ = 1000.f;
    };

protected:
    explicit Camera(ComPtr<Device> device, ComPtr<DeviceContext> context);
    explicit Camera(const Camera& rhs);
    virtual ~Camera() = default;

public:
    HRESULT Initialize_Prototype() override;
    HRESULT Initialize(void* arg) override;
    void    BeginPlay() override;

    void    Priority_Update(float timeDelta) override;
    void    Update(float timeDelta) override;
    void    Late_Update(float timeDelta) override;
    HRESULT Render() override;

public:
    void Set_InputEnabled(bool enabled) { _inputEnabled = enabled; }
    bool Is_InputEnabled() const { return _inputEnabled; }

public:
    float Get_FovY() const { return _fovY; }
    float Get_NearZ() const { return _nearZ; }
    float Get_FarZ() const { return _farZ; }

public: /* Camera Shake */
    virtual void Request_CameraShake(const FCameraShakeDesc& request) {}
    virtual void Stop_CameraShake(const string& tag = "") {}
    virtual void Clear_CameraShake() {}

protected:
    // View + Proj 행렬을 PipeLine에 세팅
    void    Update_TransformMatrices();

protected:
    float   _fovY     = {};
    float   _nearZ    = {};
    float   _farZ     = {};
    float   _aspect   = {};

    bool    _inputEnabled = true;

public:
    virtual Shared<GameObject> Clone(void* arg) = 0;
    virtual void Free() override;

};

NS_END
