#include "pch.h"
#include "Monster_Wood.h"
#include "CharkraMove_Component.h"
#include "GameObject_Factory.h"

REGISTER_GAMEOBJECT(Monster_Wood, Protocol::OBJECT_TYPE_MONSTER_WOOD)

Monster_Wood::Monster_Wood(ComPtr<Device> device, ComPtr<DeviceContext> context)
    : Monster(device, context)
{
}

Monster_Wood::Monster_Wood(const Monster_Wood& rhs)
    : Monster(rhs)
    , _chakraTrail(rhs._chakraTrail)
{
}

HRESULT Monster_Wood::Initialize_Prototype()
{
    return Monster::Initialize_Prototype();
}

HRESULT Monster_Wood::Initialize(void* arg)
{
    CHECK_FAILED(Monster::Initialize(arg), E_FAIL);

    return S_OK;
}

void Monster_Wood::Update(float timeDelta)
{
    Monster::Update(timeDelta);

    if (_chakraTrail)
        _chakraTrail->Update_ChakraMove(timeDelta);
}

void Monster_Wood::Late_Update(float timeDelta)
{
    Monster::Late_Update(timeDelta);
}

HRESULT Monster_Wood::Render()
{
    CHECK_FAILED(Monster::Render(), E_FAIL);

    if (_chakraTrail)
        CHECK_FAILED(_chakraTrail->Render(), E_FAIL);
}

HRESULT Monster_Wood::Ready_Components()
{
    CHECK_FAILED(Monster::Ready_Components(), E_FAIL);

    CHECK_FAILED(Add_Component(Protocol::COMPONENT_TYPE_CHAKRA_MOVE, _chakraTrail), E_FAIL);

    if (_chakraTrail)
    {
        _chakraTrail->Set_TrailTintColor(Vec4(1.00f, 0.18f, 0.12f, 1.f));
        _chakraTrail->Set_TrailEmissiveStrength(1.8f);
    }

    return S_OK;
}

Protocol::OBJECT_TYPE Monster_Wood::Get_EnemyObjectType() const
{
    return Protocol::OBJECT_TYPE_MONSTER_WOOD;
}

Shared<GameObject> Monster_Wood::Create(ComPtr<Device> device, ComPtr<DeviceContext> context)
{
    auto instance = make_shared<Monster_Wood>(device, context);

    if (FAILED(instance->Initialize_Prototype()))
    {
        MSG_BOX("Failed to Create : Monster_Wood");
        return nullptr;
    }

    return instance;
}

Shared<GameObject> Monster_Wood::Clone(void* arg)
{
    auto clone = make_shared<Monster_Wood>(*this);

    if (FAILED(clone->Initialize(arg)))
    {
        MSG_BOX("Failed to Clone : Monster_Wood");
        return nullptr;
    }

    return clone;
}

void Monster_Wood::Free()
{
    Monster::Free();
}
