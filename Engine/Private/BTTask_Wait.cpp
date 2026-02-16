#include "pch.h"
#include "BTTask_Wait.h"

BTTask_Wait::BTTask_Wait(float waitTime)
    : _waitTime(waitTime), _elapsed(0.f)
{
}

BTTask_Wait::BTTask_Wait(const BTTask_Wait& rhs)
    : BTTask(rhs)
    , _waitTime(rhs._waitTime)
    , _elapsed(0.f)
{

}

void BTTask_Wait::Initialize()
{
    BTNode::Initialize();

    _elapsed = 0.0f;
}

EBTNodeResult BTTask_Wait::Update(float timeDelta)
{
    _elapsed += timeDelta;

    if (_elapsed >= _waitTime)
    {
        _lastResult = EBTNodeResult::Succeeded;
        _elapsed = 0.f;

        return EBTNodeResult::Succeeded;
    }

    _lastResult = EBTNodeResult::InProgress;
    return EBTNodeResult::InProgress;

}

Shared<BTNode> BTTask_Wait::Clone()
{
    auto clone = make_shared<BTTask_Wait>(*this);

    clone->Initialize();

    return clone;
}
