#pragma once

#include "BTTask.h"

NS_BEGIN(Client)

class BTTask_SelectAttackType : public BTTask
{
    GENERATED_BT_REFLECTION(BTTask_SelectAttackType)

public:
    explicit BTTask_SelectAttackType();
    explicit BTTask_SelectAttackType(const BTTask_SelectAttackType& rhs);
    virtual ~BTTask_SelectAttackType() = default;

public:
     void Initialize() override;

    // 타겟의 공중 여부와 높이 차이를 보고 이번 공격 타입을 결정
    EBTNodeResult Update(float timeDelta) override;

private:
    string _targetObjectKey = "TargetObjectKey";
    string _targetIsAirborneKey = "TargetIsAirborne";
    string _targetHeightDeltaKey = "TargetHeightDelta";

    string _desiredCombatModeKey = "DesiredCombatMode";

    string _selectedAttackStateKey = "SelectedAttackState";

    string _groundAttackCycleIndexKey = "GroundAttackCycleIndex";
    string _airAttackCycleIndexKey = "AirAttackCycleIndex";

    // 높이 차이가 이 값 이상이면 공중 전투로 판단
    float _airborneHeightThreshold = 1.2f;

    string _groundAttackState01 = "Attack_01";
    string _groundAttackState02 = "Attack_02";
    string _groundAttackState03 = "Attack_03";
    string _groundAttackState04 = "Attack_04";

    string _airAttackState01 = "Attack_Air_01";
    string _airAttackState02 = "Attack_Air_02";
    string _airAttackState03 = "Attack_Air_03";
    string _airAttackState04 = "Attack_Air_04";

    string _canUseAerialAttackKey = "CanUseAerialAttack";

private:
    string Select_NextAttackState(
        const Shared<Blackboard>& blackboard,
        bool useAerialAttack) const;

public:
    static Shared<BTTask_SelectAttackType> Create();
    Shared<BTNode> Clone() override;

};

NS_END
