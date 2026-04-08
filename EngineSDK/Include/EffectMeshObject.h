#pragma once

#include "GameObject.h"
#include "EffectAsset_Types.h"

NS_BEGIN(Engine)

class Shader;
class Model;
class Texture;

class ENGINE_DLL EffectMeshObject : public GameObject
{
public:
    struct FEffectMeshDesc : public FGameObjectDesc
    {
        FEffectMeshLayerDesc layerDesc;
    };

public:
    explicit EffectMeshObject(ComPtr<Device> device, ComPtr<DeviceContext> context);
    explicit EffectMeshObject(const EffectMeshObject& rhs);
    virtual ~EffectMeshObject() = default;

public:
    virtual HRESULT Initialize_Prototype() override;
    virtual HRESULT Initialize(void* arg) override;
    virtual void    Priority_Update(float timeDelta) override;
    virtual void    Update(float timeDelta) override;
    virtual void    Late_Update(float timeDelta) override;
    virtual HRESULT Render() override;

    virtual HRESULT Bind_ShaderResources() override;

    HRESULT Resolve_Resources();

public:
    void Set_ElapsedTime(float time) { _elapsed = time; }
    float Get_ElapsedTime() const { return _elapsed; }

    void Update_Rotation(float timeDelta);

    const FEffectMeshLayerDesc& Get_LayerDesc() const { return _layerDesc; }

private:
    HRESULT Ready_Components();

    uint32 Resolve_PassIndex() const;

private:
    FEffectMeshLayerDesc _layerDesc;
    Shared<Shader>  _shaderCom;
    Shared<Model>   _modelCom;
    Shared<Texture> _diffuseTexture;
    Shared<Texture> _maskTexture;

    float _elapsed = 0.f;                     
    float _accumulatedRotation = 0.f;         
    bool  _hasMask = false;                   

public:
    static Shared<GameObject> Create(ComPtr<Device> device, ComPtr<DeviceContext> context);
    virtual Shared<GameObject> Clone(void* arg) override;
    virtual void Free() override;
};

NS_END
