#pragma once

#include "EnemyCharacter.h"

NS_BEGIN(Client)

class UI_MonsterHp;

class Monster : public EnemyCharacter
{
    GENERATED_BODY(Monster)

public:
    explicit Monster(ComPtr<Device> device, ComPtr<DeviceContext> context);
    explicit Monster(const Monster& rhs);
    virtual ~Monster();

public:
    HRESULT Initialize_Prototype() override;
    HRESULT Initialize(void* arg) override;

    json    To_Json() const override;
    void    From_Json(const json& data) override;

protected:
    HRESULT Ready_Components() override;

    // Returns the object type used by regular monsters in HUD and packets.
    Protocol::OBJECT_TYPE Get_EnemyObjectType() const override;

    // Maps regular monster BT animation names to replicated object states.
    Protocol::OBJECT_STATE_TYPE To_EnemyObjectState(const string& animStateName) const override;

    HRESULT Ready_UI();

private:
    Shared<UI_MonsterHp> _hpBar; // HP UI used only by regular monsters.

public:
    static Shared<Monster> Create(ComPtr<Device> device, ComPtr<DeviceContext> context);
    shared_ptr<GameObject> Clone(void* arg) override;
    void Free() override;
};

NS_END
