#pragma once

#include "Component.h"
#include "IReplicable.h"

NS_BEGIN(Client)

class CombatStat : public Component, public IReplicable
{
    GENERATED_COMPONENT(CombatStat, Protocol::COMPONENT_TYPE_COMBAT_STAT);

public:
    struct FCombatStatDesc
    {
        float maxHp = 100.f;
        float maxMp = 100.f;
        float speed = 5.f;
        float attack = 10.f;
        float defense = 5.f;
    };

public:
    explicit CombatStat(ComPtr<Device> device, ComPtr<DeviceContext> context);
    explicit CombatStat(const CombatStat& rhs);
    virtual ~CombatStat();

public:
    HRESULT Initialize_Prototype() override;
    HRESULT Initialize(void* arg) override;

public:
    float   Get_Hp() const { return _hp; }
    float   Get_Mp() const { return _mp; }
    float   Get_HpRatio() const { return _hp / _maxHp; }
    bool    Is_Dead() const { return _hp <= 0.f; }

private:
    void    Set_Hp(float hp);
    void    Set_Mp(float mp);

public:
    void    Take_Damage(float damage);
    void    Heal(float amount);

public: /* Protobuf */
    void    Sync_FromProtobuf(Message& message) override;
    void    Serialize_ToProtobuf(Message& message) const override;

private:
    float _maxHp        = {};
    float _hp           = {};
    float _maxMp        = {};
    float _mp           = {};
    float _speed        = {};
    float _attack       = {};
    float _defense      = {};

public:
    static shared_ptr<CombatStat> Create(ComPtr<Device> device, ComPtr<DeviceContext> context);
    shared_ptr<Component> Clone(void* arg) override;
    void Free() override;
};

NS_END
