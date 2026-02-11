#include "pch.h"
#include "TestPlayer.h"
#include "CombatStat.h"

TestPlayer::TestPlayer(ComPtr<Device> device, ComPtr<DeviceContext> context)
    : GameObject(device, context)
{
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

    Add_Component(ETOI(ELevelType::Static), Protocol::COMPONENT_TYPE_COMBAT_STAT, _combatStat, &desc);

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
