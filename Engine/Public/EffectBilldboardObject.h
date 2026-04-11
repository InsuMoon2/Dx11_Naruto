#pragma once

#include "GameObject.h"
#include "EffectAsset_Types.h"

NS_BEGIN(Engine)

class Shader;
class Texture;
class VIBuffer_Rect;

class ENGINE_DLL EffectBillboardObject : public GameObject
{
    GENERATED_BODY(EffectBillboardObject)

public:
    struct FEffectBillboardDesc : public GameObject::FGameObjectDesc
    {
        FEffectLayerDesc layerDesc; 
    };

public:
    explicit EffectBillboardObject(ComPtr<Device> device, ComPtr<DeviceContext> context);
    explicit EffectBillboardObject(const EffectBillboardObject& rhs);
    virtual ~EffectBillboardObject() = default;

public:
    HRESULT Initialize_Prototype() override;
    HRESULT Initialize(void* arg) override;
    void    Update(float timeDelta) override;
    void    Late_Update(float timeDelta) override;
    HRESULT Render() override;
    bool    Should_ExcludeFromEditorSnapshot() const override { return true; }
    HRESULT Bind_ShaderResources() override;

public:
    void Apply_LayerDesc(const FEffectLayerDesc& layerDesc);

private:
    void Update_BillboardRotation();
    HRESULT Resolve_Textures();
    HRESULT Resolve_TextureComponent(const string& textureGuid, Shared<Texture>& outTextureCom);
    uint32 Resolve_PassIndex() const;
    void Apply_BaseTransform(const FEffectLayerBase& baseDesc);

private:
    Shared<Shader>        _shaderCom;
    Shared<VIBuffer_Rect> _bufferCom;
    Shared<Texture>       _baseTextureCom;
    Shared<Texture>       _ringTextureCom;

    FEffectLayerDesc      _layerDesc{}; 
    string                _resolvedBaseTextureGuid;
    string                _resolvedRingTextureGuid;

public:
    static Shared<GameObject> Create(ComPtr<Device> device, ComPtr<DeviceContext> context);
    Shared<GameObject> Clone(void* arg) override;
    void Free() override;
};

NS_END
