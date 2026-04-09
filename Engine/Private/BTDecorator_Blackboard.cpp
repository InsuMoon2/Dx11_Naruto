#include "pch.h"
#include "BTDecorator_Blackboard.h"
#include "Blackboard.h"
#include "BTNode.h"

IMPLEMENT_REFLECTION(BTDecorator_Blackboard)

bool BTDecorator_Blackboard::Register_Properties()
{
    auto& info = GetStaticReflectionInfo();
    info.className = "BTDecorator_Blackboard";

    // 에디터 패널에 변수명 노출
    PROPERTY_STRING_JSON("Target Blackboard Key", "target_bb_key", _targetKey);
    PROPERTY_BOOL_JSON("Condition Expected Value", "expected_value", _expectedValue);

    return true;
}

BTDecorator_Blackboard::BTDecorator_Blackboard()
{
}

BTDecorator_Blackboard::BTDecorator_Blackboard(const BTDecorator_Blackboard& rhs)
    : BTDecorator(rhs)
    , _targetKey(rhs._targetKey)
    , _expectedValue(rhs._expectedValue)
{
}

void BTDecorator_Blackboard::Initialize()
{
    BTDecorator::Initialize();
}

EBTNodeResult BTDecorator_Blackboard::Update(float timeDelta)
{
    BTDecorator::Update(timeDelta);

    auto blackboard = _blackboard.lock();
    if (!blackboard)
    {
        _lastResult = EBTNodeResult::Failed;
        return _lastResult;
    }

    bool currentValue = blackboard->Get_ValueAsBool(_targetKey);

    // 종료조건이 맞지 않으면 Failed
    if (currentValue != _expectedValue)
    {
        _lastResult = EBTNodeResult::Failed;
        return _lastResult;
    }

    // 데코레이터에 자식이 있다면, 업데이트 처리 후 상태 전달
    if (_child)
    {
        _lastResult = _child->Update(timeDelta);

        return _lastResult;
    }

    _lastResult = EBTNodeResult::Succeeded;
    return _lastResult;
}

Shared<BTDecorator_Blackboard> BTDecorator_Blackboard::Create()
{
    return make_shared<BTDecorator_Blackboard>();
}

Shared<BTNode> BTDecorator_Blackboard::Clone()
{
    auto clone = make_shared<BTDecorator_Blackboard>(*this);

    clone->Set_DebugId(_debugId);
    if (_child)
    {
        clone->Set_Child(_child->Clone());
    }
    return clone;
}
