#include "pch.h"
#include "CombatStat.h"

CombatStat::CombatStat(ComPtr<Device> device, ComPtr<DeviceContext> context)
    : Component(device, context)
{
}

CombatStat::CombatStat(const CombatStat& rhs)
    : Component(rhs)
    , _maxHp(rhs._maxHp)
    , _hp(rhs._maxHp)
    , _maxMp(rhs._maxMp)
    , _mp(rhs._maxMp)
    , _speed(rhs._speed)
    , _attack(rhs._attack)
    , _defense(rhs._defense)
{
}

CombatStat::~CombatStat()
{
}

HRESULT CombatStat::Initialize_Prototype()
{

    return S_OK;
}

HRESULT CombatStat::Initialize(void* arg)
{
    FCombatStatDesc* desc = static_cast<FCombatStatDesc*>(arg);

    _maxHp = desc->maxHp;
    _hp = desc->maxHp;

    _maxMp = desc->maxMp;
    _mp = desc->maxMp;

    _speed = desc->speed;
    _attack = desc->attack;
    _defense = desc->defense;

    return S_OK;
}

void CombatStat::Set_Hp(float hp)
{
    _hp = ::clamp(_hp, 0.f, _maxHp);
}

void CombatStat::Set_Mp(float mp)
{
    _mp = ::clamp(_mp, 0.f, _maxMp);
}

void CombatStat::Take_Damage(float damage)
{
    float actualDamage = max(0.f, damage - _defense);

    Set_Hp(actualDamage);
}

void CombatStat::Heal(float amount)
{
    Set_Hp(_hp + amount);
}

json CombatStat::To_Json() const
{
    json j;

    j = Component::To_Json(); // 타입 받아오기

    j["hp"] = _hp;
    j["maxHp"] = _maxHp;
    j["mp"] = _mp;
    j["maxMp"] = _maxHp;
    j["attack"] = _attack;
    j["defense"] = _defense;
    j["speed"] = _speed;

    return j;
}

void CombatStat::From_Json(const json& data)
{
    if (data.contains("maxHp")) _maxHp = data["maxHp"];
    if (data.contains("hp")) _hp = data["hp"];
    if (data.contains("maxMp")) _maxMp = data["maxMp"];
    if (data.contains("mp")) _mp = data["mp"];
    if (data.contains("attack")) _attack = data["attack"];
    if (data.contains("defense")) _defense = data["defense"];
    if (data.contains("speed")) _speed = data["speed"];
}

shared_ptr<CombatStat> CombatStat::Create(ComPtr<Device> device, ComPtr<DeviceContext> context)
{
    auto instance = make_shared<CombatStat>(device, context);

    if (FAILED(instance->Initialize_Prototype()))
    {
        MSG_BOX("Failed to Create : CombatStat");

        return nullptr;
    }

    return instance;
}

shared_ptr<Component> CombatStat::Clone(void* arg)
{
    auto instance = make_shared<CombatStat>(*this);

    if (FAILED(instance->Initialize(arg)))
    {
        MSG_BOX("Failed to Clone : CombatStat");

        return nullptr;
    }

    return instance;
}

void CombatStat::Free()
{
    Component::Free();
}
