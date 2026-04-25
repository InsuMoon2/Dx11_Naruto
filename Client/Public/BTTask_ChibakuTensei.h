#pragma once

#include "BTTask.h"

NS_BEGIN(Client)

enum class EChibakuTenseiTaskPhase
{
    Approach,
    Attack,
    Finish,
};

class BTTask_ChibakuTensei : public BTTask
{
    GENERATED_BT_REFLECTION(BTTask_ChibakuTensei)

public:
    explicit BTTask_ChibakuTensei();
    explicit BTTask_ChibakuTensei(const BTTask_ChibakuTensei& rhs);
    virtual ~BTTask_ChibakuTensei() = default;

public:
    void Initialize() override;
    EBTNodeResult Update(float timeDelta) override;

private:
    string _targetObjectKey = "TargetObjectKey";
    string _cooldownRemainKey = "ChibakuTenseiCooldownRemain";

    string _globalCooldownRemainKey = "BossSkillGlobalCooldownRemain";
    string _startAnimState = "Skill_ChibakuTensei_Start";

    string _attackAnimState = "Skill_ChibakuTensei_Attack";
    string _hitAnimStateOverride = "Hit_Launch";

    float _cooldown = 14.f;
    float _globalCooldown = 4.5f;

    float _approachDuration = 1.2f;
    float _attackStartRange = 2.4f;

    float _hitRange = 3.0f;
    float _impactTime = 0.35f;

    float _attackDuration = 1.0f;
    bool _useSprint = true;

    bool _faceTarget = true;

    bool _requestAnimEnd = true;
    float _damage = 2.f;

    float _launchPower = 8.f;
    float _launchUp = 14.f;

    EChibakuTenseiTaskPhase _phase = EChibakuTenseiTaskPhase::Approach;

    float _phaseElapsed = 0.f;
    bool _impactApplied = false;

    bool _previousOrientRotationToMovement = false;

private:
    bool Is_CooldownBlocked(const Shared<Blackboard>& blackboard) const;
    void Tick_Phase(float timeDelta);

    void Begin_Chibaku(const Shared<Blackboard>& blackboard, const Shared<GameObject>& owner);

    bool Update_Approach(const Shared<Blackboard>& blackboard, const Shared<GameObject>& owner, const Shared<GameObject>& target);

    void Begin_Attack(const Shared<Blackboard>& blackboard);
    void Update_AttackImpact(const Shared<GameObject>& owner, const Shared<GameObject>& target);

    bool Is_TargetInHitRange(const Shared<GameObject>& owner, const Shared<GameObject>& target) const;

    void Apply_ChibakuHit(const Shared<GameObject>& owner, const Shared<GameObject>& target) const;
    void Finish_Chibaku(const Shared<Blackboard>& blackboard, const Shared<GameObject>& owner, bool writeCooldown);
    void Request_AnimState(const Shared<Blackboard>& blackboard, const string& animState) const;

    void Face_Target(const Shared<GameObject>& owner, const Shared<GameObject>& target) const;

public:
    static Shared<BTTask_ChibakuTensei> Create();
    Shared<BTNode> Clone() override;
};

NS_END
