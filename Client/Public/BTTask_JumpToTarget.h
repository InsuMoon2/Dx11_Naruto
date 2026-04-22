#pragma once

#include "BTTask.h"

NS_BEGIN(Client)

class BTTask_JumpToTarget : public BTTask
{
    GENERATED_BT_REFLECTION(BTTask_JumpToTarget)

public:
    explicit BTTask_JumpToTarget();
    explicit BTTask_JumpToTarget(const BTTask_JumpToTarget& rhs);
    virtual ~BTTask_JumpToTarget() = default;

public:
    void Initialize() override;
    EBTNodeResult Update(float timeDelta) override;

private:
    string _targetObjectKey = "TargetObjectKey";

    string _desiredCombatModeKey = "DesiredCombatMode";

    string _jumpAnimState = "JumpDash";

    float _acceptanceRadius = 2.5f;

    float _horizontalSpeed = 12.f;
    bool _startedJump = false;

public:
    static Shared<BTTask_JumpToTarget> Create();
    Shared<BTNode> Clone() override;
};

NS_END
