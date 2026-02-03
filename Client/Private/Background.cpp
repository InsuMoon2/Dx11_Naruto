#include "pch.h"
#include "Background.h"
#include "GameInstance.h"
#include "Texture.h"

Background::Background(ComPtr<Device> device, ComPtr<DeviceContext> context)
    : GameObject { device, context }
{
}

Background::Background(const Background& rhs)
    : GameObject { rhs }
{
}

Background::~Background()
{
}

HRESULT Background::Initialize_Prototype()
{
    

    return S_OK;
}

HRESULT Background::Initialize(void* arg)
{
    FBackgroundDesc desc{};

    desc.speedPerSec = 1.f;
    desc.rotationPerSec = 1.f;

    CHECK_FAILED(GameObject::Initialize(&desc), E_FAIL);

    CHECK_FAILED(Ready_Components(), E_FAIL);

    return S_OK;
}

void Background::Priority_Update(float timeDelta)
{
    GameObject::Priority_Update(timeDelta);
}

void Background::Update(float timeDelta)
{
    GameObject::Update(timeDelta);
}

void Background::Late_Update(float timeDelta)
{
    GameObject::Late_Update(timeDelta);

    GAME->Add_RenderGroup(ERenderGroup::UI, GetSharedPtr());
}

HRESULT Background::Ready_Components()
{
    CHECK_FAILED(Add_Component<Texture>(ETOI(ELevelType::Logo), Protocol::COMPONENT_TYPE_TEXTURE_DEFAULT, _textureCom), E_FAIL);
    
    return S_OK;
}

shared_ptr<GameObject> Background::Create(ComPtr<Device> device, ComPtr<DeviceContext> context)
{
    auto instance = make_shared<Background>(device, context);

    if (FAILED(instance->Initialize_Prototype()))
    {
        MSG_BOX("Failed to Created : Background");

        return nullptr;
    }

    return instance;
}

shared_ptr<GameObject> Background::Clone(void* arg)
{
    auto instance = make_shared<Background>(*this);

    if (FAILED(instance->Initialize(arg)))
    {
        MSG_BOX("Failed to Created : Background");

        return nullptr;
    }

    return instance;
}

void Background::Free()
{
    GameObject::Free();
}
