#pragma once

#include "BTTask.h"

NS_BEGIN(Engine)

class ENGINE_DLL BTTask_Wait : public BTTask
{
public:
    explicit BTTask_Wait(float waitTime = 1.f);
    explicit BTTask_Wait(const BTTask_Wait& rhs);
    virtual ~BTTask_Wait() = default;

public:
    virtual void Initialize() override;
    virtual EBTNodeResult Update(float timeDelta) override;

public:
    float _waitTime = 1.f;
    float _accTime = 0.f;

public:
    virtual Shared<BTNode> Clone() override;

};

NS_END
