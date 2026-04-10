#include "pch.h"
#include "EffectBilldboardObject.h"
#include "Shader.h"
#include "Texture.h"
#include "VIBuffer_Rect.h"
#include "GameObject_Factory.h"
#include "GameInstance.h"

REGISTER_GAMEOBJECT(EffectBillboardObject, Protocol::OBJECT_TYPE_EFFECT_BILLBOARD)

EffectBillboardObject::EffectBillboardObject(ComPtr<Device> device, ComPtr<DeviceContext> context)
    : GameObject(device, context)
{
}

EffectBillboardObject::EffectBillboardObject(const EffectBillboardObject& rhs)
    : GameObject(rhs)
    , _shaderCom(rhs._shaderCom)
    , _bufferCom(rhs._bufferCom)
    , _baseTextureCom(rhs._baseTextureCom)
    , _ringTextureCom(rhs._ringTextureCom)
    , _layerDesc(rhs._layerDesc)
    , _resolvedBaseTextureGuid(rhs._resolvedBaseTextureGuid)
    , _resolvedRingTextureGuid(rhs._resolvedRingTextureGuid)
{
}

HRESULT EffectBillboardObject::Initialize_Prototype()
{
    return GameObject::Initialize_Prototype();
}

HRESULT EffectBillboardObject::Initialize(void* arg)
{
    auto* desc = static_cast<FEffectBillboardDesc*>(arg);
    CHECK_NULL(desc, E_FAIL);
    CHECK_FAILED(GameObject::Initialize(desc), E_FAIL);

    _layerDesc = desc->layerDesc;

    CHECK_FAILED(Add_Component(Protocol::COMPONENT_TYPE_SHADER_BILLBOARD, _shaderCom), E_FAIL);
    CHECK_FAILED(Add_Component(Protocol::COMPONENT_TYPE_RECT, _bufferCom), E_FAIL);
    CHECK_FAILED(Resolve_Textures(), E_FAIL);

    Apply_BaseTransform(_layerDesc.base);

    return S_OK;
}

void EffectBillboardObject::Apply_LayerDesc(const FEffectLayerDesc& layerDesc)
{
    _layerDesc = layerDesc;
    Apply_BaseTransform(_layerDesc.base);
    Resolve_Textures();
}

void EffectBillboardObject::Apply_BaseTransform(const FEffectLayerBase& baseDesc)
{
    CHECK_NULL(_transformCom);

    _transformCom->Set_LocalPosition(baseDesc.localPosition);
    _transformCom->Set_LocalEulerAngles(
        baseDesc.localRotation.x,
        baseDesc.localRotation.y,
        baseDesc.localRotation.z);
    _transformCom->Set_LocalScale(baseDesc.localScale);
}

void EffectBillboardObject::Update(float timeDelta)
{
    GameObject::Update(timeDelta);


}

void EffectBillboardObject::Late_Update(float timeDelta)
{
    GameObject::Late_Update(timeDelta);

    if (_layerDesc.billboard.billboardToCamera)
    {
        Update_BillboardRotation();
    }

    if (_layerDesc.billboard.blendMode == EEffectBlendMode::Opaque)
        GAME->Add_RenderGroup(ERenderGroup::NonBlend, GetSharedPtr());
    else
        GAME->Add_RenderGroup(ERenderGroup::Blend, GetSharedPtr());
}
void EffectBillboardObject::Update_BillboardRotation()
{
    CHECK_NULL(_transformCom);

    const Vec4* camPos4 = GAME->Get_CamPosition();
    CHECK_NULL(camPos4);

    const Vec3 camPos(camPos4->x, camPos4->y, camPos4->z);
    _transformCom->LookAt(camPos);
}

HRESULT EffectBillboardObject::Render()
{
    GameObject::Render();

    CHECK_NULL(_shaderCom, E_FAIL);
    CHECK_NULL(_bufferCom, E_FAIL);

    CHECK_FAILED(Bind_ShaderResources(), E_FAIL);
    CHECK_FAILED(_shaderCom->Begin_Pass(Resolve_PassIndex()), E_FAIL);
    CHECK_FAILED(_bufferCom->Bind_Resources(), E_FAIL);
    CHECK_FAILED(_bufferCom->Render(), E_FAIL);

    return S_OK;
}

HRESULT EffectBillboardObject::Bind_ShaderResources()
{
    CHECK_FAILED(_shaderCom->Bind_Matrix("g_WorldMatrix", &_transformCom->Get_WorldMatrix()), E_FAIL);
    CHECK_FAILED(_shaderCom->Bind_Matrix("g_ViewMatrix", GAME->Get_Transform(ETransformState::View)), E_FAIL);
    CHECK_FAILED(_shaderCom->Bind_Matrix("g_ProjMatrix", GAME->Get_Transform(ETransformState::Proj)), E_FAIL);

    CHECK_NULL(_baseTextureCom, E_FAIL);
    CHECK_FAILED(_baseTextureCom->Bind_SRV(_shaderCom, "g_BaseTexture", 0), E_FAIL);

    const int useRing = (_layerDesc.billboard.useRing && _ringTextureCom) ? 1 : 0;
    CHECK_FAILED(_shaderCom->Bind_RawValue("g_UseRing", &useRing, sizeof(int)), E_FAIL);
    CHECK_FAILED(_shaderCom->Bind_RawValue("g_BaseTint", &_layerDesc.billboard.baseTint, sizeof(Vec4)), E_FAIL);
    CHECK_FAILED(_shaderCom->Bind_RawValue("g_RingTint", &_layerDesc.billboard.ringTint, sizeof(Vec4)), E_FAIL);
    CHECK_FAILED(_shaderCom->Bind_RawValue("g_BaseOpacity", &_layerDesc.billboard.baseOpacity, sizeof(float)), E_FAIL);
    CHECK_FAILED(_shaderCom->Bind_RawValue("g_RingOpacity", &_layerDesc.billboard.ringOpacity, sizeof(float)), E_FAIL);

    if (_layerDesc.billboard.useRing && _ringTextureCom)
    {
        CHECK_FAILED(_ringTextureCom->Bind_SRV(_shaderCom, "g_RingTexture", 0), E_FAIL);
    }
    else
    {
        CHECK_FAILED(_shaderCom->Bind_SRV("g_RingTexture", nullptr), E_FAIL);
    }
        

    return S_OK;
}

HRESULT EffectBillboardObject::Resolve_Textures()
{
    if (_layerDesc.billboard.baseTextureGuid.empty())
    {
        _baseTextureCom.reset();
        _resolvedBaseTextureGuid.clear();
    }
    else if (_resolvedBaseTextureGuid != _layerDesc.billboard.baseTextureGuid || !_baseTextureCom)
    {
        CHECK_FAILED(Resolve_TextureComponent(_layerDesc.billboard.baseTextureGuid, _baseTextureCom), E_FAIL);
        _resolvedBaseTextureGuid = _layerDesc.billboard.baseTextureGuid;
    }

    if (_layerDesc.billboard.ringTextureGuid.empty())
    {
        _ringTextureCom.reset();
        _resolvedRingTextureGuid.clear();
    }
    else if (_layerDesc.billboard.ringTextureGuid == _resolvedBaseTextureGuid && _baseTextureCom)
    {
        _ringTextureCom = _baseTextureCom;
        _resolvedRingTextureGuid = _layerDesc.billboard.ringTextureGuid;
    }
    else if (_resolvedRingTextureGuid != _layerDesc.billboard.ringTextureGuid || !_ringTextureCom)
    {
        CHECK_FAILED(Resolve_TextureComponent(_layerDesc.billboard.ringTextureGuid, _ringTextureCom), E_FAIL);
        _resolvedRingTextureGuid = _layerDesc.billboard.ringTextureGuid;
    }

    return S_OK;
}

HRESULT EffectBillboardObject::Resolve_TextureComponent(const string& textureGuid, Shared<Texture>& outTextureCom)
{
    if (textureGuid.empty())
        return E_FAIL;

    const uint32 texKey = static_cast<uint32>(hash<string>{}(textureGuid));

    if (auto existing = Get_Component(texKey))
    {
        outTextureCom = static_pointer_cast<Texture>(existing);
        return S_OK;
    }

    const wstring resolvedPath = GAME->Resolve_AssetPath(textureGuid);
    if (resolvedPath.empty())
        return E_FAIL;

    auto proto = Texture::Create(_device, _context, resolvedPath, 1);
    CHECK_NULL(proto, E_FAIL);

    CHECK_FAILED(GAME->Add_Component_Prototype(0, texKey, proto), E_FAIL);
    CHECK_FAILED(Add_Component(0, texKey, outTextureCom), E_FAIL);

    return S_OK;
}

uint32 EffectBillboardObject::Resolve_PassIndex() const
{
    switch (_layerDesc.billboard.blendMode)
    {
    case EEffectBlendMode::Translucent:
        return 0;
    case EEffectBlendMode::Additive:
        return 1;
    case EEffectBlendMode::Opaque:
        return 2;
    default:
        return 1;
    }
}

Shared<GameObject> EffectBillboardObject::Create(ComPtr<Device> device, ComPtr<DeviceContext> context)
{
    auto instance = make_shared<EffectBillboardObject>(device, context);

    if (FAILED(instance->Initialize_Prototype()))
    {
        MSG_BOX("Failed to Created : EffectBillboardObject");
        return nullptr;
    }

    return instance;
}

Shared<GameObject> EffectBillboardObject::Clone(void* arg)
{
    auto clone = make_shared<EffectBillboardObject>(*this);

    if (FAILED(clone->Initialize(arg)))
    {
        MSG_BOX("Failed to Cloned : EffectBillboardObject");
        return nullptr;
    }

    return clone;
}

void EffectBillboardObject::Free()
{
    GameObject::Free();
}
