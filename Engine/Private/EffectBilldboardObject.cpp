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
    , _baseMaskTextureCom(rhs._baseMaskTextureCom)
    , _baseOpacityTextureCom(rhs._baseOpacityTextureCom)
    , _baseOpacityGradationTextureCom(rhs._baseOpacityGradationTextureCom)
    , _ringTextureCom(rhs._ringTextureCom)
    , _ringOpacityTextureCom(rhs._ringOpacityTextureCom)
    , _ringOpacityGradationTextureCom(rhs._ringOpacityGradationTextureCom)
    , _screenDistortionNormalTextureCom(rhs._screenDistortionNormalTextureCom)
    , _layerDesc(rhs._layerDesc)
    , _resolvedBaseTextureGuid(rhs._resolvedBaseTextureGuid)
    , _resolvedBaseMaskTextureGuid(rhs._resolvedBaseMaskTextureGuid)
    , _resolvedBaseOpacityTextureGuid(rhs._resolvedBaseOpacityTextureGuid)
    , _resolvedBaseOpacityGradationTextureGuid(rhs._resolvedBaseOpacityGradationTextureGuid)
    , _resolvedRingTextureGuid(rhs._resolvedRingTextureGuid)
    , _resolvedRingOpacityTextureGuid(rhs._resolvedRingOpacityTextureGuid)
    , _resolvedRingOpacityGradationTextureGuid(rhs._resolvedRingOpacityGradationTextureGuid)
    , _resolvedScreenDistortionNormalTextureGuid(rhs._resolvedScreenDistortionNormalTextureGuid)
    , _useRuntimeBaseOpacityOverride(rhs._useRuntimeBaseOpacityOverride)
    , _runtimeBaseOpacity(rhs._runtimeBaseOpacity)
    , _useRuntimeRingOpacityOverride(rhs._useRuntimeRingOpacityOverride)
    , _runtimeRingOpacity(rhs._runtimeRingOpacity)
    , _elapsedTime(rhs._elapsedTime)
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
    _elapsedTime = 0.f;

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

void EffectBillboardObject::Set_RuntimeBaseOpacityOverride(float baseOpacity, bool enabled)
{
    _useRuntimeBaseOpacityOverride = enabled;
    _runtimeBaseOpacity = baseOpacity;
}

void EffectBillboardObject::Set_RuntimeRingOpacityOverride(float ringOpacity, bool enabled)
{
    _useRuntimeRingOpacityOverride = enabled;
    _runtimeRingOpacity = ringOpacity;
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
    _elapsedTime += timeDelta;

}

void EffectBillboardObject::Late_Update(float timeDelta)
{
    GameObject::Late_Update(timeDelta);

    if (_layerDesc.billboard.billboardToCamera)
    {
        Update_BillboardRotation();
    }

    if (_layerDesc.billboard.renderMode == EEffectBillboardRenderMode::ScreenDistortion)
    {
        GAME->Add_RenderGroup(ERenderGroup::ScreenDistortion, GetSharedPtr());
        return;
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

    const float rollDegrees = _layerDesc.base.localRotation.z;
    if (abs(rollDegrees) > 0.0001f)
    {
        Vec3 forwardAxis = _transformCom->Get_WorldForward();
        if (forwardAxis.LengthSquared() > FLT_EPSILON)
        {
            forwardAxis.Normalize();

            Quat rollDelta = Quat::CreateFromAxisAngle(
                forwardAxis,
                XMConvertToRadians(rollDegrees));

            _transformCom->Add_WorldRotation(rollDelta);
        }
    }
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
    // Billboard shader variants in the workspace are mixed.
    // Treat newer runtime-only uniforms as optional so older shaders still render.
    auto BindOptionalRaw = [this](const char* name, const void* data, uint32 size)
        {
            _shaderCom->Bind_RawValue(name, data, size);
        };

    auto BindOptionalNullSrv = [this](const char* name)
        {
            _shaderCom->Bind_SRV(name, nullptr);
        };

    auto BindOptionalTexture = [this](Shared<Texture> texture, const char* name)
        {
            if (texture)
                texture->Bind_SRV(_shaderCom, name, 0);
            else
                _shaderCom->Bind_SRV(name, nullptr);
        };

    const float finalBaseOpacity = _useRuntimeBaseOpacityOverride
        ? _runtimeBaseOpacity
        : _layerDesc.billboard.baseOpacity;
    const float finalRingOpacity = _useRuntimeRingOpacityOverride
        ? _runtimeRingOpacity
        : _layerDesc.billboard.ringOpacity;
    const int useBaseFlipbook = _layerDesc.billboard.baseFlipbook.enabled ? 1 : 0;
    const int useRingFlipbook = _layerDesc.billboard.ringFlipbook.enabled ? 1 : 0;
    const int baseFlipbookColumns = (std::max)(_layerDesc.billboard.baseFlipbook.columns, 1);
    const int baseFlipbookRows = (std::max)(_layerDesc.billboard.baseFlipbook.rows, 1);
    const int ringFlipbookColumns = (std::max)(_layerDesc.billboard.ringFlipbook.columns, 1);
    const int ringFlipbookRows = (std::max)(_layerDesc.billboard.ringFlipbook.rows, 1);
    const int baseFlipbookLoop = _layerDesc.billboard.baseFlipbook.loop ? 1 : 0;
    const int ringFlipbookLoop = _layerDesc.billboard.ringFlipbook.loop ? 1 : 0;
    const int renderMode = static_cast<int>(_layerDesc.billboard.renderMode);
    const int hasBaseMaskTexture = _baseMaskTextureCom ? 1 : 0;
    const int hasBaseOpacityTexture = _baseOpacityTextureCom ? 1 : 0;
    const int hasBaseOpacityGradationTexture = _baseOpacityGradationTextureCom ? 1 : 0;
    const int hasRingOpacityTexture = _ringOpacityTextureCom ? 1 : 0;
    const int hasRingOpacityGradationTexture = _ringOpacityGradationTextureCom ? 1 : 0;
    const int hasScreenDistortionNormalTexture = _screenDistortionNormalTextureCom ? 1 : 0;
    Vec2 screenDistortionInvViewportSize = Vec2(1.f, 1.f); // 현재 viewport 픽셀 좌표를 0~1 화면 UV로 바꾸는 역해상도다.

    UINT viewportCount = 1; // ScreenDistortion이 Scene/Game/Effect View 크기를 직접 따르도록 현재 viewport 하나를 읽는다.
    D3D11_VIEWPORT viewport{};
    _context->RSGetViewports(&viewportCount, &viewport);

    if (viewport.Width > 0.f && viewport.Height > 0.f)
        screenDistortionInvViewportSize = Vec2(1.f / viewport.Width, 1.f / viewport.Height);

    CHECK_FAILED(_shaderCom->Bind_Matrix("g_WorldMatrix", &_transformCom->Get_WorldMatrix()), E_FAIL);
    CHECK_FAILED(_shaderCom->Bind_Matrix("g_ViewMatrix", GAME->Get_Transform(ETransformState::View)), E_FAIL);
    CHECK_FAILED(_shaderCom->Bind_Matrix("g_ProjMatrix", GAME->Get_Transform(ETransformState::Proj)), E_FAIL);

    CHECK_NULL(_baseTextureCom, E_FAIL);
    CHECK_FAILED(_baseTextureCom->Bind_SRV(_shaderCom, "g_BaseTexture", 0), E_FAIL);
    BindOptionalRaw("g_HasBaseMaskTexture", &hasBaseMaskTexture, sizeof(int));

    if (_baseMaskTextureCom)
    {
        BindOptionalTexture(_baseMaskTextureCom, "g_BaseMaskTexture");
    }
    else
    {
        BindOptionalNullSrv("g_BaseMaskTexture");
    }

    BindOptionalRaw("g_HasBaseOpacityTexture", &hasBaseOpacityTexture, sizeof(int));
    if (_baseOpacityTextureCom)
    {
        BindOptionalTexture(_baseOpacityTextureCom, "g_BaseOpacityTexture");
    }
    else
    {
        BindOptionalNullSrv("g_BaseOpacityTexture");
    }

    BindOptionalRaw("g_HasBaseOpacityGradationTexture", &hasBaseOpacityGradationTexture, sizeof(int));
    if (_baseOpacityGradationTextureCom)
    {
        BindOptionalTexture(_baseOpacityGradationTextureCom, "g_BaseOpacityGradationTexture");
    }
    else
    {
        BindOptionalNullSrv("g_BaseOpacityGradationTexture");
    }

    const int useRing = (_layerDesc.billboard.useRing && _ringTextureCom) ? 1 : 0;
    CHECK_FAILED(_shaderCom->Bind_RawValue("g_UseRing", &useRing, sizeof(int)), E_FAIL);
    CHECK_FAILED(_shaderCom->Bind_RawValue("g_BaseTint", &_layerDesc.billboard.baseTint, sizeof(Vec4)), E_FAIL);
    CHECK_FAILED(_shaderCom->Bind_RawValue("g_RingTint", &_layerDesc.billboard.ringTint, sizeof(Vec4)), E_FAIL);
    CHECK_FAILED(_shaderCom->Bind_RawValue("g_BaseOpacity", &finalBaseOpacity, sizeof(float)), E_FAIL);
    CHECK_FAILED(_shaderCom->Bind_RawValue("g_RingOpacity", &finalRingOpacity, sizeof(float)), E_FAIL);
    BindOptionalRaw("g_BaseEmissiveStrength", &_layerDesc.billboard.baseEmissiveStrength, sizeof(float));
    BindOptionalRaw("g_RingEmissiveStrength", &_layerDesc.billboard.ringEmissiveStrength, sizeof(float));
    BindOptionalRaw("g_ElapsedTime", &_elapsedTime, sizeof(float));
    BindOptionalRaw("g_RenderMode", &renderMode, sizeof(int));
    BindOptionalRaw("g_BaseUvOffset", &_layerDesc.billboard.baseUvOffset, sizeof(Vec2));
    BindOptionalRaw("g_BaseUvScale", &_layerDesc.billboard.baseUvScale, sizeof(Vec2));
    BindOptionalRaw("g_RingUvOffset", &_layerDesc.billboard.ringUvOffset, sizeof(Vec2));
    BindOptionalRaw("g_RingUvScale", &_layerDesc.billboard.ringUvScale, sizeof(Vec2));
    BindOptionalRaw("g_UseBaseFlipbook", &useBaseFlipbook, sizeof(int));
    BindOptionalRaw("g_BaseFlipbookColumns", &baseFlipbookColumns, sizeof(int));
    BindOptionalRaw("g_BaseFlipbookRows", &baseFlipbookRows, sizeof(int));
    BindOptionalRaw("g_BaseFlipbookFps", &_layerDesc.billboard.baseFlipbook.fps, sizeof(float));
    BindOptionalRaw("g_BaseFlipbookStartFrame", &_layerDesc.billboard.baseFlipbook.startFrame, sizeof(int));
    BindOptionalRaw("g_BaseFlipbookEndFrame", &_layerDesc.billboard.baseFlipbook.endFrame, sizeof(int));
    BindOptionalRaw("g_BaseFlipbookLoop", &baseFlipbookLoop, sizeof(int));
    BindOptionalRaw("g_UseRingFlipbook", &useRingFlipbook, sizeof(int));
    BindOptionalRaw("g_RingFlipbookColumns", &ringFlipbookColumns, sizeof(int));
    BindOptionalRaw("g_RingFlipbookRows", &ringFlipbookRows, sizeof(int));
    BindOptionalRaw("g_RingFlipbookFps", &_layerDesc.billboard.ringFlipbook.fps, sizeof(float));
    BindOptionalRaw("g_RingFlipbookStartFrame", &_layerDesc.billboard.ringFlipbook.startFrame, sizeof(int));
    BindOptionalRaw("g_RingFlipbookEndFrame", &_layerDesc.billboard.ringFlipbook.endFrame, sizeof(int));
    BindOptionalRaw("g_RingFlipbookLoop", &ringFlipbookLoop, sizeof(int));
    BindOptionalRaw("g_CustomParams0", &_layerDesc.billboard.customParams0, sizeof(Vec4));
    BindOptionalRaw("g_CustomParams1", &_layerDesc.billboard.customParams1, sizeof(Vec4));
    BindOptionalRaw("g_HasScreenDistortionNormalTexture", &hasScreenDistortionNormalTexture, sizeof(int));
    BindOptionalRaw("g_ScreenDistortionInvViewportSize", &screenDistortionInvViewportSize, sizeof(Vec2));
    BindOptionalRaw("g_ScreenDistortionStrength", &_layerDesc.billboard.screenDistortionStrength, sizeof(float));
    BindOptionalRaw("g_ScreenDistortionRadialStrength", &_layerDesc.billboard.screenDistortionRadialStrength, sizeof(float));
    BindOptionalRaw("g_ScreenDistortionNormalTiling", &_layerDesc.billboard.screenDistortionNormalTiling, sizeof(Vec2));
    BindOptionalRaw("g_ScreenDistortionScrollA", &_layerDesc.billboard.screenDistortionScrollA, sizeof(Vec2));
    BindOptionalRaw("g_ScreenDistortionScrollB", &_layerDesc.billboard.screenDistortionScrollB, sizeof(Vec2));

    if (_layerDesc.billboard.renderMode == EEffectBillboardRenderMode::ScreenDistortion)
    {
        CHECK_FAILED(GAME->Bind_RT_ShaderResource(_shaderCom, "g_SceneColorTexture", L"Target_SceneColorCopy"), E_FAIL);
    }
    else
    {
        BindOptionalNullSrv("g_SceneColorTexture");
    }

    BindOptionalTexture(_screenDistortionNormalTextureCom, "g_ScreenDistortionNormalTexture");

    if (_layerDesc.billboard.useRing && _ringTextureCom)
    {
        CHECK_FAILED(_ringTextureCom->Bind_SRV(_shaderCom, "g_RingTexture", 0), E_FAIL);
    }
    else
    {
        CHECK_FAILED(_shaderCom->Bind_SRV("g_RingTexture", nullptr), E_FAIL);
    }

    BindOptionalRaw("g_HasRingOpacityTexture", &hasRingOpacityTexture, sizeof(int));
    if (_ringOpacityTextureCom)
    {
        BindOptionalTexture(_ringOpacityTextureCom, "g_RingOpacityTexture");
    }
    else
    {
        BindOptionalNullSrv("g_RingOpacityTexture");
    }

    BindOptionalRaw("g_HasRingOpacityGradationTexture", &hasRingOpacityGradationTexture, sizeof(int));
    if (_ringOpacityGradationTextureCom)
    {
        BindOptionalTexture(_ringOpacityGradationTextureCom, "g_RingOpacityGradationTexture");
    }
    else
    {
        BindOptionalNullSrv("g_RingOpacityGradationTexture");
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

    if (_layerDesc.billboard.baseMaskTextureGuid.empty())
    {
        _baseMaskTextureCom.reset();
        _resolvedBaseMaskTextureGuid.clear();
    }
    else if (_layerDesc.billboard.baseMaskTextureGuid == _resolvedBaseTextureGuid && _baseTextureCom)
    {
        _baseMaskTextureCom = _baseTextureCom;
        _resolvedBaseMaskTextureGuid = _layerDesc.billboard.baseMaskTextureGuid;
    }
    else if (_resolvedBaseMaskTextureGuid != _layerDesc.billboard.baseMaskTextureGuid || !_baseMaskTextureCom)
    {
        CHECK_FAILED(Resolve_TextureComponent(_layerDesc.billboard.baseMaskTextureGuid, _baseMaskTextureCom), E_FAIL);
        _resolvedBaseMaskTextureGuid = _layerDesc.billboard.baseMaskTextureGuid;
    }

    if (_layerDesc.billboard.baseOpacityTextureGuid.empty())
    {
        _baseOpacityTextureCom.reset();
        _resolvedBaseOpacityTextureGuid.clear();
    }
    else if (_layerDesc.billboard.baseOpacityTextureGuid == _resolvedBaseTextureGuid && _baseTextureCom)
    {
        _baseOpacityTextureCom = _baseTextureCom;
        _resolvedBaseOpacityTextureGuid = _layerDesc.billboard.baseOpacityTextureGuid;
    }
    else if (_layerDesc.billboard.baseOpacityTextureGuid == _resolvedBaseMaskTextureGuid && _baseMaskTextureCom)
    {
        _baseOpacityTextureCom = _baseMaskTextureCom;
        _resolvedBaseOpacityTextureGuid = _layerDesc.billboard.baseOpacityTextureGuid;
    }
    else if (_resolvedBaseOpacityTextureGuid != _layerDesc.billboard.baseOpacityTextureGuid || !_baseOpacityTextureCom)
    {
        CHECK_FAILED(Resolve_TextureComponent(_layerDesc.billboard.baseOpacityTextureGuid, _baseOpacityTextureCom), E_FAIL);
        _resolvedBaseOpacityTextureGuid = _layerDesc.billboard.baseOpacityTextureGuid;
    }

    if (_layerDesc.billboard.baseOpacityGradationTextureGuid.empty())
    {
        _baseOpacityGradationTextureCom.reset();
        _resolvedBaseOpacityGradationTextureGuid.clear();
    }
    else if (_resolvedBaseOpacityGradationTextureGuid != _layerDesc.billboard.baseOpacityGradationTextureGuid || !_baseOpacityGradationTextureCom)
    {
        CHECK_FAILED(Resolve_TextureComponent(_layerDesc.billboard.baseOpacityGradationTextureGuid, _baseOpacityGradationTextureCom), E_FAIL);
        _resolvedBaseOpacityGradationTextureGuid = _layerDesc.billboard.baseOpacityGradationTextureGuid;
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

    if (_layerDesc.billboard.ringOpacityTextureGuid.empty())
    {
        _ringOpacityTextureCom.reset();
        _resolvedRingOpacityTextureGuid.clear();
    }
    else if (_layerDesc.billboard.ringOpacityTextureGuid == _resolvedRingTextureGuid && _ringTextureCom)
    {
        _ringOpacityTextureCom = _ringTextureCom;
        _resolvedRingOpacityTextureGuid = _layerDesc.billboard.ringOpacityTextureGuid;
    }
    else if (_resolvedRingOpacityTextureGuid != _layerDesc.billboard.ringOpacityTextureGuid || !_ringOpacityTextureCom)
    {
        CHECK_FAILED(Resolve_TextureComponent(_layerDesc.billboard.ringOpacityTextureGuid, _ringOpacityTextureCom), E_FAIL);
        _resolvedRingOpacityTextureGuid = _layerDesc.billboard.ringOpacityTextureGuid;
    }

    if (_layerDesc.billboard.ringOpacityGradationTextureGuid.empty())
    {
        _ringOpacityGradationTextureCom.reset();
        _resolvedRingOpacityGradationTextureGuid.clear();
    }
    else if (_resolvedRingOpacityGradationTextureGuid != _layerDesc.billboard.ringOpacityGradationTextureGuid || !_ringOpacityGradationTextureCom)
    {
        CHECK_FAILED(Resolve_TextureComponent(_layerDesc.billboard.ringOpacityGradationTextureGuid, _ringOpacityGradationTextureCom), E_FAIL);
        _resolvedRingOpacityGradationTextureGuid = _layerDesc.billboard.ringOpacityGradationTextureGuid;
    }

    if (_layerDesc.billboard.screenDistortionNormalTextureGuid.empty())
    {
        _screenDistortionNormalTextureCom.reset();
        _resolvedScreenDistortionNormalTextureGuid.clear();
    }
    else if (_layerDesc.billboard.screenDistortionNormalTextureGuid == _resolvedBaseTextureGuid && _baseTextureCom)
    {
        _screenDistortionNormalTextureCom = _baseTextureCom;
        _resolvedScreenDistortionNormalTextureGuid = _layerDesc.billboard.screenDistortionNormalTextureGuid;
    }
    else if (_resolvedScreenDistortionNormalTextureGuid != _layerDesc.billboard.screenDistortionNormalTextureGuid || !_screenDistortionNormalTextureCom)
    {
        CHECK_FAILED(Resolve_TextureComponent(_layerDesc.billboard.screenDistortionNormalTextureGuid, _screenDistortionNormalTextureCom), E_FAIL);
        _resolvedScreenDistortionNormalTextureGuid = _layerDesc.billboard.screenDistortionNormalTextureGuid;
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
    if (_layerDesc.billboard.renderMode == EEffectBillboardRenderMode::ScreenDistortion)
        return 0;

    // Distortion billboards are heat/shock masks, so they must never alpha-darken the background.
    if (_layerDesc.billboard.renderMode == EEffectBillboardRenderMode::Distortion)
        return 1;

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
