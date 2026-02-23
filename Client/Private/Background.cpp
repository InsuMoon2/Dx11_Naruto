#include "pch.h"
#include "Background.h"
#include "GameInstance.h"
#include "Texture.h"
#include "Shader.h"
#include "VIBuffer_Rect.h"

Background::Background(ComPtr<Device> device, ComPtr<DeviceContext> context)
    : UIObject { device, context }
{
}

Background::Background(const Background& rhs)
    : UIObject{ rhs }
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
    desc.posX = 100.f;
    desc.posY = 100.f;
    desc.sizeX = 200.f;
    desc.sizeY = 200.f;

    CHECK_FAILED(UIObject::Initialize(&desc), E_FAIL);

    CHECK_FAILED(Ready_Components(), E_FAIL);

    return S_OK;
}

void Background::Priority_Update(float timeDelta)
{
    UIObject::Priority_Update(timeDelta);
}

void Background::Update(float timeDelta)
{
    UIObject::Update(timeDelta);

    _posX += 10.f * timeDelta;

    __super::Update_Transform();
}

void Background::Late_Update(float timeDelta)
{
    UIObject::Late_Update(timeDelta);

    GAME->Add_RenderGroup(ERenderGroup::UI, this->GetSharedPtr());
}

HRESULT Background::Render()
{
    UIObject::Render();

    _shaderCom->Bind_Matrix("g_WorldMatrix", &_worldMatrix);

    __super::Bind_ShaderResource(_shaderCom, "g_ViewMatrix", ETransformState::View);
    __super::Bind_ShaderResource(_shaderCom, "g_ProjMatrix", ETransformState::Proj);

    // 텍스처 바인딩 (첫번째 텍스처 사용)
    CHECK_FAILED(_textureCom->Bind_SRV(_shaderCom, "g_Texture", 0), E_FAIL);

    CHECK_FAILED(_shaderCom->Begin(0), E_FAIL);
    CHECK_FAILED(_bufferCom->Bind_Resources(), E_FAIL);
    CHECK_FAILED(_bufferCom->Render(), E_FAIL);

    return S_OK;
}

HRESULT Background::Ready_Components()
{
    CHECK_FAILED(Add_Component(ETOI(ELevelType::Logo),
        Protocol::COMPONENT_TYPE_TEXTURE_DEFAULT, _textureCom), E_FAIL);

    CHECK_FAILED(Add_Component(ETOI(ELevelType::Static),
        Protocol::COMPONENT_TYPE_SHADER, _shaderCom), E_FAIL);

    CHECK_FAILED(Add_Component(ETOI(ELevelType::Static),
        Protocol::COMPONENT_TYPE_RECT, _bufferCom), E_FAIL);

    return S_OK;
}

Shared<UIObject> Background::Create(ComPtr<Device> device, ComPtr<DeviceContext> context)
{
    auto instance = make_shared<Background>(device, context);

    if (FAILED(instance->Initialize_Prototype()))
    {
        MSG_BOX("Failed to Created : Background");

        return nullptr;
    }

    return instance;
}

Shared<GameObject> Background::Clone(void* arg)
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
    UIObject::Free();
}
