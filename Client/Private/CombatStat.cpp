#include "pch.h"
#include "CombatStat.h"

IMPLEMENT_REFLECTION(CombatStat)

bool CombatStat::Register_Properties()
{
    auto& info = GetStaticReflectionInfo();
    info.className = "CombatStat";

    PROPERTY_FLOAT("HP", _hp, 1.f, 9999.f);
    PROPERTY_FLOAT("Max HP", _maxHp, 1.f, 9999.f);
    PROPERTY_READONLY("MP", _mp);
    PROPERTY_FLOAT("Max MP", _maxMp, 0.f, 9999.f);
    PROPERTY_FLOAT("Attack", _attack, 0.f, 999.f);
    PROPERTY_FLOAT("Defense", _defense, 0.f, 999.f);
    PROPERTY_FLOAT("Speed", _speed, 0.f, 100.f);

    return true;
}

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
    FCombatStatDesc defaultDesc = {};
    FCombatStatDesc* desc = arg ? static_cast<FCombatStatDesc*>(arg) : &defaultDesc;

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
    _hp = ::clamp(hp, 0.f, _maxHp);
}

void CombatStat::Set_Mp(float mp)
{
    _mp = ::clamp(mp, 0.f, _maxMp);
}

void CombatStat::Take_Damage(float damage)
{
    float actualDamage = max(0.f, damage - _defense);

    Set_Hp(_hp - actualDamage);
}

void CombatStat::Heal(float amount)
{
    Set_Hp(_hp + amount);
}

void CombatStat::Sync_FromProtobuf(Message& message)
{
    Protocol::CombatStat& stat = dynamic_cast<Protocol::CombatStat&>(message);

    _maxHp      = stat.max_hp();
    _hp         = stat.current_hp();
    _maxMp      = stat.max_mp();
    _mp         = stat.current_mp();
    _attack     = stat.attack();
    _defense    = stat.defense();
    _speed      = stat.speed();
}

void CombatStat::Serialize_ToProtobuf(Message& message) const
{
    Protocol::CombatStat& stat = dynamic_cast<Protocol::CombatStat&>(message);

    stat.set_max_hp(_maxHp);
    stat.set_current_hp(_hp);
    stat.set_max_mp(_maxMp);
    stat.set_current_mp(_mp);
    stat.set_attack(_attack);
    stat.set_defense(_defense);
    stat.set_speed(_speed);
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
