#pragma once

#include "BTTask.h"

NS_BEGIN(Engine)

class ENGINE_DLL BTTask_Wait : public BTTask
{
public:
    explicit BTTask_Wait(float waitTime = 1.f);
    virtual ~BTTask_Wait() = default;

public:
    virtual void Initialize() override;
    virtual EBTNodeResult Update(float timeDelta) override;

public:
    float _waitTime = 1.f;
    float _accTime = 0.f;

};

NS_END
