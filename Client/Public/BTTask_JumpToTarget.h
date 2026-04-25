#pragma once

#include "BTTask.h"

NS_BEGIN(Engine)
class MovementComponent;
NS_END

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
    // 접근 태스크가 끝날 때 중력/내부 상태를 원래대로 되돌리기 위해 호출한다.
    void Reset_AirApproachState(const Shared<MovementComponent>& movement);

private:
    string _targetObjectKey = "TargetObjectKey";
    string _desiredCombatModeKey = "DesiredCombatMode";

    string _jumpStartAnimState = "Jump";
    string _airApproachAnimState = "AirApproach";

    float _acceptanceRadius = 2.5f;
    float _horizontalSpeed = 12.f;
    float _verticalApproachSpeed = 8.f;

    float _startupDuration = 0.12f;
    bool _disableGravityDuringApproach = true;
    bool  _startedJump = false;

    bool _switchedToApproach = false;
    float _elapsedJumpTime = 0.f;
    bool _savedGravityEnabled = true;
    bool _hasSavedGravityState = false;

private:
    Vec3 _dashTargetPosition = Vec3::Zero;
    float _targetLeadTime = 0.12f;

    float _homingElapsed = 0.f;
    float _homingInterval = 0.1f;

    float _hardLockAfter = 0.22f;
    float _verticalLerpSpeed = 6.f;

    float _maxApproachDuration = 0.65f;

public:
    static Shared<BTTask_JumpToTarget> Create();
    Shared<BTNode> Clone() override;
};

NS_END
