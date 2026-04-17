#include "pch.h"
#include "SkySphereActor.h"
#include "Model.h"
#include "Shader.h"
#include "ModelMaterial.h"
#include "GameObject_Factory.h"
#include "Transform.h"

REGISTER_GAMEOBJECT(SkySphereActor, Protocol::OBJECT_TYPE_SKY_SPHERE);

SkySphereActor::SkySphereActor(ComPtr<Device> device, ComPtr<DeviceContext> context)
    : GameObject(device, context)
{
}

SkySphereActor::SkySphereActor(const SkySphereActor& rhs)
    : GameObject(rhs)
    , _shaderCom(rhs._shaderCom)
    , _modelCom(rhs._modelCom)
    , _modelComponentName(rhs._modelComponentName)
    , _modelComponentId(rhs._modelComponentId)
    , _followCamera(rhs._followCamera)
    , _useBlend(rhs._useBlend)
    , _twoSided(rhs._twoSided)
    , _renderStyle(rhs._renderStyle)
    , _uvTiling(rhs._uvTiling)
    , _uvScrollSpeed(rhs._uvScrollSpeed)
    , _colorTint(rhs._colorTint)
    , _horizonColor(rhs._horizonColor)
    , _zenithColor(rhs._zenithColor)
    , _subUVTiling(rhs._subUVTiling)
    , _subUVScrollSpeed(rhs._subUVScrollSpeed)
    , _opacity(rhs._opacity)
    , _emissiveStrength(rhs._emissiveStrength)
    , _elapsedTime(rhs._elapsedTime)

{
}

HRESULT SkySphereActor::Initialize_Prototype()
{
    return GameObject::Initialize_Prototype();
}

HRESULT SkySphereActor::Initialize(void* arg)
{
    auto* desc = static_cast<FSkySphereDesc*>(arg);
    CHECK_NULL(desc, E_FAIL);

    CHECK_FAILED(GameObject::Initialize(arg), E_FAIL);

    _modelComponentName = desc->modelComponentName;
    _modelComponentId = static_cast<uint32>(hash<string>{}(_modelComponentName));
    _followCamera = desc->followCamera;
    _useBlend = desc->useBlend;
    _twoSided = desc->twoSided;
    _renderStyle = desc->renderStyle;
    _uvTiling = desc->uvTiling;
    _uvScrollSpeed = desc->uvScrollSpeed;
    _colorTint = desc->colorTint;
    _horizonColor = desc->horizonColor;
    _zenithColor = desc->zenithColor;
    _subUVTiling = desc->subUVTiling;
    _subUVScrollSpeed = desc->subUVScrollSpeed;
    _opacity = desc->opacity;
    _emissiveStrength = desc->emissiveStrength;
    _elapsedTime = 0.f;

    CHECK_FAILED(Ready_Components(), E_FAIL);

    //_transformCom->Set_LocalScale(0.05f);
    

    return S_OK;
}

void SkySphereActor::Priority_Update(float timeDelta)
{
    GameObject::Priority_Update(timeDelta);
}

void SkySphereActor::Update(float timeDelta)
{
    GameObject::Update(timeDelta);

    _elapsedTime += timeDelta;

    if (_followCamera)
    {
        const Vec4* camPos = GAME->Get_CamPosition();
        if (camPos)
        {
            _transformCom->Set_LocalPosition(camPos->x, camPos->y, camPos->z);
        }
    }
}

void SkySphereActor::Late_Update(float timeDelta)
{
    GameObject::Late_Update(timeDelta);

    if (_useBlend)
        GAME->Add_RenderGroup(ERenderGroup::Blend, GetSharedPtr());
    else
        GAME->Add_RenderGroup(ERenderGroup::NonLight, GetSharedPtr());
}

HRESULT SkySphereActor::Render()
{
    CHECK_FAILED(GameObject::Render(), E_FAIL);
    CHECK_NULL(_shaderCom, E_FAIL);
    CHECK_NULL(_modelCom, E_FAIL);

    CHECK_FAILED(Bind_ShaderResources(), E_FAIL);

    const size_t meshCount = _modelCom->Get_NumMeshes();

    for (size_t i = 0; i < meshCount; ++i)
    {
        Vec4 baseColorFactor = Vec4(1.f, 1.f, 1.f, 1.f);
        Vec4 shadowColor = Vec4(1.f, 1.f, 1.f, 1.f);

        int hasDiffuseTexture = 0;
        int hasMaskTexture = 0;
        int baseColorUVChannel = 0;
        int maskUVChannel = 0;
        float baseColorUVScale = 1.f;
        float maskUVScale = 1.f;

        const uint32 materialIndex = _modelCom->Get_MeshMaterialIndex(static_cast<uint32>(i));
        auto material = _modelCom->Get_Material(materialIndex);

        if (material)
        {
            baseColorFactor = material->Get_BaseColorFactor();
            shadowColor = material->Get_ShadowColor();
            hasDiffuseTexture = (material->Get_TextureCount(EMaterialTextureSlot::BaseColor) > 0) ? 1 : 0;
            hasMaskTexture = (material->Get_TextureCount(EMaterialTextureSlot::Mask) > 0) ? 1 : 0;
            baseColorUVChannel = static_cast<int>(material->Get_TextureUVChannel(EMaterialTextureSlot::BaseColor, 0));
            maskUVChannel = static_cast<int>(material->Get_TextureUVChannel(EMaterialTextureSlot::Mask, 0));
            baseColorUVScale = material->Get_TextureSamplingScale(EMaterialTextureSlot::BaseColor, 0);
            maskUVScale = material->Get_TextureSamplingScale(EMaterialTextureSlot::Mask, 0);
        }

        CHECK_FAILED(_shaderCom->Bind_RawValue("g_BaseColorFactor", &baseColorFactor, sizeof(Vec4)), E_FAIL);
        CHECK_FAILED(_shaderCom->Bind_RawValue("g_ShadowColor", &shadowColor, sizeof(Vec4)), E_FAIL);
        CHECK_FAILED(_shaderCom->Bind_RawValue("g_HasDiffuseTexture", &hasDiffuseTexture, sizeof(int)), E_FAIL);
        CHECK_FAILED(_shaderCom->Bind_RawValue("g_HasMaskTexture", &hasMaskTexture, sizeof(int)), E_FAIL);
        CHECK_FAILED(_shaderCom->Bind_RawValue("g_BaseColorUVChannel", &baseColorUVChannel, sizeof(int)), E_FAIL);
        CHECK_FAILED(_shaderCom->Bind_RawValue("g_MaskUVChannel", &maskUVChannel, sizeof(int)), E_FAIL);
        CHECK_FAILED(_shaderCom->Bind_RawValue("g_BaseColorUVScale", &baseColorUVScale, sizeof(float)), E_FAIL);
        CHECK_FAILED(_shaderCom->Bind_RawValue("g_MaskUVScale", &maskUVScale, sizeof(float)), E_FAIL);

        if (hasDiffuseTexture != 0)
        {
            CHECK_FAILED(
                _modelCom->Bind_Material(_shaderCom, "g_DiffuseTexture",
                    static_cast<uint32>(i), EMaterialTextureSlot::BaseColor, 0), E_FAIL);
        }
        else
        {
            CHECK_FAILED(_shaderCom->Bind_SRV("g_DiffuseTexture", nullptr), E_FAIL);
        }

        if (hasMaskTexture != 0)
        {
            CHECK_FAILED(
                _modelCom->Bind_Material(_shaderCom, "g_MaskTexture",
                    static_cast<uint32>(i), EMaterialTextureSlot::Mask, 0), E_FAIL);
        }
        else
        {
            CHECK_FAILED(_shaderCom->Bind_SRV("g_MaskTexture", nullptr), E_FAIL);
        }

        CHECK_FAILED(_shaderCom->Begin_Pass(Resolve_PassIndex()), E_FAIL);
        CHECK_FAILED(_modelCom->Render(static_cast<uint32>(i)), E_FAIL);
    }

    return S_OK;
}

HRESULT SkySphereActor::Bind_ShaderResources()
{
    CHECK_FAILED(_shaderCom->Bind_Matrix("g_WorldMatrix", &_transformCom->Get_WorldMatrix()), E_FAIL);
    CHECK_FAILED(_shaderCom->Bind_Matrix("g_ViewMatrix", GAME->Get_Transform(ETransformState::View)), E_FAIL);
    CHECK_FAILED(_shaderCom->Bind_Matrix("g_ProjMatrix", GAME->Get_Transform(ETransformState::Proj)), E_FAIL);

    CHECK_FAILED(_shaderCom->Bind_RawValue("g_RenderStyle", &_renderStyle, sizeof(int)), E_FAIL);
    CHECK_FAILED(_shaderCom->Bind_RawValue("g_UVTiling", &_uvTiling, sizeof(Vec2)), E_FAIL);
    CHECK_FAILED(_shaderCom->Bind_RawValue("g_UVScrollSpeed", &_uvScrollSpeed, sizeof(Vec2)), E_FAIL);
    CHECK_FAILED(_shaderCom->Bind_RawValue("g_ColorTint", &_colorTint, sizeof(Vec4)), E_FAIL);
    CHECK_FAILED(_shaderCom->Bind_RawValue("g_HorizonColor", &_horizonColor, sizeof(Vec4)), E_FAIL);
    CHECK_FAILED(_shaderCom->Bind_RawValue("g_ZenithColor", &_zenithColor, sizeof(Vec4)), E_FAIL);
    CHECK_FAILED(_shaderCom->Bind_RawValue("g_SubUVTiling", &_subUVTiling, sizeof(Vec2)), E_FAIL);
    CHECK_FAILED(_shaderCom->Bind_RawValue("g_SubUVScrollSpeed", &_subUVScrollSpeed, sizeof(Vec2)), E_FAIL);
    CHECK_FAILED(_shaderCom->Bind_RawValue("g_Opacity", &_opacity, sizeof(float)), E_FAIL);
    CHECK_FAILED(_shaderCom->Bind_RawValue("g_EmissiveStrength", &_emissiveStrength, sizeof(float)), E_FAIL);
    CHECK_FAILED(_shaderCom->Bind_RawValue("g_ElapsedTime", &_elapsedTime, sizeof(float)), E_FAIL);

    return S_OK;
}

HRESULT SkySphereActor::Ready_Components()
{
    CHECK_FAILED(Add_Component(Protocol::COMPONENT_TYPE_SHADER_SKYSPHERE, _shaderCom), E_FAIL);
    CHECK_FAILED(Add_Component(ETOI(ELevelType::Static), _modelComponentId, _modelCom), E_FAIL);

    return S_OK;
}

uint32 SkySphereActor::Resolve_PassIndex() const
{
    if (_twoSided == false && _useBlend == false)
        return 0;

    if (_twoSided == false && _useBlend == true)
        return 1;

    if (_twoSided == true && _useBlend == false)
        return 2;

    return 3;
}

Shared<GameObject> SkySphereActor::Create(ComPtr<Device> device, ComPtr<DeviceContext> context)
{
    auto instance = make_shared<SkySphereActor>(device, context);

    if (FAILED(instance->Initialize_Prototype()))
    {
        MSG_BOX("Failed to Created : SkySphereActor");
        return nullptr;
    }

    return instance;
}

Shared<GameObject> SkySphereActor::Clone(void* arg)
{
    auto clone = make_shared<SkySphereActor>(*this);

    if (FAILED(clone->Initialize(arg)))
    {
        MSG_BOX("Failed to Cloned : SkySphereActor");
        return nullptr;
    }

    return clone;
}

void SkySphereActor::Free()
{
    GameObject::Free();
}
