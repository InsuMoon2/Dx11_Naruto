#include "pch.h"
#include "Particle_Point.h"
#include "Texture.h"
#include "Shader.h"
#include "VIBuffer_Particle_Point.h"
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

    CHECK_FAILED(_shaderCom->Begin_Pass(0), E_FAIL);
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

    return S_OK;
}

HRESULT Particle_Point::Ready_Components(const FParticlePointDesc& desc)
{
    CHECK_FAILED(Add_Component(desc.shaderType, _shaderCom), E_FAIL);
    CHECK_FAILED(Add_Component(desc.textureType, _textureCom), E_FAIL);

    auto bufferDesc = desc.bufferDesc;
    CHECK_FAILED(Add_Component(Protocol::COMPONENT_TYPE_VIBUFFER_PARTICLE_POINT, _bufferCom, &bufferDesc), E_FAIL);

    return S_OK;
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
