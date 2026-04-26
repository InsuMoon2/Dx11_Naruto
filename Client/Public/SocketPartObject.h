#pragma once

#include "PartObject.h"

NS_BEGIN(Engine)
class Shader;
class Model;
NS_END

NS_BEGIN(Client)

class SocketPartObject : public PartObject
{
    GENERATED_BODY(SocketPartObject)

public:
    struct FSocketPartDesc : public PartObject::FPartObjectDesc
    {
        const Matrix* socketMatrix = nullptr; 
        Vec3 localOffset = Vec3::Zero; 
        Vec3 localRotation = Vec3::Zero;
        Vec3 localScale = Vec3::One; 
    };

public:
    explicit SocketPartObject(ComPtr<Device> device, ComPtr<DeviceContext> context);
    explicit SocketPartObject(const SocketPartObject& rhs);
    virtual ~SocketPartObject() = default;

public:
    HRESULT Initialize_Prototype() override;
    HRESULT Initialize(void* arg) override;
    void Update(float timeDelta) override;
    void Late_Update(float timeDelta) override;
    HRESULT Render() override;
    HRESULT Render_Shadow() override;

private:
    // Edit 모드처럼 Update가 생략되는 프레임에서도 소켓 파츠 렌더 행렬을 최신 상태로 맞추기 위해 호출한다.
    void Refresh_CombinedWorldMatrix();

    HRESULT Ready_Components(const wstring& modelAssetTag);
    HRESULT Bind_ShaderResources();
    HRESULT Bind_ShadowShaderResources();

private:
    Shared<Shader> _shader; 
    Shared<Model> _model; 

    const Matrix* _socketMatrix = nullptr;
    Vec3 _localOffset = Vec3::Zero; 
    Vec3 _localRotation = Vec3::Zero;
    Vec3 _localScale = Vec3::One;

public:
    static Shared<GameObject> Create(ComPtr<Device> device, ComPtr<DeviceContext> context);
    Shared<GameObject> Clone(void* arg) override;
    void Free() override;
};

NS_END
