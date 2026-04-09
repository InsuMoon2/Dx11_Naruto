#pragma once

#include "BTDecorator.h"

NS_BEGIN(Engine)

class ENGINE_DLL BTDecorator_Blackboard : public BTDecorator
{
    GENERATED_BT_REFLECTION(BTDecorator_Blackboard)

public:
    explicit BTDecorator_Blackboard();
    explicit BTDecorator_Blackboard(const BTDecorator_Blackboard& rhs);
    virtual ~BTDecorator_Blackboard() = default;

public:
    virtual void Initialize() override;
    virtual EBTNodeResult Update(float timeDelta) override;

private:
    string _targetKey = "IsHit";

    // 데코레이터를 통과하기 위한 기대값
    bool _expectedValue = true;

public:
    static Shared<BTDecorator_Blackboard> Create();
    virtual Shared<BTNode> Clone() override;

};

NS_END
