#pragma once

#include "BTTask.h"

NS_BEGIN(Client)

class BTTask_Hit : public BTTask
{
    GENERATED_BT_REFLECTION(BTTask_Hit)

public:
    explicit BTTask_Hit();
    explicit BTTask_Hit(const BTTask_Hit& rhs);
    virtual ~BTTask_Hit();

public:
    void Initialize() override;
    EBTNodeResult Update(float timeDelta) override;

private:
    string _hitFlagKey = "IsHit";
    string _hitAnimState = "Hit";

    bool   _requestAnimEnd = false;
    bool   _startedHit = false;

public:
    static Shared<BTTask_Hit> Create();
    Shared<BTNode> Clone() override;
};

NS_END

