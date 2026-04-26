#pragma once

#include "BTTask.h"

NS_BEGIN(Client)

class BTTask_KonohaSenpung : public BTTask
{
    GENERATED_BT_REFLECTION(BTTask_KonohaSenpung)

public:
    explicit BTTask_KonohaSenpung();
    explicit BTTask_KonohaSenpung(const BTTask_KonohaSenpung& rhs);
    virtual ~BTTask_KonohaSenpung() = default;

public:
    void Initialize() override;
    EBTNodeResult Update(float timeDelta) override;

private:
    string _targetObjectKey     = "TargetObjectKey";
    string _skillAnimState      = "Skill_Konoha";
    string _cooldownRemainKey   = "KonohaSenpungCooldownRemain";

    float _minRange = 0.f; 
    float _maxRange = 3.f; 
    float _cooldown = 5.f; 
    float _skillDuration = 1.f;

    bool _faceTarget = true; 
    bool _requestAnimEnd = true;

    bool _startedSkill = false;
    float _elapsed = 0.f; 

private:
    bool Tick_Cooldown(const Shared<Blackboard>& blackboard, float timeDelta);
    bool Is_TargetInRange(const Shared<GameObject>& owner, const Shared<GameObject>& target) const;
    void Begin_Skill(const Shared<Blackboard>& blackboard, const Shared<GameObject>& owner, const Shared<GameObject>& target);
    void Finish_Skill(const Shared<Blackboard>& blackboard, bool succeeded);
    void Request_SkillAnimation(const Shared<Blackboard>& blackboard) const;
    void Face_Target(const Shared<GameObject>& owner, const Shared<GameObject>& target) const;
    void Stop_Move(const Shared<Blackboard>& blackboard) const;

public:
    static Shared<BTTask_KonohaSenpung> Create();
    Shared<BTNode> Clone() override;
};

NS_END
