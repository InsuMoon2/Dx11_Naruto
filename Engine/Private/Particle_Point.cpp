#include "pch.h"
#include "Particle_Point.h"
#include "Texture.h"
#include "Shader.h"
#include "VIBuffer_Particle_Point.h"
#include "GameInstance.h"
#include "GameObject_Factory.h"

REGISTER_GAMEOBJECT(Particle_Point, Protocol::OBJECT_TYPE_INSTANCED_PARTICLE_POINT)


Particle_Point::Particle_Point(ComPtr<Device> device, ComPtr<DeviceContext> context)
    : GameObject(device, context)
{
}

Particle_Point::Particle_Point(const Particle_Point& rhs)
    : GameObject(rhs)
    , _textureIndex(rhs._textureIndex)
    , _addToBlendGroup(rhs._addToBlendGroup)
    , _blendMode(rhs._blendMode)
    , _colorTint(rhs._colorTint)
    , _opacity(rhs._opacity)
    , _emissiveStrength(rhs._emissiveStrength)
    , _useLifetimeFade(rhs._useLifetimeFade)
    , _flipbook(rhs._flipbook)
    , _customParams0(rhs._customParams0)
    , _customParams1(rhs._customParams1)
{
}

HRESULT Particle_Point::Initialize_Prototype()
{
    return GameObject::Initialize_Prototype();
}

HRESULT Particle_Point::Initialize(void* arg)
{
    CHECK_FAILED(GameObject::Initialize(arg), E_FAIL);

    auto* desc = static_cast<FParticlePointDesc*>(arg);
    CHECK_NULL(desc, E_FAIL);

    _textureIndex = desc->textureIndex;
    _addToBlendGroup = desc->addToBlendGroup;
    _blendMode = desc->blendMode;
    _colorTint = desc->colorTint;
    _opacity = desc->opacity;
    _emissiveStrength = desc->emissiveStrength;

    _useLifetimeFade = (desc->bufferDesc.moveMode == VIBuffer_Particle_Point::EMoveMode::Drop);
    _flipbook = desc->flipbook;
    _customParams0 = desc->customParams0;
    _customParams1 = desc->customParams1;

    CHECK_FAILED(Ready_Components(*desc), E_FAIL);

    return S_OK;
}

void Particle_Point::Priority_Update(float timeDelta)
{
    GameObject::Priority_Update(timeDelta);
}

void Particle_Point::Update(float timeDelta)
{
    if (_bufferCom)
    {
        _bufferCom->Update_Particles(timeDelta);
    }
}

void Particle_Point::Late_Update(float timeDelta)
{
    if (Is_Destroy())
        return;

    if (_addToBlendGroup)
    {
        GAME->Add_RenderGroup(ERenderGroup::Blend, GetSharedPtr());
    }
    else
    {
        GAME->Add_RenderGroup(ERenderGroup::NonBlend, GetSharedPtr());
    }
}

HRESULT Particle_Point::Render()
{
    CHECK_FAILED(GameObject::Render(), E_FAIL);

    CHECK_NULL(_shaderCom, E_FAIL);
    CHECK_NULL(_bufferCom, E_FAIL);

    CHECK_FAILED(_shaderCom->Begin_Pass(Resolve_PassIndex()), E_FAIL);
    CHECK_FAILED(_bufferCom->Bind_Resources(), E_FAIL);
    CHECK_FAILED(_bufferCom->Render(), E_FAIL);

    return S_OK;
}

HRESULT Particle_Point::Bind_ShaderResources()
{
    CHECK_NULL(_shaderCom, E_FAIL);
    CHECK_NULL(_textureCom, E_FAIL);
    CHECK_NULL(_transformCom, E_FAIL);

    const Matrix& worldMatrix = _transformCom->Get_WorldMatrix();

    CHECK_FAILED(_shaderCom->Bind_Matrix("g_WorldMatrix", &worldMatrix), E_FAIL);
    CHECK_FAILED(_shaderCom->Bind_Matrix("g_ViewMatrix", GAME->Get_Transform(ETransformState::View)), E_FAIL);
    CHECK_FAILED(_shaderCom->Bind_Matrix("g_ProjMatrix", GAME->Get_Transform(ETransformState::Proj)), E_FAIL);

    CHECK_FAILED(GAME->Bind_CamPosition(_shaderCom, "g_CamPosition"), E_FAIL);

    CHECK_FAILED(_textureCom->Bind_SRV(_shaderCom, "g_DiffuseTexture", _textureIndex), E_FAIL);
    if (_maskTextureCom)
        {CHECK_FAILED(_maskTextureCom->Bind_SRV(_shaderCom, "g_MaskTexture", 0), E_FAIL);}
    else
        CHECK_FAILED(_shaderCom->Bind_SRV("g_MaskTexture", nullptr), E_FAIL);

    if (_opacityTextureCom)
        {CHECK_FAILED(_opacityTextureCom->Bind_SRV(_shaderCom, "g_OpacityTexture", 0), E_FAIL);}
    else
        CHECK_FAILED(_shaderCom->Bind_SRV("g_OpacityTexture", nullptr), E_FAIL);
    CHECK_FAILED(_shaderCom->Bind_RawValue("g_ColorTint", &_colorTint, sizeof(Vec4)), E_FAIL);
    CHECK_FAILED(_shaderCom->Bind_RawValue("g_Opacity", &_opacity, sizeof(float)), E_FAIL);
    CHECK_FAILED(_shaderCom->Bind_RawValue("g_EmissiveStrength", &_emissiveStrength, sizeof(float)), E_FAIL);
    CHECK_FAILED(_shaderCom->Bind_RawValue("g_CustomParams0", &_customParams0, sizeof(Vec4)), E_FAIL);
    CHECK_FAILED(_shaderCom->Bind_RawValue("g_CustomParams1", &_customParams1, sizeof(Vec4)), E_FAIL);
    {
        const int hasMaskTexture = (_maskTextureCom != nullptr) ? 1 : 0;
        const int hasOpacityTexture = (_opacityTextureCom != nullptr) ? 1 : 0;
        const int blendMode = static_cast<int>(_blendMode);
        CHECK_FAILED(_shaderCom->Bind_RawValue("g_HasMaskTexture", &hasMaskTexture, sizeof(int)), E_FAIL);
        CHECK_FAILED(_shaderCom->Bind_RawValue("g_HasOpacityTexture", &hasOpacityTexture, sizeof(int)), E_FAIL);
        CHECK_FAILED(_shaderCom->Bind_RawValue("g_BlendMode", &blendMode, sizeof(int)), E_FAIL);
    }

    // Static Point는 데칼처럼 계속 유지돼야 하므로 lifetime fade를 끈다.
    const int useLifetimeFade = _useLifetimeFade ? 1 : 0;
    const int useFlipbook = _flipbook.enabled ? 1 : 0;
    const int flipbookColumns = (std::max)(_flipbook.columns, 1);
    const int flipbookRows = (std::max)(_flipbook.rows, 1);
    CHECK_FAILED(_shaderCom->Bind_RawValue("g_UseLifetimeFade", &useLifetimeFade, sizeof(int)), E_FAIL);
    CHECK_FAILED(_shaderCom->Bind_RawValue("g_UseFlipbook", &useFlipbook, sizeof(int)), E_FAIL);
    CHECK_FAILED(_shaderCom->Bind_RawValue("g_FlipbookColumns", &flipbookColumns, sizeof(int)), E_FAIL);
    CHECK_FAILED(_shaderCom->Bind_RawValue("g_FlipbookRows", &flipbookRows, sizeof(int)), E_FAIL);
    CHECK_FAILED(_shaderCom->Bind_RawValue("g_FlipbookFps", &_flipbook.fps, sizeof(float)), E_FAIL);
    CHECK_FAILED(_shaderCom->Bind_RawValue("g_FlipbookStartFrame", &_flipbook.startFrame, sizeof(int)), E_FAIL);
    CHECK_FAILED(_shaderCom->Bind_RawValue("g_FlipbookEndFrame", &_flipbook.endFrame, sizeof(int)), E_FAIL);
    {
        const int flipbookLoop = _flipbook.loop ? 1 : 0;
        CHECK_FAILED(_shaderCom->Bind_RawValue("g_FlipbookLoop", &flipbookLoop, sizeof(int)), E_FAIL);
    }

    return S_OK;
}

void Particle_Point::Set_ColorTint(const Vec4& colorTint)
{
    _colorTint = colorTint;
}

void Particle_Point::Set_Opacity(float opacity)
{
    _opacity = opacity;
}

HRESULT Particle_Point::Ready_Components(const FParticlePointDesc& desc)
{
    CHECK_FAILED(Add_Component(desc.shaderType, _shaderCom), E_FAIL);
    CHECK_FAILED(Resolve_TextureComponent(desc), E_FAIL);
    CHECK_FAILED(Resolve_OptionalTextureComponent(desc.maskTextureGuid, _maskTextureCom), E_FAIL);
    CHECK_FAILED(Resolve_OptionalTextureComponent(desc.opacityTextureGuid, _opacityTextureCom), E_FAIL);

    auto bufferDesc = desc.bufferDesc;
    CHECK_FAILED(Add_Component(Protocol::COMPONENT_TYPE_VIBUFFER_PARTICLE_POINT, _bufferCom, &bufferDesc), E_FAIL);

    return S_OK;
}

HRESULT Particle_Point::Resolve_TextureComponent(const FParticlePointDesc& desc)
{
    if (!desc.textureGuid.empty())
    {
        const uint32 texKey = static_cast<uint32>(hash<string>{}(desc.textureGuid));
        if (SUCCEEDED(Add_Component(texKey, _textureCom)))
            return S_OK;

        const wstring resolvedPath = GAME->Resolve_AssetPath(desc.textureGuid);
        if (resolvedPath.empty())
            return E_FAIL;

        auto proto = Texture::Create(_device, _context, resolvedPath, 1);
        CHECK_NULL(proto, E_FAIL);

        GAME->Add_Component_Prototype(0, texKey, proto);
        return Add_Component(0, texKey, _textureCom);
    }

    return Add_Component(desc.textureType, _textureCom);
}

HRESULT Particle_Point::Resolve_OptionalTextureComponent(const string& textureGuid, Shared<Texture>& outTextureCom)
{
    outTextureCom.reset();

    if (textureGuid.empty())
        return S_OK;

    const uint32 texKey = static_cast<uint32>(hash<string>{}(textureGuid));
    if (SUCCEEDED(Add_Component(texKey, outTextureCom)))
        return S_OK;

    const wstring resolvedPath = GAME->Resolve_AssetPath(textureGuid);
    if (resolvedPath.empty())
        return E_FAIL;

    auto proto = Texture::Create(_device, _context, resolvedPath, 1);
    CHECK_NULL(proto, E_FAIL);

    GAME->Add_Component_Prototype(0, texKey, proto);
    return Add_Component(0, texKey, outTextureCom);
}

uint32 Particle_Point::Resolve_PassIndex() const
{
    switch (_blendMode)
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

Shared<GameObject> Particle_Point::Create(ComPtr<Device> device, ComPtr<DeviceContext> context)
{
    auto instance = make_shared<Particle_Point>(device, context);

    if (FAILED(instance->Initialize_Prototype()))
    {
        MSG_BOX("Failed to Created : Particle_Point");
        return nullptr;
    }

    return instance;
}

Shared<GameObject> Particle_Point::Clone(void* arg)
{
    auto clone = make_shared<Particle_Point>(*this);

    if (FAILED(clone->Initialize(arg)))
    {
        MSG_BOX("Failed to Cloned : Particle_Point");
        return nullptr;
    }

    return clone;
}

void Particle_Point::Free()
{
    GameObject::Free();
}
