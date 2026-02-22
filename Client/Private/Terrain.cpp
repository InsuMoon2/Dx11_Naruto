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

}

Terrain::Terrain(const Terrain& rhs)
    : GameObject(rhs)
{
}

HRESULT Terrain::Initialize_Prototype()
{
    return GameObject::Initialize_Prototype();
}

HRESULT Terrain::Initialize(void* arg)
{
    CHECK_FAILED(GameObject::Initialize(arg), E_FAIL);

    CHECK_FAILED(Ready_Components(), E_FAIL);
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

    //if (FAILED(__super::Bind_ShaderResource(m_pShaderCom, "g_ViewMatrix", D3DTS::VIEW)))
    //	return E_FAIL;

    //if (FAILED(__super::Bind_ShaderResource(m_pShaderCom, "g_ProjMatrix", D3DTS::PROJ)))
    //	return E_FAIL;

    CHECK_FAILED(_textureCom->Bind_SRV(_shaderCom, "g_Texture", 0), E_FAIL);
    CHECK_FAILED(_shaderCom->Begin(0), E_FAIL);

    CHECK_FAILED(_bufferCom->Bind_Resources(), E_FAIL);
    CHECK_FAILED(_bufferCom->Render(), E_FAIL);

    _shaderCom->Begin(0);


    return S_OK;
}

HRESULT Terrain::Ready_Components()
{
    CHECK_FAILED(Add_Component(ETOI(ELevelType::Logo),
        Protocol::COMPONENT_TYPE_TEXTURE_DEFAULT, _textureCom), E_FAIL);

    CHECK_FAILED(Add_Component(ETOI(ELevelType::Static),
        Protocol::COMPONENT_TYPE_SHADER, _shaderCom), E_FAIL);

    CHECK_FAILED(Add_Component(ETOI(ELevelType::Static),
        Protocol::COMPONENT_TYPE_RECT, _bufferCom), E_FAIL);
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
    auto instance = make_shared<Terrain>(*this);
    if (FAILED(instance->Initialize(arg)))
    {
        MSG_BOX("Failed to Created : Terrain");
        instance->Free();

        return nullptr;
    }

    return instance;
}

void Terrain::Free()
{
    GameObject::Free();
}
