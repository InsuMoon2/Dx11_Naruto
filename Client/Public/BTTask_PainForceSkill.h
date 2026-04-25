#pragma once

#include "BTTask.h"

NS_BEGIN(Client)

enum class EPainForceSkillType
{
    BanshoTenin,
    ShinraTensei,
    ChibakuTensei,
};

class BTTask_PainForceSkill : public BTTask
{
    GENERATED_BT_REFLECTION(BTTask_PainForceSkill)

public:
    explicit BTTask_PainForceSkill();
    explicit BTTask_PainForceSkill(const BTTask_PainForceSkill& rhs);
    virtual ~BTTask_PainForceSkill() = default;

public:
    void Initialize() override;
    EBTNodeResult Update(float timeDelta) override;

private:
    string _targetObjectKey = "TargetObjectKey";
    string _skillAnimState = "Skill_ShinraTensei";
    string _cooldownRemainKey = "ShinraTenseiCooldownRemain";
    string _globalCooldownRemainKey = "BossSkillGlobalCooldownRemain"; // 모든 보스 스킬이 공유하는 쿨타임 블랙보드 키다.

    EPainForceSkillType _skillType = EPainForceSkillType::ShinraTensei;

    float _minRange = 0.f;
    float _maxRange = 5.f;

    float _cooldown = 8.f;
    float _globalCooldown = 4.f; // 스킬 하나가 끝난 뒤 다른 스킬을 막는 공용 쿨타임이다.
    float _impactTime = 0.35f;

    float _skillDuration = 1.f;
    float _launchPower = 14.f;

    float _launchUp = 2.f;

    float _damage = 0.f;

    string _hitAnimStateOverride = ""; // 스킬에 맞은 대상이 기본 HitReaction 매핑 대신 재생할 피격 애니메이션 상태명이다.

    bool _affectAllPlayersInRange = true;
    bool _faceTarget = true;

    bool _requestAnimEnd = true;

    string _effectAssetName = "";

    bool _startedSkill = false;

    float _elapsed = 0.f;
    bool _impactApplied = false;

    Vec3 _effectScale = Vec3::One;

private:
    bool Tick_Cooldown(const Shared<Blackboard>& blackboard, float timeDelta);

    vector<Shared<GameObject>> Collect_Targets(const Shared<GameObject>& owner) const;

    void Request_SkillAnimation(const Shared<Blackboard>& blackboard);
    void Apply_ForceToTarget(const Shared<GameObject>& owner, const Shared<GameObject>& target) const;

public:
    static Shared<BTTask_PainForceSkill> Create();
    Shared<BTNode> Clone() override;

};

NS_END
