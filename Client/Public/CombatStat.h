#pragma once

#include "Component.h"
#include "IReplicable.h"
#include <set>

NS_BEGIN(Engine)
class GameObject;
NS_END

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

    // ANS Collision Enable에서 오버라이드해서 Hit Animation 세팅해주게
    // 다음엔 데이터말고 그냥 여기서 다 노티파이에서 진행?
    struct FAttackSwingOverride
    {
        bool useHitReactionOverride = false;

        EHitReactionType hitReactionType = EHitReactionType::Default;

        bool  useLaunchOverride = false;
        float launchPower = 0.f;
        float launchUp = 0.f;

        bool   useHitSoundOverride = false; // ANS_CollisionEnable 같은 notify가 현재 타격 구간의 피격음을 직접 지정할지 여부다.
        int32  hitSound = 0; // useHitSoundOverride가 켜졌을 때 정수 기반 피격 사운드 ID를 FDamageEvent에 실어 보낸다.
        string hitSoundFile = ""; // useHitSoundOverride가 켜졌을 때 파일명 기반 피격 사운드를 FDamageEvent에 실어 보낸다.
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

    float   Get_Attack() const { return _attack; }

public: /* Hit Tracking */
    void    Begin_AttackSwing() { _hitTargets.clear(); }
    bool    Is_AlreadyHit(GameObject* target) const { return _hitTargets.contains(target); }
    void    Register_Hit(GameObject* target) { _hitTargets.insert(target); }

    void    Begin_Swing()
    {
        _hitTargets.clear();
        _attackSwingOverride = {};
    }

    void    Set_AttackSwingOverride(const FAttackSwingOverride& overrideDesc)
    {
        _attackSwingOverride = overrideDesc;
    }

    void    Clear_AttackSwingOverride()
    {
        _attackSwingOverride = {};
    }

private:
    void    Set_Hp(float hp);
    void    Set_Mp(float mp);

public:
    void    Take_Damage(FDamageEvent damageEvent);
    bool    Apply_Damage(Character* hitted);
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

private:
    set<GameObject*>        _hitTargets;
    FAttackSwingOverride    _attackSwingOverride{};

public:
    static shared_ptr<CombatStat> Create(ComPtr<Device> device, ComPtr<DeviceContext> context);
    shared_ptr<Component> Clone(void* arg) override;
    void Free() override;
};

NS_END
