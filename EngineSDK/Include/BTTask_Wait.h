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
    void Initialize() override;
    EBTNodeResult Update(float timeDelta) override;

    void OnDraw_Inspector() override;
    json Serialize_ToJson() override;
    void Deserialize_FromJson(const json& data) override;

public:
    float _waitTime = 1.f;
    float _elapsed = 0.f;

public:
    virtual Shared<BTNode> Clone() override;

};

NS_END
