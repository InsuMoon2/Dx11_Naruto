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
    Apply_BaseTransform(_layerDesc.base);

    return S_OK;
}

void EffectMeshObject::Apply_LayerDesc(const FEffectLayerDesc& layerDesc)
{
    _layerDesc = layerDesc;
    Apply_BaseTransform(_layerDesc.base);
}

void EffectMeshObject::Apply_BaseTransform(const FEffectLayerBase& baseDesc)
{
    CHECK_NULL(_transformCom);

    _transformCom->Set_LocalPosition(baseDesc.localPosition);
    _transformCom->Set_LocalEulerAngles(
        baseDesc.localRotation.x,
        baseDesc.localRotation.y,
        baseDesc.localRotation.z);
    _transformCom->Set_LocalScale(baseDesc.localScale);
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

    if (_layerDesc.mesh.blendMode == EEffectBlendMode::Opaque)
        GAME->Add_RenderGroup(ERenderGroup::NonBlend, GetSharedPtr());
    else
        GAME->Add_RenderGroup(ERenderGroup::Blend, GetSharedPtr());
}

HRESULT EffectMeshObject::Render()
{
    GameObject::Render();

    if (!_modelCom || !_shaderCom)
        return S_OK;

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
    Vec2 uvOffset = _layerDesc.mesh.uvScrollSpeed * _elapsed;
    const int forceVisiblePreview = _forceVisiblePreview ? 1 : 0;

    CHECK_FAILED(_shaderCom->Bind_Matrix("g_WorldMatrix", &_transformCom->Get_WorldMatrix()), E_FAIL);
    CHECK_FAILED(_shaderCom->Bind_Matrix("g_ViewMatrix", GAME->Get_Transform(ETransformState::View)), E_FAIL);
    CHECK_FAILED(_shaderCom->Bind_Matrix("g_ProjMatrix", GAME->Get_Transform(ETransformState::Proj)), E_FAIL);

    CHECK_FAILED(GAME->Bind_CamPosition(_shaderCom, "g_CamPosition"), E_FAIL);

    CHECK_FAILED(_shaderCom->Bind_RawValue("g_UVOffset", &uvOffset, sizeof(Vec2)), E_FAIL);
    CHECK_FAILED(_shaderCom->Bind_RawValue("g_UVTiling", &_layerDesc.mesh.uvTiling, sizeof(Vec2)), E_FAIL);
    CHECK_FAILED(_shaderCom->Bind_RawValue("g_ColorTint", &_layerDesc.mesh.colorTint, sizeof(Vec4)), E_FAIL);
    CHECK_FAILED(_shaderCom->Bind_RawValue("g_Opacity", &_layerDesc.mesh.opacity, sizeof(float)), E_FAIL);
    CHECK_FAILED(_shaderCom->Bind_RawValue("g_FresnelPower", &_layerDesc.mesh.fresnelPower, sizeof(float)), E_FAIL);
    CHECK_FAILED(_shaderCom->Bind_RawValue("g_FresnelMultiplier", &_layerDesc.mesh.fresnelMultiplier, sizeof(float)), E_FAIL);
    CHECK_FAILED(_shaderCom->Bind_RawValue("g_ForceVisiblePreview", &forceVisiblePreview, sizeof(int)), E_FAIL);

	if (_diffuseTexture)
	{
		CHECK_FAILED(_diffuseTexture->Bind_SRV(_shaderCom, "g_DiffuseTexture", 0), E_FAIL);
	}
	else
	{
		CHECK_FAILED(_shaderCom->Bind_SRV("g_DiffuseTexture", nullptr), E_FAIL);
	}

	if (_maskTexture)
	{
		CHECK_FAILED(_maskTexture->Bind_SRV(_shaderCom, "g_MaskTexture", 0), E_FAIL);
	}
	else
	{
		CHECK_FAILED(_shaderCom->Bind_SRV("g_MaskTexture", nullptr), E_FAIL);
	}


    return S_OK;
}

HRESULT EffectMeshObject::Resolve_Resources()
{
    _hasMask = !_layerDesc.mesh.maskTextureGuid.empty();

    if (!_layerDesc.mesh.modelGuid.empty() && !_modelCom)
    {
        uint32 modelKey = static_cast<uint32>(hash<string>{}(_layerDesc.mesh.modelGuid));
        if (FAILED(Add_Component(modelKey, _modelCom)))
        {
            wstring resolvedPath = GAME->Resolve_AssetPath(_layerDesc.mesh.modelGuid);
            if (!resolvedPath.empty())
            {
				Matrix scaleMatrix = Matrix::CreateScale(0.001f);
				Matrix rotationMatrix = Matrix::CreateRotationY(XMConvertToRadians(180.f));
				Matrix preTransform = scaleMatrix * rotationMatrix;

                auto proto = Model::Create(
					_device, _context, EMeshVertexType::StaticMesh, Utils::ToString(resolvedPath), preTransform);

                if (proto)
                {
                    GAME->Add_Component_Prototype(0, modelKey, proto);
                    CHECK_FAILED(Add_Component(0, modelKey, _modelCom), E_FAIL);
                }
            }
        }
    }

    if (!_layerDesc.mesh.diffuseTextureGuid.empty() && !_diffuseTexture)
    {
        uint32 texKey = static_cast<uint32>(hash<string>{}(_layerDesc.mesh.diffuseTextureGuid));
        if (FAILED(Add_Component(texKey, _diffuseTexture)))
        {
            wstring resolvedPath = GAME->Resolve_AssetPath(_layerDesc.mesh.diffuseTextureGuid);
            if (!resolvedPath.empty())
            {
                auto proto = Texture::Create(_device, _context, resolvedPath, 1);
                if (proto)
                {
                    GAME->Add_Component_Prototype(0, texKey, proto);
                    CHECK_FAILED(Add_Component(0, texKey, _diffuseTexture), E_FAIL);
                }
            }
        }
    }

	// Diffuse와 Mask가 같은 GUID면 Texture 컴포넌트 재사용
	if (!_layerDesc.mesh.maskTextureGuid.empty() &&
		_layerDesc.mesh.maskTextureGuid == _layerDesc.mesh.diffuseTextureGuid)
	{
		_maskTexture = _diffuseTexture;
		return S_OK;
	}

    if (!_layerDesc.mesh.maskTextureGuid.empty() && !_maskTexture)
    {
        uint32 texKey = static_cast<uint32>(hash<string>{}(_layerDesc.mesh.maskTextureGuid));
        if (FAILED(Add_Component(texKey, _maskTexture)))
        {
            wstring resolvedPath = GAME->Resolve_AssetPath(_layerDesc.mesh.maskTextureGuid);
            if (!resolvedPath.empty())
            {
                auto proto = Texture::Create(_device, _context, resolvedPath, 1);
                if (proto)
                {
                    GAME->Add_Component_Prototype(0, texKey, proto);
                    CHECK_FAILED(Add_Component(0, texKey, _maskTexture), E_FAIL);
                }
            }
        }
    }

    return S_OK;
}

void EffectMeshObject::Update_Rotation(float timeDelta)
{
    if (abs(_layerDesc.mesh.rotationSpeed) <= 0.0001f)
        return;

    Vec3 axis = _layerDesc.mesh.rotationAxis;
    if (axis.LengthSquared() <= FLT_EPSILON)
        axis = Vec3::Up;

    axis.Normalize();

    _accumulatedRotation += _layerDesc.mesh.rotationSpeed * timeDelta;
    _transformCom->Turn(axis, _layerDesc.mesh.rotationSpeed * timeDelta);
}

HRESULT EffectMeshObject::Ready_Components()
{
    CHECK_FAILED(Resolve_Resources(), E_FAIL);
    CHECK_FAILED(Add_Component(Protocol::COMPONENT_TYPE_SHADER_EFFECT_MESH, _shaderCom), E_FAIL);
    return S_OK;
}

uint32 EffectMeshObject::Resolve_PassIndex() const
{
    const bool twoSided = _layerDesc.mesh.twoSided;

    if (!twoSided)
    {
        if (_layerDesc.mesh.blendMode == EEffectBlendMode::Opaque)
            return 2;

        if (_hasMask)
            return static_cast<uint32>(_layerDesc.mesh.blendMode);

        return 3 + static_cast<uint32>(_layerDesc.mesh.blendMode);
    }

    if (_layerDesc.mesh.blendMode == EEffectBlendMode::Opaque)
        return 7;

    if (_hasMask)
        return 5 + static_cast<uint32>(_layerDesc.mesh.blendMode);

    return 8 + static_cast<uint32>(_layerDesc.mesh.blendMode);
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
