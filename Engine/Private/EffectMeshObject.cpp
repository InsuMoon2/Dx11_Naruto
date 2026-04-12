#include "pch.h"
#include "EffectMeshObject.h"
#include "Shader.h"
#include "Model.h"
#include "Texture.h"
#include "GameObject_Factory.h"

REGISTER_GAMEOBJECT(EffectMeshObject, Protocol::OBJECT_TYPE_EFFECT_MESH)

/* Additive 메쉬 레이어라도 opacity를 일반 알파처럼 느끼게 하고 싶을 때 런타임 블렌드 모드를 바꿔준다. */
static EEffectBlendMode Resolve_RuntimeBlendMode(const FEffectMeshLayerDesc& meshDesc)
{
    if (meshDesc.blendMode == EEffectBlendMode::Additive &&
        meshDesc.useOpacityAsTransparency)
    {
        return EEffectBlendMode::Translucent;
    }

    return meshDesc.blendMode;
}

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
    , _emissiveTexture(rhs._emissiveTexture)
    , _opacityTexture(rhs._opacityTexture)
    , _opacitySubUvTexture(rhs._opacitySubUvTexture)
    , _opacityGradationTexture(rhs._opacityGradationTexture)
    , _emissiveGradationTexture(rhs._emissiveGradationTexture)
    , _uvDistortionTexture(rhs._uvDistortionTexture)
    , _hasOpacity(rhs._hasOpacity)
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

    _elapsed += timeDelta;
    Update_Rotation(timeDelta);
}

void EffectMeshObject::Late_Update(float timeDelta)
{
    GameObject::Late_Update(timeDelta);

    const EEffectBlendMode runtimeBlendMode = Resolve_RuntimeBlendMode(_layerDesc.mesh);

    if (runtimeBlendMode == EEffectBlendMode::Opaque)
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
    Vec2 uvDistortionOffset = _layerDesc.mesh.uvDistortionSpeed * _elapsed;

    const int forceVisiblePreview = _forceVisiblePreview ? 1 : 0;
    const int shadingMode = static_cast<int>(_layerDesc.mesh.shadingMode);
    const int hasDiffuseTexture = _diffuseTexture ? 1 : 0;
    const int hasOpacityTexture = _opacityTexture ? 1 : 0;
    const int hasOpacitySubUvTexture = _opacitySubUvTexture ? 1 : 0;
    const int hasOpacityGradationTexture = _opacityGradationTexture ? 1 : 0;
    const int hasEmissiveGradationTexture = _emissiveGradationTexture ? 1 : 0;
    const int hasUvDistortionTexture = _uvDistortionTexture ? 1 : 0;
    const int hasNormalTexture = _normalTexture ? 1 : 0;
    const int hasRoughnessTexture = _roughnessTexture ? 1 : 0;
    const int hasSpecularTexture = _specularTexture ? 1 : 0;

    CHECK_FAILED(_shaderCom->Bind_Matrix("g_WorldMatrix", &_transformCom->Get_WorldMatrix()), E_FAIL);
    CHECK_FAILED(_shaderCom->Bind_Matrix("g_ViewMatrix", GAME->Get_Transform(ETransformState::View)), E_FAIL);
    CHECK_FAILED(_shaderCom->Bind_Matrix("g_ProjMatrix", GAME->Get_Transform(ETransformState::Proj)), E_FAIL);

    CHECK_FAILED(GAME->Bind_CamPosition(_shaderCom, "g_CamPosition"), E_FAIL);

    CHECK_FAILED(_shaderCom->Bind_RawValue("g_UVOffset", &uvOffset, sizeof(Vec2)), E_FAIL);
    CHECK_FAILED(_shaderCom->Bind_RawValue("g_UVTiling", &_layerDesc.mesh.uvTiling, sizeof(Vec2)), E_FAIL);
    CHECK_FAILED(_shaderCom->Bind_RawValue("g_UVDistortionOffset", &uvDistortionOffset, sizeof(Vec2)), E_FAIL);
    CHECK_FAILED(_shaderCom->Bind_RawValue("g_UVDistortionStrength", &_layerDesc.mesh.uvDistortionStrength, sizeof(Vec2)), E_FAIL);
    
    const Vec4 colorTint = _useRuntimeColorTintOverride ? _runtimeColorTint : _layerDesc.mesh.colorTint;
    const float opacity = _useRuntimeOpacityOverride ? _runtimeOpacity : _layerDesc.mesh.opacity;
    const float emissiveStrength = _useRuntimeEmissiveStrengthOverride ? _runtimeEmissiveStrength : _layerDesc.mesh.emissiveStrength;

    CHECK_FAILED(_shaderCom->Bind_RawValue("g_ColorTint", &colorTint, sizeof(Vec4)), E_FAIL);
    CHECK_FAILED(_shaderCom->Bind_RawValue("g_Opacity", &opacity, sizeof(float)), E_FAIL);
    CHECK_FAILED(_shaderCom->Bind_RawValue("g_EmissiveStrength", &emissiveStrength, sizeof(float)), E_FAIL);
    CHECK_FAILED(_shaderCom->Bind_RawValue("g_FresnelPower", &_layerDesc.mesh.fresnelPower, sizeof(float)), E_FAIL);
    CHECK_FAILED(_shaderCom->Bind_RawValue("g_FresnelMultiplier", &_layerDesc.mesh.fresnelMultiplier, sizeof(float)), E_FAIL);

    CHECK_FAILED(_shaderCom->Bind_RawValue("g_ForceVisiblePreview", &forceVisiblePreview, sizeof(int)), E_FAIL);
    CHECK_FAILED(_shaderCom->Bind_RawValue("g_ShadingMode", &shadingMode, sizeof(int)), E_FAIL);
    CHECK_FAILED(_shaderCom->Bind_RawValue("g_HasDiffuseTexture", &hasDiffuseTexture, sizeof(int)), E_FAIL);
    CHECK_FAILED(_shaderCom->Bind_RawValue("g_HasOpacityTexture", &hasOpacityTexture, sizeof(int)), E_FAIL);
    CHECK_FAILED(_shaderCom->Bind_RawValue("g_HasOpacitySubUvTexture", &hasOpacitySubUvTexture, sizeof(int)), E_FAIL);
    CHECK_FAILED(_shaderCom->Bind_RawValue("g_HasOpacityGradationTexture", &hasOpacityGradationTexture, sizeof(int)), E_FAIL);
    CHECK_FAILED(_shaderCom->Bind_RawValue("g_HasEmissiveGradationTexture", &hasEmissiveGradationTexture, sizeof(int)), E_FAIL);
    CHECK_FAILED(_shaderCom->Bind_RawValue("g_HasUVDistortionTexture", &hasUvDistortionTexture, sizeof(int)), E_FAIL);
    CHECK_FAILED(_shaderCom->Bind_RawValue("g_HasNormalTexture", &hasNormalTexture, sizeof(int)), E_FAIL);
    CHECK_FAILED(_shaderCom->Bind_RawValue("g_HasRoughnessTexture", &hasRoughnessTexture, sizeof(int)), E_FAIL);
    CHECK_FAILED(_shaderCom->Bind_RawValue("g_HasSpecularTexture", &hasSpecularTexture, sizeof(int)), E_FAIL);

    if (_diffuseTexture)
        {CHECK_FAILED(_diffuseTexture->Bind_SRV(_shaderCom, "g_DiffuseTexture", 0), E_FAIL);}
    else
        {CHECK_FAILED(_shaderCom->Bind_SRV("g_DiffuseTexture", nullptr), E_FAIL);}

    if (_maskTexture)
        {CHECK_FAILED(_maskTexture->Bind_SRV(_shaderCom, "g_MaskTexture", 0), E_FAIL);}
    else
        {CHECK_FAILED(_shaderCom->Bind_SRV("g_MaskTexture", nullptr), E_FAIL);}

    if (_emissiveTexture)
        {CHECK_FAILED(_emissiveTexture->Bind_SRV(_shaderCom, "g_EmissiveTexture", 0), E_FAIL);}
    else
        {CHECK_FAILED(_shaderCom->Bind_SRV("g_EmissiveTexture", nullptr), E_FAIL);}

    if (_opacityTexture)
        {CHECK_FAILED(_opacityTexture->Bind_SRV(_shaderCom, "g_OpacityTexture", 0), E_FAIL);}
    else
        {CHECK_FAILED(_shaderCom->Bind_SRV("g_OpacityTexture", nullptr), E_FAIL);}

    if (_opacitySubUvTexture)
        {CHECK_FAILED(_opacitySubUvTexture->Bind_SRV(_shaderCom, "g_OpacitySubUvTexture", 0), E_FAIL);}
    else
        {CHECK_FAILED(_shaderCom->Bind_SRV("g_OpacitySubUvTexture", nullptr), E_FAIL);}

    if (_opacityGradationTexture)
        {CHECK_FAILED(_opacityGradationTexture->Bind_SRV(_shaderCom, "g_OpacityGradationTexture", 0), E_FAIL);}
    else
        {CHECK_FAILED(_shaderCom->Bind_SRV("g_OpacityGradationTexture", nullptr), E_FAIL);}

    if (_emissiveGradationTexture)
        {CHECK_FAILED(_emissiveGradationTexture->Bind_SRV(_shaderCom, "g_EmissiveGradationTexture", 0), E_FAIL);}
    else
        {CHECK_FAILED(_shaderCom->Bind_SRV("g_EmissiveGradationTexture", nullptr), E_FAIL);}

    if (_uvDistortionTexture)
        {CHECK_FAILED(_uvDistortionTexture->Bind_SRV(_shaderCom, "g_UVDistortionTexture", 0), E_FAIL);}
    else
        {CHECK_FAILED(_shaderCom->Bind_SRV("g_UVDistortionTexture", nullptr), E_FAIL);}

    if (_normalTexture)
        {CHECK_FAILED(_normalTexture->Bind_SRV(_shaderCom, "g_NormalTexture", 0), E_FAIL);}
    else
        {CHECK_FAILED(_shaderCom->Bind_SRV("g_NormalTexture", nullptr), E_FAIL);}

    if (_roughnessTexture)
        {CHECK_FAILED(_roughnessTexture->Bind_SRV(_shaderCom, "g_RoughnessTexture", 0), E_FAIL);}
    else
        {CHECK_FAILED(_shaderCom->Bind_SRV("g_RoughnessTexture", nullptr), E_FAIL);}

    if (_specularTexture)
        {CHECK_FAILED(_specularTexture->Bind_SRV(_shaderCom, "g_SpecularTexture", 0), E_FAIL);}
    else
        {CHECK_FAILED(_shaderCom->Bind_SRV("g_SpecularTexture", nullptr), E_FAIL);}

    return S_OK;
}

HRESULT EffectMeshObject::Resolve_Resources()
{
    _hasOpacity =
        !_layerDesc.mesh.opacityTextureGuid.empty() ||
        !_layerDesc.mesh.maskTextureGuid.empty() ||
        !_layerDesc.mesh.opacitySubUvTextureGuid.empty();

    if (!_layerDesc.mesh.modelGuid.empty() && !_modelCom)
    {
        uint32 modelKey = static_cast<uint32>(hash<string>{}(_layerDesc.mesh.modelGuid));
        if (FAILED(Add_Component(modelKey, _modelCom)))
        {
            wstring resolvedPath = GAME->Resolve_AssetPath(_layerDesc.mesh.modelGuid);

            if (resolvedPath.empty())
            {
                LOG_ERROR(
                    "EffectMeshObject model resolve failed. layer='{}', modelGuid='{}'",
                    _layerDesc.base.layerName,
                    _layerDesc.mesh.modelGuid);

                return E_FAIL;
            }

            if (!resolvedPath.empty())
            {
                Matrix scaleMatrix = Matrix::CreateScale(0.0001f);
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

            if (resolvedPath.empty())
            {
                LOG_ERROR(
                    "EffectMeshObject diffuse resolve failed. layer='{}', diffuseGuid='{}'",
                    _layerDesc.base.layerName,
                    _layerDesc.mesh.diffuseTextureGuid);

                return E_FAIL;
            }

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

    if (!_layerDesc.mesh.maskTextureGuid.empty() && !_maskTexture)
    {
        if (_layerDesc.mesh.maskTextureGuid == _layerDesc.mesh.diffuseTextureGuid && _diffuseTexture)
        {
            _maskTexture = _diffuseTexture;
        }
        else
        {
            uint32 texKey = static_cast<uint32>(hash<string>{}(_layerDesc.mesh.maskTextureGuid));
            if (FAILED(Add_Component(texKey, _maskTexture)))
            {
                wstring resolvedPath = GAME->Resolve_AssetPath(_layerDesc.mesh.maskTextureGuid);

                if (resolvedPath.empty())
                {
                    LOG_ERROR(
                        "EffectMeshObject mask resolve failed. layer='{}', maskGuid='{}'",
                        _layerDesc.base.layerName,
                        _layerDesc.mesh.maskTextureGuid);

                    return E_FAIL;
                }

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
    }

    const string emissiveGuid = _layerDesc.mesh.emissiveTextureGuid.empty()
        ? _layerDesc.mesh.diffuseTextureGuid
        : _layerDesc.mesh.emissiveTextureGuid;

    const string opacityGuid = _layerDesc.mesh.opacityTextureGuid.empty()
        ? _layerDesc.mesh.maskTextureGuid
        : _layerDesc.mesh.opacityTextureGuid;

    if (!emissiveGuid.empty() && !_emissiveTexture)
    {
        if (emissiveGuid == _layerDesc.mesh.diffuseTextureGuid && _diffuseTexture)
        {
            _emissiveTexture = _diffuseTexture;
        }
        else if (emissiveGuid == _layerDesc.mesh.maskTextureGuid && _maskTexture)
        {
            _emissiveTexture = _maskTexture;
        }
        else
        {
            uint32 texKey = static_cast<uint32>(hash<string>{}(emissiveGuid));
            if (FAILED(Add_Component(texKey, _emissiveTexture)))
            {
                wstring resolvedPath = GAME->Resolve_AssetPath(emissiveGuid);

                if (resolvedPath.empty())
                {
                    LOG_ERROR(
                        "EffectMeshObject emissive resolve failed. layer='{}', emissiveGuid='{}'",
                        _layerDesc.base.layerName,
                        emissiveGuid);

                    return E_FAIL;
                }

                auto proto = Texture::Create(_device, _context, resolvedPath, 1);
                if (proto)
                {
                    GAME->Add_Component_Prototype(0, texKey, proto);
                    CHECK_FAILED(Add_Component(0, texKey, _emissiveTexture), E_FAIL);
                }
            }
        }
    }

    if (!opacityGuid.empty() && !_opacityTexture)
    {
        if (opacityGuid == _layerDesc.mesh.maskTextureGuid && _maskTexture)
        {
            _opacityTexture = _maskTexture;
        }
        else if (opacityGuid == _layerDesc.mesh.diffuseTextureGuid && _diffuseTexture)
        {
            _opacityTexture = _diffuseTexture;
        }
        else if (opacityGuid == _layerDesc.mesh.emissiveTextureGuid && _emissiveTexture)
        {
            _opacityTexture = _emissiveTexture;
        }
        else
        {
            uint32 texKey = static_cast<uint32>(hash<string>{}(opacityGuid));
            if (FAILED(Add_Component(texKey, _opacityTexture)))
            {
                wstring resolvedPath = GAME->Resolve_AssetPath(opacityGuid);

                if (resolvedPath.empty())
                {
                    LOG_ERROR(
                        "EffectMeshObject opacity resolve failed. layer='{}', opacityGuid='{}'",
                        _layerDesc.base.layerName,
                        opacityGuid);

                    return E_FAIL;
                }

                auto proto = Texture::Create(_device, _context, resolvedPath, 1);
                if (proto)
                {
                    GAME->Add_Component_Prototype(0, texKey, proto);
                    CHECK_FAILED(Add_Component(0, texKey, _opacityTexture), E_FAIL);
                }
            }
        }
    }

    if (!_layerDesc.mesh.opacitySubUvTextureGuid.empty() && !_opacitySubUvTexture)
    {
        if (_layerDesc.mesh.opacitySubUvTextureGuid == _layerDesc.mesh.opacityTextureGuid && _opacityTexture)
        {
            _opacitySubUvTexture = _opacityTexture;
        }
        else if (_layerDesc.mesh.opacitySubUvTextureGuid == _layerDesc.mesh.maskTextureGuid && _maskTexture)
        {
            _opacitySubUvTexture = _maskTexture;
        }
        else
        {
            uint32 texKey = static_cast<uint32>(hash<string>{}(_layerDesc.mesh.opacitySubUvTextureGuid));
            if (FAILED(Add_Component(texKey, _opacitySubUvTexture)))
            {
                wstring resolvedPath = GAME->Resolve_AssetPath(_layerDesc.mesh.opacitySubUvTextureGuid);

                if (resolvedPath.empty())
                {
                    LOG_ERROR(
                        "EffectMeshObject opacity subuv resolve failed. layer='{}', opacitySubUvGuid='{}'",
                        _layerDesc.base.layerName,
                        _layerDesc.mesh.opacitySubUvTextureGuid);

                    return E_FAIL;
                }

                auto proto = Texture::Create(_device, _context, resolvedPath, 1);
                if (proto)
                {
                    GAME->Add_Component_Prototype(0, texKey, proto);
                    CHECK_FAILED(Add_Component(0, texKey, _opacitySubUvTexture), E_FAIL);
                }
            }
        }
    }

    if (!_layerDesc.mesh.opacityGradationTextureGuid.empty() && !_opacityGradationTexture)
    {
        uint32 texKey = static_cast<uint32>(hash<string>{}(_layerDesc.mesh.opacityGradationTextureGuid));
        if (FAILED(Add_Component(texKey, _opacityGradationTexture)))
        {
            wstring resolvedPath = GAME->Resolve_AssetPath(_layerDesc.mesh.opacityGradationTextureGuid);

            if (resolvedPath.empty())
            {
                LOG_ERROR(
                    "EffectMeshObject opacity gradation resolve failed. layer='{}', opacityGradationGuid='{}'",
                    _layerDesc.base.layerName,
                    _layerDesc.mesh.opacityGradationTextureGuid);

                return E_FAIL;
            }

            auto proto = Texture::Create(_device, _context, resolvedPath, 1);
            if (proto)
            {
                GAME->Add_Component_Prototype(0, texKey, proto);
                CHECK_FAILED(Add_Component(0, texKey, _opacityGradationTexture), E_FAIL);
            }
        }
    }

    if (!_layerDesc.mesh.emissiveGradationTextureGuid.empty() && !_emissiveGradationTexture)
    {
        uint32 texKey = static_cast<uint32>(hash<string>{}(_layerDesc.mesh.emissiveGradationTextureGuid));
        if (FAILED(Add_Component(texKey, _emissiveGradationTexture)))
        {
            wstring resolvedPath = GAME->Resolve_AssetPath(_layerDesc.mesh.emissiveGradationTextureGuid);

            if (resolvedPath.empty())
            {
                LOG_ERROR(
                    "EffectMeshObject emissive gradation resolve failed. layer='{}', emissiveGradationGuid='{}'",
                    _layerDesc.base.layerName,
                    _layerDesc.mesh.emissiveGradationTextureGuid);

                return E_FAIL;
            }

            auto proto = Texture::Create(_device, _context, resolvedPath, 1);
            if (proto)
            {
                GAME->Add_Component_Prototype(0, texKey, proto);
                CHECK_FAILED(Add_Component(0, texKey, _emissiveGradationTexture), E_FAIL);
            }
        }
    }

    if (!_layerDesc.mesh.uvDistortionTextureGuid.empty() && !_uvDistortionTexture)
    {
        uint32 texKey = static_cast<uint32>(hash<string>{}(_layerDesc.mesh.uvDistortionTextureGuid));
        if (FAILED(Add_Component(texKey, _uvDistortionTexture)))
        {
            wstring resolvedPath = GAME->Resolve_AssetPath(_layerDesc.mesh.uvDistortionTextureGuid);

            if (resolvedPath.empty())
            {
                LOG_ERROR(
                    "EffectMeshObject uv distortion resolve failed. layer='{}', uvDistortionGuid='{}'",
                    _layerDesc.base.layerName,
                    _layerDesc.mesh.uvDistortionTextureGuid);

                return E_FAIL;
            }

            auto proto = Texture::Create(_device, _context, resolvedPath, 1);
            if (proto)
            {
                GAME->Add_Component_Prototype(0, texKey, proto);
                CHECK_FAILED(Add_Component(0, texKey, _uvDistortionTexture), E_FAIL);
            }
        }
    }

    return S_OK;

}

void EffectMeshObject::Set_RuntimeColorTintOverride(const Vec4& colorTint, bool enabled)
{
    _useRuntimeColorTintOverride = enabled;
    _runtimeColorTint = colorTint;
}
void EffectMeshObject::Set_RuntimeOpacityOverride(float opacity, bool enabled)
{
    _useRuntimeOpacityOverride = enabled;
    _runtimeOpacity = opacity;
}
void EffectMeshObject::Set_RuntimeEmissiveStrengthOverride(float emissiveStrength, bool enabled)
{
    _useRuntimeEmissiveStrengthOverride = enabled;
    _runtimeEmissiveStrength = emissiveStrength;
}

void EffectMeshObject::Update_Rotation(float timeDelta)
{
    if (abs(_layerDesc.mesh.rotationSpeed) <= 0.0001f)
        return;

    Vec3 axis = _layerDesc.mesh.rotationAxis;
    if (axis.LengthSquared() <= FLT_EPSILON)
        axis = Vec3::Up;

    axis.Normalize();

    const float radians = XMConvertToRadians(_layerDesc.mesh.rotationSpeed) * timeDelta;
    Quat deltaRot = Quat::CreateFromAxisAngle(axis, radians);
    _transformCom->Add_LocalRotation(deltaRot);
}

HRESULT EffectMeshObject::Ready_Components()
{
    if (FAILED(Resolve_Resources()))
    {
        LOG_ERROR(
            "EffectMeshObject::Resolve_Resources failed. layer='{}', modelGuid='{}', diffuseGuid='{}', maskGuid='{}'",
            _layerDesc.base.layerName,
            _layerDesc.mesh.modelGuid,
            _layerDesc.mesh.diffuseTextureGuid,
            _layerDesc.mesh.maskTextureGuid);

        return E_FAIL;
    }

    // 메쉬 이펙트 전용 셰이더 프로토타입이 없거나 생성 실패했는지 확인하기 위한 로그
    if (FAILED(Add_Component(Protocol::COMPONENT_TYPE_SHADER_EFFECT_MESH, _shaderCom)))
    {
        LOG_ERROR(
            "EffectMeshObject failed to add shader component. layer='{}', shaderTypeId={}",
            _layerDesc.base.layerName,
            static_cast<uint32>(Protocol::COMPONENT_TYPE_SHADER_EFFECT_MESH));

        return E_FAIL;
    }

    return S_OK;
}

uint32 EffectMeshObject::Resolve_PassIndex() const
{
    const bool twoSided = _layerDesc.mesh.twoSided;
    /* 실제 렌더링 시 사용할 블렌드 모드다. Additive를 강제로 Translucent처럼 보정할 수 있다. */
    const EEffectBlendMode runtimeBlendMode = Resolve_RuntimeBlendMode(_layerDesc.mesh);

    if (!twoSided)
    {
        if (runtimeBlendMode == EEffectBlendMode::Opaque)
            return 2;

        if (_hasOpacity)
            return static_cast<uint32>(runtimeBlendMode);

        return 3 + static_cast<uint32>(runtimeBlendMode);
    }

    if (runtimeBlendMode == EEffectBlendMode::Opaque)
        return 7;

    if (_hasOpacity)
        return 5 + static_cast<uint32>(runtimeBlendMode);

    return 8 + static_cast<uint32>(runtimeBlendMode);
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
