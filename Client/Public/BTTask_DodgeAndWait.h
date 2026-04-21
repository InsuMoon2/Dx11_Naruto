#pragma once

#include "BTTask.h"

NS_BEGIN(Client)

class BTTask_DodgeAndWait : public BTTask
{
    GENERATED_BT_REFLECTION(BTTask_DodgeAndWait)

public:
    explicit BTTask_DodgeAndWait();
    explicit BTTask_DodgeAndWait(const BTTask_DodgeAndWait& rhs);
    virtual ~BTTask_DodgeAndWait() = default;

public:
    void Initialize() override;
    EBTNodeResult Update(float timeDelta) override;

private:
    int32 Select_DodgeSide(const Shared<Blackboard>& blackboard);
    string Resolve_DodgeStateName(int32 dodgeSide) const;

private:
    string _dodgeLeftState  = "Dodge_Left";
    string _dodgeRightState = "Dodge_Right";

    bool _stopMovement = true;
    bool _requestAnimEnd = false;

    // 왼쪽 오른쪽 번갈아가게
    bool _avoidSameSideTwice = true;

    string _lastDodgeSideKey = "LastDodgeSide";

    bool _startedDodge = false;

    int32 _selectedDodgeSide = 0;
    string _selectedDodgeState = "";


public:
    static Shared<BTTask_DodgeAndWait> Create();
    Shared<BTNode> Clone() override;
};

NS_END
