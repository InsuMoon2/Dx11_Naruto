#pragma once

#include "BTTask.h"

NS_BEGIN(Engine)

class ENGINE_DLL BTTask_MoveTo : public BTTask
{
    GENERATED_BT_REFLECTION(BTTask_MoveTo)

public:
    explicit BTTask_MoveTo();
    explicit BTTask_MoveTo(const BTTask_MoveTo& rhs);
    virtual ~BTTask_MoveTo() = default;

public:
    void            Initialize() override;
    EBTNodeResult   Update(float timeDelta) override;

public:
    void Set_TargetKey(const string& key) { _targetLocationKey = key; }
    void Set_AcceptanceRadius(float radius) { _acceptanceRadius = radius; }

private:
    string  _targetObjectKey = "TargetObjectKey";
    string  _targetLocationKey = "TargetLocationKey";

    float   _acceptanceRadius = 1.f;
    bool    _useSprint = false;       // 속도
    
    bool    _controlMoveAnim = true;  // Move 테스크가 애니메이션을 세팅할지?
    string  _moveAnimState = "Run";
    string  _idleAnimState = "Idle";

    bool    _writeAnimDirection = true;

public:
    static Shared<BTTask_MoveTo> Create();
    Shared<BTNode> Clone() override;
};

NS_END
