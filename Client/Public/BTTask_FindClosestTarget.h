#pragma once

#include "BTTask.h"

NS_BEGIN(Client)

class BTTask_FindClosestTarget : public BTTask
{
    GENERATED_BT_REFLECTION(BTTask_FindClosestTarget)

public:
    explicit BTTask_FindClosestTarget();
    explicit BTTask_FindClosestTarget(const BTTask_FindClosestTarget& rhs);
    virtual ~BTTask_FindClosestTarget() = default;

public:
    void Initialize() override;
    EBTNodeResult Update(float timeDelta) override;

private:
    string _targetObjectKey = "TargetObjectKey";
    string _targetLocationKey = "TargetLocationKey";

    // 탐색 반경
    float _searchRadius = 30.f;

public:
    static Shared<BTTask_FindClosestTarget> Create();
    Shared<BTNode> Clone() override;
};

NS_END
