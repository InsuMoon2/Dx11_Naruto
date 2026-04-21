#pragma once

#include "BTTask.h"

NS_BEGIN(Engine)
class Blackboard;
class GameObject;
NS_END

NS_BEGIN(Client)

class BTTask_RetreatAndWait : public BTTask
{
    GENERATED_BT_REFLECTION(BTTask_RetreatAndWait)

public:
    explicit BTTask_RetreatAndWait();
    explicit BTTask_RetreatAndWait(const BTTask_RetreatAndWait& rhs);
    virtual ~BTTask_RetreatAndWait() = default;

public:
    void Initialize() override;
    EBTNodeResult Update(float timeDelta) override;

private:
    void Finish_Retreat(const Shared<Blackboard>& blackboard, const Shared<GameObject>& owner);

private:
    string _targetObjectKey = "TargetObjectKey";
    string _retreatAnimState = "Retreat";

    float _retreatDuration = 0.45f;
    bool _lockFacingToTarget = true;

    bool _faceTarget = true;
    bool _useSprint = false;

    bool _requestAnimEnd = false;
    bool _stopMovementOnFinish = true;

    bool _startedRetreat = false;

    float _elapsedRetreatTime = 0.f;

    bool _previousOrientRotationToMovement = false;

    int32 _selectedRetreatSide = 1;

public:
    static Shared<BTTask_RetreatAndWait> Create();
    Shared<BTNode> Clone() override;
};

NS_END
