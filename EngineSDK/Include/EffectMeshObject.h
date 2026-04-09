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
        FEffectLayerDesc layerDesc;
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
    void  Set_ElapsedTime(float time) { _elapsed = time; }
    float Get_ElapsedTime() const { return _elapsed; }

    void Apply_LayerDesc(const FEffectLayerDesc& layerDesc);
    void Apply_BaseTransform(const FEffectLayerBase& baseDesc);
    // 에디터 프리뷰에서만 텍스처/알파를 무시하고 메시 실루엣을 강제로 보이게 할 때 호출한다.
    void Set_ForceVisiblePreview(bool enabled) { _forceVisiblePreview = enabled; }

    void Update_Rotation(float timeDelta);

    const FEffectLayerDesc& Get_LayerDesc() const { return _layerDesc; }

private:
    HRESULT Ready_Components();

    uint32 Resolve_PassIndex() const;

private:
    Shared<Shader>  _shaderCom;
    Shared<Model>   _modelCom;
    Shared<Texture> _diffuseTexture;
    Shared<Texture> _maskTexture;

    FEffectLayerDesc _layerDesc;

    float _elapsed = 0.f;                     
    float _accumulatedRotation = 0.f;         
    bool  _hasMask = false;                   
    // 이펙트 뷰 진단용으로만 쓰는 강제 가시화 플래그다. 런타임 기본 동작은 false를 유지한다.
    bool  _forceVisiblePreview = false;

public:
    static Shared<GameObject> Create(ComPtr<Device> device, ComPtr<DeviceContext> context);
    virtual Shared<GameObject> Clone(void* arg) override;
    virtual void Free() override;
};

NS_END
