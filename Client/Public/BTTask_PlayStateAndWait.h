#pragma once

#include "BTTask.h"

NS_BEGIN(Client)

class BTTask_PlayStateAndWait : public BTTask
{
    GENERATED_BT_REFLECTION(BTTask_PlayStateAndWait)

public:
    explicit BTTask_PlayStateAndWait();
    explicit BTTask_PlayStateAndWait(const BTTask_PlayStateAndWait& rhs);
    virtual ~BTTask_PlayStateAndWait() = default;

public:
    void Initialize() override;
    EBTNodeResult Update(float timeDelta) override;

private:
    string _stateName = "Idle";

    bool _stopMovement = true;
    bool _requestAnimEnd = false;

    // Appearance 완료 후 HasAppeared=true 같은 처리에 사용
    string _setTrueFlagKey = "";

    string _setFalseFlagKey = "";

    bool _holdOnFinished = false;
    bool _startedPlay = false;

    bool _completionApplied = false;

public:
    static Shared<BTTask_PlayStateAndWait> Create();
    Shared<BTNode> Clone() override;
};

NS_END
