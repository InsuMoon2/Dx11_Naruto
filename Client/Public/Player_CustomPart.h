#pragma once

#include "PartObject.h"

NS_BEGIN(Engine)
class Shader;
class Model;
NS_END

NS_BEGIN(Client)

class Player_CustomPart : public PartObject
{
    GENERATED_BODY(Player_CustomPart)

public:
    explicit Player_CustomPart(ComPtr<Device> device, ComPtr<DeviceContext> context);
    explicit Player_CustomPart(const Player_CustomPart& rhs);
    virtual ~Player_CustomPart() = default;

public:
    HRESULT Initialize_Prototype() override;
    HRESULT Initialize(void* arg) override;
    void    Priority_Update(float timeDelta) override;
    void    Update(float timeDelta) override;
    void    Late_Update(float timeDelta) override;
    HRESULT Render() override;

    HRESULT Render_Shadow() override;
    HRESULT Bind_ShadowShaderResources();

public:
    //const Matrix* Get_SocketBoneMatrixPtr();

private:
    HRESULT Ready_Components(const wstring& modelAssetTag);
    HRESULT Bind_ShaderResources();

private:
    Shared<Shader>  _shader;
    Shared<Model>   _model;

private:
    Shared<Model>   _masterPoseModel;

public:
    static Shared<GameObject> Create(ComPtr<Device> device, ComPtr<DeviceContext> context);
    virtual Shared<GameObject> Clone(void* arg) override;
    void Free() override;

    

};

NS_END
