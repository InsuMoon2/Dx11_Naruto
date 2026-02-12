#include "pch.h"
#include "TestPlayer.h"
#include "CombatStat.h"

#include "Texture.h"
#include "Shader.h"
#include "VIBuffer_Rect.h"

TestPlayer::TestPlayer(ComPtr<Device> device, ComPtr<DeviceContext> context)
    : GameObject(device, context)
{
    Set_ObjectType(Protocol::OBJECT_TYPE_PLAYER);
}

TestPlayer::TestPlayer(const TestPlayer& rhs)
    : GameObject(rhs)
{

}

HRESULT TestPlayer::Initialize_Prototype()
{

    return S_OK;
}

HRESULT TestPlayer::Initialize(void* arg)
{
    CHECK_FAILED(GameObject::Initialize(arg), E_FAIL);

    CombatStat::FCombatStatDesc desc;
    desc.maxHp = 200.f;
    desc.attack = 100.f;

    _transformCom->Set_LocalPosition(0.f, 0.f, -5.f);

    Add_Component(ETOI(ELevelType::Static), Protocol::COMPONENT_TYPE_COMBAT_STAT, _combatStat, &desc);

    Add_Component(ETOI(ELevelType::Static),
        Protocol::COMPONENT_TYPE_SHADER, _shaderCom);

    Add_Component(ETOI(ELevelType::GamePlay),
        Protocol::COMPONENT_TYPE_TEXTURE_DEFAULT, _textureCom);

    Add_Component(ETOI(ELevelType::Static),
        Protocol::COMPONENT_TYPE_RECT, _bufferCom);

    return S_OK;
}

void TestPlayer::Priority_Update(float timeDelta)
{
    GameObject::Priority_Update(timeDelta);
}

void TestPlayer::Update(float timeDelta)
{
    GameObject::Update(timeDelta);
}

void TestPlayer::Late_Update(float timeDelta)
{
    GameObject::Late_Update(timeDelta);

    GAME->Add_RenderGroup(ERenderGroup::NonBlend, this->GetSharedPtr());
}

HRESULT TestPlayer::Render()
{
    GameObject::Render();

    Matrix worldMatrix = _transformCom->Get_WorldMatrix();
    // Scene_View와 동일한 카메라 설정 사용
    Matrix viewMatrix = Matrix::Identity;

    Matrix projMatrix = Matrix::CreatePerspectiveFieldOfView(
        XMConvertToRadians(60.f),
        1280.f / 720.f,  // TODO: 나중에 동적으로
        0.1f,
        1000.f
    );

    _shaderCom->Bind_Matrix("g_WorldMatrix", &worldMatrix);
    _shaderCom->Bind_Matrix("g_ViewMatrix", &viewMatrix);
    _shaderCom->Bind_Matrix("g_ProjMatrix", &projMatrix);

    CHECK_FAILED(_textureCom->Bind_SRV(_shaderCom, "g_Texture", 0), E_FAIL);
    CHECK_FAILED(_shaderCom->Begin(0), E_FAIL);
    CHECK_FAILED(_bufferCom->Bind_Resources(), E_FAIL);
    CHECK_FAILED(_bufferCom->Render(), E_FAIL);

    return S_OK;
}

shared_ptr<GameObject> TestPlayer::Create(ComPtr<Device> device, ComPtr<DeviceContext> context)
{
    auto instance = make_shared<TestPlayer>(device, context);

    if (FAILED(instance->Initialize_Prototype()))
    {
        return nullptr;
    }

    return instance;
}

shared_ptr<GameObject> TestPlayer::Clone(void* arg)
{
    auto instance = make_shared<TestPlayer>(*this);

    if (FAILED(instance->Initialize(arg)))
    {
        return nullptr;
    }

    return instance;
}
