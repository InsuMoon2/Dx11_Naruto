#include "pch.h"
#include "EffectMeshObject.h"
#include "Shader.h"
#include "Model.h"
#include "ModelMaterial.h"
#include "Texture.h"
#include "GameObject_Factory.h"

REGISTER_GAMEOBJECT(EffectMeshObject, Protocol::OBJECT_TYPE_EFFECT_MESH)

/* Additive 메쉬 레이어라도 opacity를 일반 알파처럼 느끼게 하고 싶을 때 런타임 블렌드 모드를 바꿔준다. */
static EEffectBlendMode Resolve_RuntimeBlendMode(const EffectMeshObject::FEffectMeshMaterialRuntimeDesc& meshDesc)
{
    if (meshDesc.blendMode == EEffectBlendMode::Additive &&
        meshDesc.useOpacityAsTransparency)
    {
        return EEffectBlendMode::Translucent;
    }

    return meshDesc.blendMode;
}

/* Effect Mesh Lit 프리뷰/런타임에서 등록된 라이트가 없을 때 검게 죽지 않도록 기본 방향광을 만든다. */
static FLightDesc Make_DefaultEffectMeshLightDesc()
{
    FLightDesc desc{};
    desc.type = ELightType::Directional;
    desc.direction = Vec4(-1.f, -1.f, -1.f, 0.f);
    desc.diffuse = Vec4(1.f, 1.f, 1.f, 1.f);
    desc.ambient = Vec4(0.18f, 0.18f, 0.18f, 1.f);
    desc.specular = Vec4(0.35f, 0.35f, 0.35f, 1.f);

    return desc;
}

/* Effect Mesh Lit 셰이더에 넘길 대표 라이트를 찾을 때 호출한다. Directional이 없으면 기본 방향광으로 검은 프리뷰를 방지한다. */
static FLightDesc Resolve_EffectMeshLightDesc()
{
    for (uint32 i = 0; i < 16; ++i)
    {
        const FLightDesc* lightDesc = GAME->Get_LightDesc(i);
        if (!lightDesc)
            break;

        if (lightDesc->type == ELightType::Directional)
            return *lightDesc;
    }

    return Make_DefaultEffectMeshLightDesc();
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
    , _normalTexture(rhs._normalTexture)
    , _roughnessTexture(rhs._roughnessTexture)
    , _specularTexture(rhs._specularTexture)
    , _materialOverrideResources(rhs._materialOverrideResources)
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
    CHECK_FAILED(Apply_AnimationSettings(), E_FAIL);

    return S_OK;
}

void EffectMeshObject::Apply_LayerDesc(const FEffectLayerDesc& layerDesc)
{
    _layerDesc = layerDesc;
    Apply_BaseTransform(_layerDesc.base);
    Resolve_OverrideResources();
    Apply_AnimationSettings();
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
    Sync_RotationBaseFromCurrentTransform();
}

void EffectMeshObject::Priority_Update(float timeDelta)
{
    GameObject::Priority_Update(timeDelta);
}

void EffectMeshObject::Update(float timeDelta)
{
    GameObject::Update(timeDelta);

    _elapsed += timeDelta;

    if (_modelCom && Is_SkeletalLayer() && _modelCom->Has_Animations())
    {
        _modelCom->Play_Animation(timeDelta, false);
    }

    Update_Rotation(timeDelta);
}

void EffectMeshObject::Late_Update(float timeDelta)
{
    GameObject::Late_Update(timeDelta);

    if (Has_ScreenDistortionPass())
        GAME->Add_RenderGroup(ERenderGroup::ScreenDistortion, GetSharedPtr());
    else if (Has_NonOpaquePass())
        GAME->Add_RenderGroup(ERenderGroup::Blend, GetSharedPtr());
    else
        GAME->Add_RenderGroup(ERenderGroup::NonBlend, GetSharedPtr());
}

HRESULT EffectMeshObject::Render()
{
    GameObject::Render();

    if (!_modelCom || !_shaderCom)
        return S_OK;

    for (size_t i = 0; i < _modelCom->Get_NumMeshes(); ++i)
    {
        CHECK_FAILED(Bind_ShaderResources(static_cast<uint32>(i)), E_FAIL);

        if (Is_SkeletalLayer())
            CHECK_FAILED(_modelCom->Bind_BoneMatrices(_shaderCom, "g_BoneMatrices"), E_FAIL);

        CHECK_FAILED(_shaderCom->Begin_Pass(Resolve_PassIndex(Resolve_RuntimeMeshDesc(static_cast<uint32>(i)))), E_FAIL);
        CHECK_FAILED(_modelCom->Render(static_cast<uint32>(i)), E_FAIL);
    }

    return S_OK;
}

HRESULT EffectMeshObject::Bind_ShaderResources()
{
    return Bind_ShaderResources(0);
}

HRESULT EffectMeshObject::Bind_ShaderResources(uint32 meshIndex)
{
    // Mesh shader variants can differ between static/skeletal effects, so new distortion uniforms are optional.
    auto BindOptionalRaw = [this](const char* name, const void* data, uint32 size)
        {
            _shaderCom->Bind_RawValue(name, data, size);
        };

    auto BindOptionalNullSrv = [this](const char* name)
        {
            _shaderCom->Bind_SRV(name, nullptr);
        };

    const FEffectMeshMaterialRuntimeDesc meshDesc = Resolve_RuntimeMeshDesc(meshIndex);
    const FResolvedMaterialResources* materialResources = Resolve_RuntimeMaterialResources(meshIndex);
    Shared<Texture> diffuseTexture = materialResources ? materialResources->diffuseTexture : _diffuseTexture;
    Shared<Texture> maskTexture = materialResources ? materialResources->maskTexture : _maskTexture;
    Shared<Texture> emissiveTexture = materialResources ? materialResources->emissiveTexture : _emissiveTexture;
    Shared<Texture> opacityTexture = materialResources ? materialResources->opacityTexture : _opacityTexture;
    Shared<Texture> opacitySubUvTexture = materialResources ? materialResources->opacitySubUvTexture : _opacitySubUvTexture;
    Shared<Texture> opacityGradationTexture = materialResources ? materialResources->opacityGradationTexture : _opacityGradationTexture;
    Shared<Texture> emissiveGradationTexture = materialResources ? materialResources->emissiveGradationTexture : _emissiveGradationTexture;
    Shared<Texture> uvDistortionTexture = materialResources ? materialResources->uvDistortionTexture : _uvDistortionTexture;
    Shared<Texture> normalTexture = materialResources ? materialResources->normalTexture : _normalTexture;
    Shared<Texture> roughnessTexture = materialResources ? materialResources->roughnessTexture : _roughnessTexture;
    Shared<Texture> specularTexture = materialResources ? materialResources->specularTexture : _specularTexture;
    Vec2 uvOffset = meshDesc.uvScrollSpeed * _elapsed;
    Vec2 uvDistortionOffset = meshDesc.uvDistortionSpeed * _elapsed;

    const int forceVisiblePreview = _forceVisiblePreview ? 1 : 0;
    const int shadingMode = static_cast<int>(meshDesc.shadingMode);
    const int hasDiffuseTexture = diffuseTexture ? 1 : 0;
    const int hasMaskTexture = maskTexture ? 1 : 0; // 원본 이펙트의 OpacityMask_Map을 실제 alpha cutout에 반영하기 위한 플래그다.
    const int hasOpacityTexture = opacityTexture ? 1 : 0;
    const int hasOpacitySubUvTexture = opacitySubUvTexture ? 1 : 0;
    const int hasOpacityGradationTexture = opacityGradationTexture ? 1 : 0;
    const int hasEmissiveGradationTexture = emissiveGradationTexture ? 1 : 0;
    const int hasUvDistortionTexture = uvDistortionTexture ? 1 : 0;
    const int hasNormalTexture = normalTexture ? 1 : 0;
    const int hasRoughnessTexture = roughnessTexture ? 1 : 0;
    const int hasSpecularTexture = specularTexture ? 1 : 0;
    const int useFlipbook = meshDesc.flipbook.enabled ? 1 : 0;
    const int flipbookColumns = (std::max)(meshDesc.flipbook.columns, 1);
    const int flipbookRows = (std::max)(meshDesc.flipbook.rows, 1);
    const int flipbookLoop = meshDesc.flipbook.loop ? 1 : 0;
    Vec2 screenDistortionInvViewportSize = Vec2(1.f, 1.f); // 현재 viewport 픽셀 좌표를 화면 UV로 변환하기 위한 역해상도다.

    UINT viewportCount = 1; // Mesh distortion이 Scene/Game/Effect View 크기를 직접 따르도록 현재 viewport 하나를 읽는다.
    D3D11_VIEWPORT viewport{};
    _context->RSGetViewports(&viewportCount, &viewport);

    if (viewport.Width > 0.f && viewport.Height > 0.f)
        screenDistortionInvViewportSize = Vec2(1.f / viewport.Width, 1.f / viewport.Height);

    CHECK_FAILED(_shaderCom->Bind_Matrix("g_WorldMatrix", &_transformCom->Get_WorldMatrix()), E_FAIL);
    CHECK_FAILED(_shaderCom->Bind_Matrix("g_ViewMatrix", GAME->Get_Transform(ETransformState::View)), E_FAIL);
    CHECK_FAILED(_shaderCom->Bind_Matrix("g_ProjMatrix", GAME->Get_Transform(ETransformState::Proj)), E_FAIL);

    CHECK_FAILED(GAME->Bind_CamPosition(_shaderCom, "g_CamPosition"), E_FAIL);

    CHECK_FAILED(_shaderCom->Bind_RawValue("g_UVOffset", &uvOffset, sizeof(Vec2)), E_FAIL);
    CHECK_FAILED(_shaderCom->Bind_RawValue("g_UVTiling", &meshDesc.uvTiling, sizeof(Vec2)), E_FAIL);
    CHECK_FAILED(_shaderCom->Bind_RawValue("g_UVDistortionOffset", &uvDistortionOffset, sizeof(Vec2)), E_FAIL);
    CHECK_FAILED(_shaderCom->Bind_RawValue("g_UVDistortionStrength", &meshDesc.uvDistortionStrength, sizeof(Vec2)), E_FAIL);
    
    const Vec4 colorTint = _useRuntimeColorTintOverride ? _runtimeColorTint : meshDesc.colorTint;
    const float opacity = _useRuntimeOpacityOverride ? _runtimeOpacity : meshDesc.opacity;
    const float emissiveStrength = _useRuntimeEmissiveStrengthOverride ? _runtimeEmissiveStrength : meshDesc.emissiveStrength;

    CHECK_FAILED(_shaderCom->Bind_RawValue("g_ColorTint", &colorTint, sizeof(Vec4)), E_FAIL);
    CHECK_FAILED(_shaderCom->Bind_RawValue("g_Opacity", &opacity, sizeof(float)), E_FAIL);
    CHECK_FAILED(_shaderCom->Bind_RawValue("g_EmissiveStrength", &emissiveStrength, sizeof(float)), E_FAIL);
    CHECK_FAILED(_shaderCom->Bind_RawValue("g_FresnelPower", &meshDesc.fresnelPower, sizeof(float)), E_FAIL);
    CHECK_FAILED(_shaderCom->Bind_RawValue("g_FresnelMultiplier", &meshDesc.fresnelMultiplier, sizeof(float)), E_FAIL);
    CHECK_FAILED(_shaderCom->Bind_RawValue("g_NormalStrength", &meshDesc.normalStrength, sizeof(float)), E_FAIL);
    CHECK_FAILED(_shaderCom->Bind_RawValue("g_Roughness", &meshDesc.roughness, sizeof(float)), E_FAIL);
    CHECK_FAILED(_shaderCom->Bind_RawValue("g_SpecularStrength", &meshDesc.specularStrength, sizeof(float)), E_FAIL);
    CHECK_FAILED(_shaderCom->Bind_RawValue("g_SpecularPower", &meshDesc.specularPower, sizeof(float)), E_FAIL);
    CHECK_FAILED(_shaderCom->Bind_RawValue("g_CustomParams0", &meshDesc.customParams0, sizeof(Vec4)), E_FAIL);
    CHECK_FAILED(_shaderCom->Bind_RawValue("g_CustomParams1", &meshDesc.customParams1, sizeof(Vec4)), E_FAIL);

    const FLightDesc effectLightDesc = Resolve_EffectMeshLightDesc(); // Lit Effect Mesh가 deferred light pass 없이도 자체 조명을 계산할 수 있게 하는 대표 라이트다.
    CHECK_FAILED(_shaderCom->Bind_RawValue("g_LightDir", &effectLightDesc.direction, sizeof(effectLightDesc.direction)), E_FAIL);
    CHECK_FAILED(_shaderCom->Bind_RawValue("g_LightDiffuse", &effectLightDesc.diffuse, sizeof(effectLightDesc.diffuse)), E_FAIL);
    CHECK_FAILED(_shaderCom->Bind_RawValue("g_LightAmbient", &effectLightDesc.ambient, sizeof(effectLightDesc.ambient)), E_FAIL);
    CHECK_FAILED(_shaderCom->Bind_RawValue("g_LightSpecular", &effectLightDesc.specular, sizeof(effectLightDesc.specular)), E_FAIL);

    CHECK_FAILED(_shaderCom->Bind_RawValue("g_ForceVisiblePreview", &forceVisiblePreview, sizeof(int)), E_FAIL);
    CHECK_FAILED(_shaderCom->Bind_RawValue("g_ShadingMode", &shadingMode, sizeof(int)), E_FAIL);
    CHECK_FAILED(_shaderCom->Bind_RawValue("g_HasDiffuseTexture", &hasDiffuseTexture, sizeof(int)), E_FAIL);
    CHECK_FAILED(_shaderCom->Bind_RawValue("g_HasMaskTexture", &hasMaskTexture, sizeof(int)), E_FAIL);
    CHECK_FAILED(_shaderCom->Bind_RawValue("g_HasOpacityTexture", &hasOpacityTexture, sizeof(int)), E_FAIL);
    CHECK_FAILED(_shaderCom->Bind_RawValue("g_HasOpacitySubUvTexture", &hasOpacitySubUvTexture, sizeof(int)), E_FAIL);
    CHECK_FAILED(_shaderCom->Bind_RawValue("g_HasOpacityGradationTexture", &hasOpacityGradationTexture, sizeof(int)), E_FAIL);
    CHECK_FAILED(_shaderCom->Bind_RawValue("g_HasEmissiveGradationTexture", &hasEmissiveGradationTexture, sizeof(int)), E_FAIL);
    CHECK_FAILED(_shaderCom->Bind_RawValue("g_HasUVDistortionTexture", &hasUvDistortionTexture, sizeof(int)), E_FAIL);
    CHECK_FAILED(_shaderCom->Bind_RawValue("g_HasNormalTexture", &hasNormalTexture, sizeof(int)), E_FAIL);
    CHECK_FAILED(_shaderCom->Bind_RawValue("g_HasRoughnessTexture", &hasRoughnessTexture, sizeof(int)), E_FAIL);
    CHECK_FAILED(_shaderCom->Bind_RawValue("g_HasSpecularTexture", &hasSpecularTexture, sizeof(int)), E_FAIL);
    CHECK_FAILED(_shaderCom->Bind_RawValue("g_ElapsedTime", &_elapsed, sizeof(float)), E_FAIL);
    CHECK_FAILED(_shaderCom->Bind_RawValue("g_UseFlipbook", &useFlipbook, sizeof(int)), E_FAIL);
    CHECK_FAILED(_shaderCom->Bind_RawValue("g_FlipbookColumns", &flipbookColumns, sizeof(int)), E_FAIL);
    CHECK_FAILED(_shaderCom->Bind_RawValue("g_FlipbookRows", &flipbookRows, sizeof(int)), E_FAIL);
    CHECK_FAILED(_shaderCom->Bind_RawValue("g_FlipbookFps", &meshDesc.flipbook.fps, sizeof(float)), E_FAIL);
    CHECK_FAILED(_shaderCom->Bind_RawValue("g_FlipbookStartFrame", &meshDesc.flipbook.startFrame, sizeof(int)), E_FAIL);
    CHECK_FAILED(_shaderCom->Bind_RawValue("g_FlipbookEndFrame", &meshDesc.flipbook.endFrame, sizeof(int)), E_FAIL);
    CHECK_FAILED(_shaderCom->Bind_RawValue("g_FlipbookLoop", &flipbookLoop, sizeof(int)), E_FAIL);
    BindOptionalRaw("g_ScreenDistortionInvViewportSize", &screenDistortionInvViewportSize, sizeof(Vec2));
    BindOptionalRaw("g_ScreenDistortionStrength", &meshDesc.screenDistortionStrength, sizeof(float));
    BindOptionalRaw("g_ScreenDistortionRadialStrength", &meshDesc.screenDistortionRadialStrength, sizeof(float));

    if (diffuseTexture)
        {CHECK_FAILED(diffuseTexture->Bind_SRV(_shaderCom, "g_DiffuseTexture", 0), E_FAIL);}
    else
        {CHECK_FAILED(_shaderCom->Bind_SRV("g_DiffuseTexture", nullptr), E_FAIL);}

    if (maskTexture)
        {CHECK_FAILED(maskTexture->Bind_SRV(_shaderCom, "g_MaskTexture", 0), E_FAIL);}
    else
        {CHECK_FAILED(_shaderCom->Bind_SRV("g_MaskTexture", nullptr), E_FAIL);}

    if (emissiveTexture)
        {CHECK_FAILED(emissiveTexture->Bind_SRV(_shaderCom, "g_EmissiveTexture", 0), E_FAIL);}
    else
        {CHECK_FAILED(_shaderCom->Bind_SRV("g_EmissiveTexture", nullptr), E_FAIL);}

    if (opacityTexture)
        {CHECK_FAILED(opacityTexture->Bind_SRV(_shaderCom, "g_OpacityTexture", 0), E_FAIL);}
    else
        {CHECK_FAILED(_shaderCom->Bind_SRV("g_OpacityTexture", nullptr), E_FAIL);}

    if (opacitySubUvTexture)
        {CHECK_FAILED(opacitySubUvTexture->Bind_SRV(_shaderCom, "g_OpacitySubUvTexture", 0), E_FAIL);}
    else
        {CHECK_FAILED(_shaderCom->Bind_SRV("g_OpacitySubUvTexture", nullptr), E_FAIL);}

    if (opacityGradationTexture)
        {CHECK_FAILED(opacityGradationTexture->Bind_SRV(_shaderCom, "g_OpacityGradationTexture", 0), E_FAIL);}
    else
        {CHECK_FAILED(_shaderCom->Bind_SRV("g_OpacityGradationTexture", nullptr), E_FAIL);}

    if (emissiveGradationTexture)
        {CHECK_FAILED(emissiveGradationTexture->Bind_SRV(_shaderCom, "g_EmissiveGradationTexture", 0), E_FAIL);}
    else
        {CHECK_FAILED(_shaderCom->Bind_SRV("g_EmissiveGradationTexture", nullptr), E_FAIL);}

    if (uvDistortionTexture)
        {CHECK_FAILED(uvDistortionTexture->Bind_SRV(_shaderCom, "g_UVDistortionTexture", 0), E_FAIL);}
    else
        {CHECK_FAILED(_shaderCom->Bind_SRV("g_UVDistortionTexture", nullptr), E_FAIL);}

    if (normalTexture)
        {CHECK_FAILED(normalTexture->Bind_SRV(_shaderCom, "g_NormalTexture", 0), E_FAIL);}
    else
        {CHECK_FAILED(_shaderCom->Bind_SRV("g_NormalTexture", nullptr), E_FAIL);}

    if (roughnessTexture)
        {CHECK_FAILED(roughnessTexture->Bind_SRV(_shaderCom, "g_RoughnessTexture", 0), E_FAIL);}
    else
        {CHECK_FAILED(_shaderCom->Bind_SRV("g_RoughnessTexture", nullptr), E_FAIL);}

    if (specularTexture)
        {CHECK_FAILED(specularTexture->Bind_SRV(_shaderCom, "g_SpecularTexture", 0), E_FAIL);}
    else
        {CHECK_FAILED(_shaderCom->Bind_SRV("g_SpecularTexture", nullptr), E_FAIL);}

    if (meshDesc.useScreenDistortion && !Is_SkeletalLayer())
    {
        CHECK_FAILED(GAME->Bind_RT_ShaderResource(_shaderCom, "g_SceneColorTexture", L"Target_SceneColorCopy"), E_FAIL);
    }
    else
    {
        BindOptionalNullSrv("g_SceneColorTexture");
    }

    return S_OK;
}

HRESULT EffectMeshObject::Resolve_Resources()
{
    _materialOverrideResources.clear();

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

                const EMeshVertexType meshType =
                    Is_SkeletalLayer() ? EMeshVertexType::SkeletalMesh : EMeshVertexType::StaticMesh;

                auto proto = Model::Create(
                    _device, _context, meshType, Utils::ToString(resolvedPath), preTransform);

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
        CHECK_FAILED(Resolve_TextureByGuid(_layerDesc.mesh.diffuseTextureGuid, _diffuseTexture, "diffuse"), E_FAIL);
    }

    if (!_layerDesc.mesh.maskTextureGuid.empty() && !_maskTexture)
    {
        if (_layerDesc.mesh.maskTextureGuid == _layerDesc.mesh.diffuseTextureGuid && _diffuseTexture)
        {
            _maskTexture = _diffuseTexture;
        }
        else
        {
            CHECK_FAILED(Resolve_TextureByGuid(_layerDesc.mesh.maskTextureGuid, _maskTexture, "mask"), E_FAIL);
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
            CHECK_FAILED(Resolve_TextureByGuid(emissiveGuid, _emissiveTexture, "emissive"), E_FAIL);
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
            CHECK_FAILED(Resolve_TextureByGuid(opacityGuid, _opacityTexture, "opacity"), E_FAIL);
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
            CHECK_FAILED(Resolve_TextureByGuid(_layerDesc.mesh.opacitySubUvTextureGuid, _opacitySubUvTexture, "opacity subuv"), E_FAIL);
        }
    }

    if (!_layerDesc.mesh.opacityGradationTextureGuid.empty() && !_opacityGradationTexture)
    {
        CHECK_FAILED(Resolve_TextureByGuid(_layerDesc.mesh.opacityGradationTextureGuid, _opacityGradationTexture, "opacity gradation"), E_FAIL);
    }

    if (!_layerDesc.mesh.emissiveGradationTextureGuid.empty() && !_emissiveGradationTexture)
    {
        CHECK_FAILED(Resolve_TextureByGuid(_layerDesc.mesh.emissiveGradationTextureGuid, _emissiveGradationTexture, "emissive gradation"), E_FAIL);
    }

    if (!_layerDesc.mesh.uvDistortionTextureGuid.empty() && !_uvDistortionTexture)
    {
        CHECK_FAILED(Resolve_TextureByGuid(_layerDesc.mesh.uvDistortionTextureGuid, _uvDistortionTexture, "uv distortion"), E_FAIL);
    }

    CHECK_FAILED(Resolve_OverrideResources(), E_FAIL);

    return S_OK;

}

// 이펙트 텍스처는 최초 사용 시 동적으로 프로토타입에 등록되므로, 누락 로그 없이 조용히 기존 등록 상태를 먼저 확인한다.
HRESULT EffectMeshObject::Resolve_TextureByGuid(const string& guid, Shared<Texture>& outTexture, const char* usageName)
{
    outTexture = nullptr;

    if (guid.empty())
        return S_OK;

    const uint32 texKey = static_cast<uint32>(hash<string>{}(guid));
    const uint32 currentLevel = GAME->Current_Level(); // Static에 없고 현재 레벨에만 등록된 텍스처 프로토타입도 재사용하기 위한 조회 대상이다.

    // 같은 EffectMeshObject 안에서 base/override가 동일 GUID를 공유하면
    // 이미 붙여둔 텍스처 컴포넌트를 그대로 재사용해야 중복 Add_Component 실패를 막을 수 있다.
    if (auto existingTexture = dynamic_pointer_cast<Texture>(Get_Component(texKey)))
    {
        outTexture = existingTexture;
        return S_OK;
    }

    if (GAME->Find_Component_Prototype(0, texKey) != nullptr)
    {
        CHECK_FAILED(Add_Component(0, texKey, outTexture), E_FAIL);
        return S_OK;
    }

    if (currentLevel != 0 &&
        GAME->Find_Component_Prototype(currentLevel, texKey) != nullptr)
    {
        CHECK_FAILED(Add_Component(currentLevel, texKey, outTexture), E_FAIL);
        return S_OK;
    }

    const wstring resolvedPath = GAME->Resolve_AssetPath(guid);
    if (resolvedPath.empty())
    {
        LOG_ERROR(
            "EffectMeshObject {} resolve failed. layer='{}', guid='{}'",
            usageName,
            _layerDesc.base.layerName,
            guid);

        return E_FAIL;
    }

    auto proto = Texture::Create(_device, _context, resolvedPath, 1);
    CHECK_NULL(proto, E_FAIL);
    CHECK_FAILED(GAME->Add_Component_Prototype(0, texKey, proto), E_FAIL);
    CHECK_FAILED(Add_Component(0, texKey, outTexture), E_FAIL);
    return S_OK;
}

HRESULT EffectMeshObject::Resolve_OverrideResources()
{
    _materialOverrideResources.clear();

    for (const auto& overrideDesc : _layerDesc.mesh.materialOverrides)
    {
        if (overrideDesc.materialName.empty())
            continue;

        FResolvedMaterialResources resources{};
        resources.materialName = overrideDesc.materialName;

        CHECK_FAILED(Resolve_TextureByGuid(overrideDesc.diffuseTextureGuid, resources.diffuseTexture), E_FAIL);
        CHECK_FAILED(Resolve_TextureByGuid(overrideDesc.maskTextureGuid, resources.maskTexture), E_FAIL);
        CHECK_FAILED(Resolve_TextureByGuid(overrideDesc.emissiveTextureGuid, resources.emissiveTexture), E_FAIL);
        CHECK_FAILED(Resolve_TextureByGuid(overrideDesc.opacityTextureGuid, resources.opacityTexture), E_FAIL);
        CHECK_FAILED(Resolve_TextureByGuid(overrideDesc.opacitySubUvTextureGuid, resources.opacitySubUvTexture), E_FAIL);
        CHECK_FAILED(Resolve_TextureByGuid(overrideDesc.opacityGradationTextureGuid, resources.opacityGradationTexture), E_FAIL);
        CHECK_FAILED(Resolve_TextureByGuid(overrideDesc.emissiveGradationTextureGuid, resources.emissiveGradationTexture), E_FAIL);
        CHECK_FAILED(Resolve_TextureByGuid(overrideDesc.uvDistortionTextureGuid, resources.uvDistortionTexture), E_FAIL);
        CHECK_FAILED(Resolve_TextureByGuid(overrideDesc.normalTextureGuid, resources.normalTexture), E_FAIL);
        CHECK_FAILED(Resolve_TextureByGuid(overrideDesc.roughnessTextureGuid, resources.roughnessTexture), E_FAIL);
        CHECK_FAILED(Resolve_TextureByGuid(overrideDesc.specularTextureGuid, resources.specularTexture), E_FAIL);

        if (!resources.diffuseTexture) resources.diffuseTexture = _diffuseTexture;
        if (!resources.maskTexture) resources.maskTexture = _maskTexture;
        if (!resources.emissiveTexture) resources.emissiveTexture = _emissiveTexture;
        if (!resources.opacityTexture) resources.opacityTexture = _opacityTexture;
        if (!resources.opacitySubUvTexture) resources.opacitySubUvTexture = _opacitySubUvTexture;
        if (!resources.opacityGradationTexture) resources.opacityGradationTexture = _opacityGradationTexture;
        if (!resources.emissiveGradationTexture) resources.emissiveGradationTexture = _emissiveGradationTexture;
        if (!resources.uvDistortionTexture) resources.uvDistortionTexture = _uvDistortionTexture;
        if (!resources.normalTexture) resources.normalTexture = _normalTexture;
        if (!resources.roughnessTexture) resources.roughnessTexture = _roughnessTexture;
        if (!resources.specularTexture) resources.specularTexture = _specularTexture;

        _materialOverrideResources.push_back(resources);
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

void EffectMeshObject::Sync_RotationBaseFromCurrentTransform()
{
    CHECK_NULL(_transformCom);

    _rotationBaseLocal = _transformCom->Get_LocalRotation();
    _rotationBaseLocal.Normalize();
    _accumulatedRotation = 0.f;
}

void EffectMeshObject::Update_Rotation(float timeDelta)
{
    if (abs(_layerDesc.mesh.rotationSpeed) <= 0.0001f)
        return;

    Vec3 axis = _layerDesc.mesh.rotationAxis;
    if (axis.LengthSquared() <= FLT_EPSILON)
        axis = Vec3::Up;

    axis.Normalize();

    _accumulatedRotation += XMConvertToRadians(_layerDesc.mesh.rotationSpeed) * timeDelta;

    const float wrappedRotation = fmod(_accumulatedRotation, XM_2PI);
    Quat axisRotation = Quat::CreateFromAxisAngle(axis, wrappedRotation);
    Quat finalRotation = Quat::Identity;

    // Local Space면 "기울어진 레이어 자신의 축"으로 자전하고,
    // Owner Space면 "오너 기준 고정 축"을 먼저 적용한 뒤 배치 자세를 덧입힌다.
    if (_layerDesc.mesh.rotateInLocalSpace)
        finalRotation = _rotationBaseLocal * axisRotation;
    else
        finalRotation = axisRotation * _rotationBaseLocal;

    finalRotation.Normalize();
    _transformCom->Set_LocalRotation(finalRotation);
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
    const uint32 shaderComponentId = Resolve_ShaderComponentId();

    if (FAILED(Add_Component(shaderComponentId, _shaderCom)))
    {
        LOG_ERROR(
            "EffectMeshObject failed to add shader component. layer='{}', shaderTypeId={}",
            _layerDesc.base.layerName,
            shaderComponentId);

        return E_FAIL;
    }

    return S_OK;
}

HRESULT EffectMeshObject::Apply_AnimationSettings()
{
    if (!_modelCom || !Is_SkeletalLayer())
        return S_OK;

    _modelCom->Set_EnableNotifies(false);
    _modelCom->Set_AnimationPlayRate(_layerDesc.mesh.animationPlayRate);

    if (!_layerDesc.mesh.animationName.empty() &&
        _modelCom->Find_AnimationIndex_ByName(_layerDesc.mesh.animationName) < 0)
    {
        if (auto animation = GAME->Get_Animation(_layerDesc.mesh.animationName))
        {
            _modelCom->Add_Animation(animation);
        }
        else if (!_layerDesc.mesh.modelGuid.empty())
        {
            const wstring modelPath = GAME->Resolve_AssetPath(_layerDesc.mesh.modelGuid);

            if (!modelPath.empty())
            {
                const string animationFolder = fs::path(modelPath).parent_path().string();
                const auto animations = GAME->Get_Animations_InFolder(animationFolder);

                for (const auto& animation : animations)
                {
                    _modelCom->Add_Animation(animation);
                }
            }
        }
    }

    if (!_modelCom->Has_Animations())
        return S_OK;

    if (!_layerDesc.mesh.animationName.empty())
    {
        _modelCom->Set_Animation(_layerDesc.mesh.animationName, _layerDesc.mesh.animationLoop);
        return S_OK;
    }

    _modelCom->Set_Animation(0, _layerDesc.mesh.animationLoop);
    return S_OK;
}

bool EffectMeshObject::Is_SkeletalLayer() const
{
    return _layerDesc.base.kind == EEffectLayerKind::SkeletalMesh;
}

uint32 EffectMeshObject::Resolve_ShaderComponentId() const
{
    if (!Is_SkeletalLayer())
        return static_cast<uint32>(Protocol::COMPONENT_TYPE_SHADER_EFFECT_MESH);

    return static_cast<uint32>(Protocol::COMPONENT_TYPE_SHADER_EFFECT_SKELETAL_MESH);
}

uint32 EffectMeshObject::Resolve_PassIndex() const
{
    return Resolve_PassIndex(Resolve_RuntimeMeshDesc(0));
}

uint32 EffectMeshObject::Resolve_PassIndex(const FEffectMeshMaterialRuntimeDesc& meshDesc) const
{
    const bool twoSided = meshDesc.twoSided;
    const EEffectBlendMode runtimeBlendMode = Resolve_RuntimeBlendMode(meshDesc);
    const bool hasOpacity = Has_OpacityTexture(meshDesc);

    if (meshDesc.useScreenDistortion && !Is_SkeletalLayer())
        return twoSided ? 11 : 10;

    if (!twoSided)
    {
        if (runtimeBlendMode == EEffectBlendMode::Opaque)
            return 2;

        if (hasOpacity)
            return static_cast<uint32>(runtimeBlendMode);

        return 3 + static_cast<uint32>(runtimeBlendMode);
    }

    if (runtimeBlendMode == EEffectBlendMode::Opaque)
        return 7;

    if (hasOpacity)
        return 5 + static_cast<uint32>(runtimeBlendMode);

    return 8 + static_cast<uint32>(runtimeBlendMode);
}

bool EffectMeshObject::Has_OpacityTexture(const FEffectMeshMaterialRuntimeDesc& meshDesc) const
{
    return
        !meshDesc.opacityTextureGuid.empty() ||
        !meshDesc.maskTextureGuid.empty() ||
        !meshDesc.opacitySubUvTextureGuid.empty();
}

bool EffectMeshObject::Has_NonOpaquePass() const
{
    if (!_modelCom || _modelCom->Get_NumMeshes() == 0)
        return Resolve_RuntimeBlendMode(Resolve_RuntimeMeshDesc(0)) != EEffectBlendMode::Opaque;

    for (uint32 i = 0; i < static_cast<uint32>(_modelCom->Get_NumMeshes()); ++i)
    {
        if (Resolve_RuntimeBlendMode(Resolve_RuntimeMeshDesc(i)) != EEffectBlendMode::Opaque)
            return true;
    }

    return false;
}

bool EffectMeshObject::Has_ScreenDistortionPass() const
{
    if (Is_SkeletalLayer())
        return false;

    if (!_modelCom || _modelCom->Get_NumMeshes() == 0)
        return Resolve_RuntimeMeshDesc(0).useScreenDistortion;

    for (uint32 i = 0; i < static_cast<uint32>(_modelCom->Get_NumMeshes()); ++i)
    {
        if (Resolve_RuntimeMeshDesc(i).useScreenDistortion)
            return true;
    }

    return false;
}

EffectMeshObject::FEffectMeshMaterialRuntimeDesc EffectMeshObject::Resolve_RuntimeMeshDesc(uint32 meshIndex) const
{
    FEffectMeshMaterialRuntimeDesc result{};
    result.diffuseTextureGuid = _layerDesc.mesh.diffuseTextureGuid;
    result.maskTextureGuid = _layerDesc.mesh.maskTextureGuid;
    result.emissiveTextureGuid = _layerDesc.mesh.emissiveTextureGuid;
    result.opacityTextureGuid = _layerDesc.mesh.opacityTextureGuid;
    result.opacitySubUvTextureGuid = _layerDesc.mesh.opacitySubUvTextureGuid;
    result.opacityGradationTextureGuid = _layerDesc.mesh.opacityGradationTextureGuid;
    result.emissiveGradationTextureGuid = _layerDesc.mesh.emissiveGradationTextureGuid;
    result.uvDistortionTextureGuid = _layerDesc.mesh.uvDistortionTextureGuid;
    result.normalTextureGuid = _layerDesc.mesh.normalTextureGuid;
    result.roughnessTextureGuid = _layerDesc.mesh.roughnessTextureGuid;
    result.specularTextureGuid = _layerDesc.mesh.specularTextureGuid;
    result.blendMode = _layerDesc.mesh.blendMode;
    result.shadingMode = _layerDesc.mesh.shadingMode;
    result.uvScrollSpeed = _layerDesc.mesh.uvScrollSpeed;
    result.uvTiling = _layerDesc.mesh.uvTiling;
    result.uvDistortionStrength = _layerDesc.mesh.uvDistortionStrength;
    result.uvDistortionSpeed = _layerDesc.mesh.uvDistortionSpeed;
    result.flipbook = _layerDesc.mesh.flipbook;
    result.colorTint = _layerDesc.mesh.colorTint;
    result.opacity = _layerDesc.mesh.opacity;
    result.normalStrength = _layerDesc.mesh.normalStrength;
    result.roughness = _layerDesc.mesh.roughness;
    result.specularStrength = _layerDesc.mesh.specularStrength;
    result.specularPower = _layerDesc.mesh.specularPower;
    result.emissiveStrength = _layerDesc.mesh.emissiveStrength;
    result.fresnelPower = _layerDesc.mesh.fresnelPower;
    result.fresnelMultiplier = _layerDesc.mesh.fresnelMultiplier;
    result.useScreenDistortion = _layerDesc.mesh.useScreenDistortion;
    result.screenDistortionStrength = _layerDesc.mesh.screenDistortionStrength;
    result.screenDistortionRadialStrength = _layerDesc.mesh.screenDistortionRadialStrength;
    result.customParams0 = _layerDesc.mesh.customParams0;
    result.customParams1 = _layerDesc.mesh.customParams1;
    result.twoSided = _layerDesc.mesh.twoSided;
    result.useOpacityAsTransparency = _layerDesc.mesh.useOpacityAsTransparency;

    if (!_modelCom)
        return result;

    const uint32 materialIndex = _modelCom->Get_MeshMaterialIndex(meshIndex);
    const auto material = _modelCom->Get_Material(materialIndex);
    if (!material)
        return result;

    const string materialName = material->Get_MaterialName();

    for (const auto& overrideDesc : _layerDesc.mesh.materialOverrides)
    {
        if (!overrideDesc.enabled || overrideDesc.materialName != materialName)
            continue;

        if (!overrideDesc.diffuseTextureGuid.empty()) result.diffuseTextureGuid = overrideDesc.diffuseTextureGuid;
        if (!overrideDesc.maskTextureGuid.empty()) result.maskTextureGuid = overrideDesc.maskTextureGuid;
        if (!overrideDesc.emissiveTextureGuid.empty()) result.emissiveTextureGuid = overrideDesc.emissiveTextureGuid;
        if (!overrideDesc.opacityTextureGuid.empty()) result.opacityTextureGuid = overrideDesc.opacityTextureGuid;
        if (!overrideDesc.opacitySubUvTextureGuid.empty()) result.opacitySubUvTextureGuid = overrideDesc.opacitySubUvTextureGuid;
        if (!overrideDesc.opacityGradationTextureGuid.empty()) result.opacityGradationTextureGuid = overrideDesc.opacityGradationTextureGuid;
        if (!overrideDesc.emissiveGradationTextureGuid.empty()) result.emissiveGradationTextureGuid = overrideDesc.emissiveGradationTextureGuid;
        if (!overrideDesc.uvDistortionTextureGuid.empty()) result.uvDistortionTextureGuid = overrideDesc.uvDistortionTextureGuid;
        if (!overrideDesc.normalTextureGuid.empty()) result.normalTextureGuid = overrideDesc.normalTextureGuid;
        if (!overrideDesc.roughnessTextureGuid.empty()) result.roughnessTextureGuid = overrideDesc.roughnessTextureGuid;
        if (!overrideDesc.specularTextureGuid.empty()) result.specularTextureGuid = overrideDesc.specularTextureGuid;

        result.blendMode = overrideDesc.blendMode;
        result.shadingMode = overrideDesc.shadingMode;
        result.uvScrollSpeed = overrideDesc.uvScrollSpeed;
        result.uvTiling = overrideDesc.uvTiling;
        result.uvDistortionStrength = overrideDesc.uvDistortionStrength;
        result.uvDistortionSpeed = overrideDesc.uvDistortionSpeed;
        result.flipbook = overrideDesc.flipbook;
        result.colorTint = overrideDesc.colorTint;
        result.opacity = overrideDesc.opacity;
        result.normalStrength = overrideDesc.normalStrength;
        result.roughness = overrideDesc.roughness;
        result.specularStrength = overrideDesc.specularStrength;
        result.specularPower = overrideDesc.specularPower;
        result.emissiveStrength = overrideDesc.emissiveStrength;
        result.fresnelPower = overrideDesc.fresnelPower;
        result.fresnelMultiplier = overrideDesc.fresnelMultiplier;
        result.useScreenDistortion = overrideDesc.useScreenDistortion;
        result.screenDistortionStrength = overrideDesc.screenDistortionStrength;
        result.screenDistortionRadialStrength = overrideDesc.screenDistortionRadialStrength;
        result.twoSided = overrideDesc.twoSided;
        result.useOpacityAsTransparency = overrideDesc.useOpacityAsTransparency;
        break;
    }

    return result;
}

const EffectMeshObject::FResolvedMaterialResources* EffectMeshObject::Resolve_RuntimeMaterialResources(uint32 meshIndex) const
{
    if (!_modelCom)
        return nullptr;

    const uint32 materialIndex = _modelCom->Get_MeshMaterialIndex(meshIndex);
    const auto material = _modelCom->Get_Material(materialIndex);
    if (!material)
        return nullptr;

    const string materialName = material->Get_MaterialName();

    bool hasEnabledOverride = false;

    for (const auto& overrideDesc : _layerDesc.mesh.materialOverrides)
    {
        if (overrideDesc.materialName != materialName)
            continue;

        if (!overrideDesc.enabled)
            return nullptr;

        hasEnabledOverride = true;
        break;
    }

    if (!hasEnabledOverride)
        return nullptr;

    for (const auto& resources : _materialOverrideResources)
    {
        if (resources.materialName == materialName)
            return &resources;
    }

    return nullptr;
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
