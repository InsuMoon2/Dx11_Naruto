#include "pch.h"
#include "EffectMeshObject.h"
#include "Shader.h"
#include "Model.h"
#include "Texture.h"
#include "GameObject_Factory.h"

REGISTER_GAMEOBJECT(EffectMeshObject, Protocol::OBJECT_TYPE_EFFECT_MESH)

EffectMeshObject::EffectMeshObject(ComPtr<Device> device, ComPtr<DeviceContext> context)
    : GameObject(device, context)
{
}

EffectMeshObject::EffectMeshObject(const EffectMeshObject& rhs)
    : GameObject(rhs)
    , _layerDesc(rhs._layerDesc)
    , _shaderCom(rhs._shaderCom)
    , _modelCom(rhs._modelCom)
    , _diffuseTexture(rhs._diffuseTexture)
    , _maskTexture(rhs._maskTexture)
    , _hasMask(rhs._hasMask)
{
}

HRESULT EffectMeshObject::Initialize_Prototype()
{
    return GameObject::Initialize_Prototype();
}

HRESULT EffectMeshObject::Initialize(void* arg)
{
    if (!arg)
        return E_FAIL;

    FEffectMeshDesc* desc = static_cast<FEffectMeshDesc*>(arg);
    CHECK_FAILED(GameObject::Initialize(desc), E_FAIL);

    _layerDesc = desc->layerDesc;
    CHECK_FAILED(Ready_Components(), E_FAIL);

    return S_OK;
}

void EffectMeshObject::Priority_Update(float timeDelta)
{
    GameObject::Priority_Update(timeDelta);
}

void EffectMeshObject::Update(float timeDelta)
{
    GameObject::Update(timeDelta);

    Update_Rotation(timeDelta);
}

void EffectMeshObject::Late_Update(float timeDelta)
{
    GameObject::Late_Update(timeDelta);

    if (_layerDesc.blendMode == EEffectBlendMode::Opaque)
        GAME->Add_RenderGroup(ERenderGroup::NonBlend, GetSharedPtr());
    else
        GAME->Add_RenderGroup(ERenderGroup::Blend, GetSharedPtr());
}

HRESULT EffectMeshObject::Render()
{
    GameObject::Render();

    if (!_modelCom || !_shaderCom) return S_OK;

    CHECK_FAILED(Bind_ShaderResources(), E_FAIL);

    for (size_t i = 0; i < _modelCom->Get_NumMeshes(); ++i)
    {
        CHECK_FAILED(_shaderCom->Begin_Pass(Resolve_PassIndex()), E_FAIL);
        CHECK_FAILED(_modelCom->Render(static_cast<uint32>(i)), E_FAIL);
    }

    return S_OK;
}

HRESULT EffectMeshObject::Bind_ShaderResources()
{
    Vec2 uvOffset = _layerDesc.uvScrollSpeed * _elapsed;

    CHECK_FAILED(_shaderCom->Bind_Matrix("g_WorldMatrix", &_transformCom->Get_WorldMatrix()), E_FAIL);
    CHECK_FAILED(_shaderCom->Bind_Matrix("g_ViewMatrix", GAME->Get_Transform(ETransformState::View)), E_FAIL);
    CHECK_FAILED(_shaderCom->Bind_Matrix("g_ProjMatrix", GAME->Get_Transform(ETransformState::Proj)), E_FAIL);

    CHECK_FAILED(GAME->Bind_CamPosition(_shaderCom, "g_CamPosition"), E_FAIL);

    CHECK_FAILED(_shaderCom->Bind_RawValue("g_UVOffset", &uvOffset, sizeof(Vec2)), E_FAIL);
    CHECK_FAILED(_shaderCom->Bind_RawValue("g_UVTiling", &_layerDesc.uvTiling, sizeof(Vec2)), E_FAIL);
    CHECK_FAILED(_shaderCom->Bind_RawValue("g_ColorTint", &_layerDesc.colorTint, sizeof(Vec4)), E_FAIL);
    CHECK_FAILED(_shaderCom->Bind_RawValue("g_Opacity", &_layerDesc.opacity, sizeof(float)), E_FAIL);
    CHECK_FAILED(_shaderCom->Bind_RawValue("g_FresnelPower", &_layerDesc.fresnelPower, sizeof(float)), E_FAIL);
    CHECK_FAILED(_shaderCom->Bind_RawValue("g_FresnelMultiplier", &_layerDesc.fresnelMultiplier, sizeof(float)), E_FAIL);

    if (_diffuseTexture)
        CHECK_FAILED(_diffuseTexture->Bind_SRV(_shaderCom, "g_DiffuseTexture", 0), E_FAIL);

    if (_maskTexture)
        CHECK_FAILED(_maskTexture->Bind_SRV(_shaderCom, "g_MaskTexture", 0), E_FAIL);

    return S_OK;
}

HRESULT EffectMeshObject::Resolve_Resources()
{
    if (!_layerDesc.modelGuid.empty() && !_modelCom)
    {
        uint32 modelKey = static_cast<uint32>(hash<string>{}(_layerDesc.modelGuid));
        CHECK_FAILED(Add_Component(modelKey, _modelCom), E_FAIL);
    }

    if (!_layerDesc.diffuseTextureGuid.empty() && !_diffuseTexture)
    {
        uint32 texKey = static_cast<uint32>(hash<string>{}(_layerDesc.diffuseTextureGuid));
        CHECK_FAILED(Add_Component(texKey, _diffuseTexture), E_FAIL);
    }

    if (!_layerDesc.maskTextureGuid.empty() && !_maskTexture)
    {
        uint32 texKey = static_cast<uint32>(hash<string>{}(_layerDesc.maskTextureGuid));
        CHECK_FAILED(Add_Component(texKey, _maskTexture), E_FAIL);
        _hasMask = true;
    }

    return S_OK;
}

void EffectMeshObject::Update_Rotation(float timeDelta)
{
    if (abs(_layerDesc.rotationSpeed) <= 0.0001f)
        return;

    _accumulatedRotation += _layerDesc.rotationSpeed * timeDelta;

    Vec3 axis = _layerDesc.rotationAxis;

    if (axis.LengthSquared() <= FLT_EPSILON)
        axis = Vec3::Up; // 회전축이 비어 있으면 기본 Y축 사용

    axis.Normalize();

    _accumulatedRotation += _layerDesc.rotationSpeed * timeDelta; 
    _transformCom->Turn(axis, _layerDesc.rotationSpeed * timeDelta);
}

HRESULT EffectMeshObject::Ready_Components()
{
    CHECK_FAILED(Resolve_Resources(), E_FAIL);
    CHECK_FAILED(Add_Component(Protocol::COMPONENT_TYPE_SHADER_EFFECT_MESH, _shaderCom), E_FAIL);

    return S_OK;
}

uint32 EffectMeshObject::Resolve_PassIndex() const
{
    int offset = _hasMask ? 0 : 3;
    if (_layerDesc.blendMode == EEffectBlendMode::Opaque)
    {
        return 2;
    }

    return (int)_layerDesc.blendMode + offset;
}

Shared<GameObject> EffectMeshObject::Create(ComPtr<Device> device, ComPtr<DeviceContext> context)
{
    auto instance = make_shared<EffectMeshObject>(device, context);

    if (FAILED(instance->Initialize_Prototype()))
    {
        MSG_BOX("Failed to Created : EffectMeshObject");
        return nullptr;
    }

    return instance;
}

Shared<GameObject> EffectMeshObject::Clone(void* arg)
{
    auto clone = make_shared<EffectMeshObject>(*this);

    if (FAILED(clone->Initialize(arg)))
    {
        MSG_BOX("Failed to Cloned : EffectMeshObject");
        return nullptr;
    }

    return clone;
}

void EffectMeshObject::Free()
{
    GameObject::Free();
}
