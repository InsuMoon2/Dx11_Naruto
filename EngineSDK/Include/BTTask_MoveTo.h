#pragma once

#include "BTTask.h"

NS_BEGIN(Engine)

class ENGINE_DLL BTTask_MoveTo : public BTTask
{
public:
    explicit BTTask_MoveTo();
    explicit BTTask_MoveTo(const BTTask_MoveTo& rhs);
    virtual ~BTTask_MoveTo();

public:
    void            Initialize() override;
    EBTNodeResult   Update(float timeDelta) override;

public:
    void Set_TargetKey(const string& key) { _targetKey = key; }
    void Set_AcceptanceRadius(float radius) { _acceptanceRadius = radius; }

private:
    string _targetKey = "TargetLocation";
    float _acceptanceRadius = 1.f;

public:
    Shared<BTNode> Clone() override;
};

NS_END
