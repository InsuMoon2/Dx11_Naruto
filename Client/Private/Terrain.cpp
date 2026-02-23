#include "pch.h"
#include "Terrain.h"
#include "GameInstance.h"
#include "Texture.h"
#include "Shader.h"
#include "VIBuffer_Rect.h"
#include "VIBuffer_Terrain.h"

Terrain::Terrain(ComPtr<Device> device, ComPtr<DeviceContext> context)
    : GameObject(device, context)
{
    Set_ObjectType(Protocol::OBJECT_TYPE_TERRAIN);
}

Terrain::Terrain(const Terrain& rhs)
    : GameObject(rhs)
{
}

HRESULT Terrain::Initialize_Prototype()
{
    GameObject::Initialize_Prototype();

    return S_OK;
}

HRESULT Terrain::Initialize(void* arg)
{
    CHECK_FAILED(GameObject::Initialize(arg), E_FAIL);

    CHECK_FAILED(Ready_Components(), E_FAIL);

    return S_OK;
}

void Terrain::BeginPlay()
{
    GameObject::BeginPlay();


}

void Terrain::Priority_Update(float fTimeDelta)
{
    GameObject::Priority_Update(fTimeDelta);

}

void Terrain::Update(float fTimeDelta)
{
    GameObject::Update(fTimeDelta);

}

void Terrain::Late_Update(float fTimeDelta)
{
    GameObject::Late_Update(fTimeDelta);

    GAME->Add_RenderGroup(ERenderGroup::NonBlend, this->GetSharedPtr());
}

HRESULT Terrain::Render()
{
    GameObject::Render();

    Matrix worldMatrix = _transformCom->Get_WorldMatrix();
    _shaderCom->Bind_Matrix("g_WorldMatrix", &worldMatrix);
    _shaderCom->Bind_Matrix("g_ViewMatrix", GAME->Get_Transform(ETransformState::View));
    _shaderCom->Bind_Matrix("g_ProjMatrix", GAME->Get_Transform(ETransformState::Proj));
    
    CHECK_FAILED(_textureCom->Bind_SRV(_shaderCom, "g_Texture", 0), E_FAIL);

    _shaderCom->Begin(0);
    CHECK_FAILED(_bufferCom->Bind_Resources(), E_FAIL);
    CHECK_FAILED(_bufferCom->Render(), E_FAIL);

    return S_OK;
}

HRESULT Terrain::Ready_Components()
{
    CHECK_FAILED(Add_Component(Protocol::COMPONENT_TYPE_TEXTURE_TERRAIN, _textureCom), E_FAIL);

    CHECK_FAILED(Add_Component(Protocol::COMPONENT_TYPE_SHADER_VTXNORTEX, _shaderCom), E_FAIL);
    CHECK_FAILED(Add_Component(Protocol::COMPONENT_TYPE_TERRAIN, _bufferCom), E_FAIL);

    return S_OK;
}

Shared<Terrain> Terrain::Create(ComPtr<Device> device, ComPtr<DeviceContext> context)
{
    auto instance = make_shared<Terrain>(device, context);
    if (FAILED(instance->Initialize_Prototype()))
    {
        MSG_BOX("Failed to Created : Terrain");
        instance->Free();

        return nullptr;
    }

    return instance;
}

Shared<GameObject> Terrain::Clone(void* arg)
{
    auto clone = make_shared<Terrain>(*this);

    if (FAILED(clone->Initialize(arg)))
    {
        MSG_BOX("Failed to Cloned : Terrain");
        clone->Free();

        return nullptr;
    }

    return clone;
}

void Terrain::Free()
{
    GameObject::Free();
}
