#include "pch.h"
#include "BTTask_Wait.h"

BTTask_Wait::BTTask_Wait(float waitTime)
    : _waitTime(waitTime), _accTime(0.f)
{
}

BTTask_Wait::BTTask_Wait(const BTTask_Wait& rhs)
    : BTTask(rhs)
    , _waitTime(rhs._waitTime)
{

}

void BTTask_Wait::Initialize()
{
    _accTime = 0.0f;
}

EBTNodeResult BTTask_Wait::Update(float timeDelta)
{
    _accTime += timeDelta;

    if (_accTime >= _waitTime)
    {
        _lastResult = EBTNodeResult::Succeeded;
        return EBTNodeResult::Succeeded;
    }

    _lastResult = EBTNodeResult::InProgress;
    return EBTNodeResult::InProgress;

}

Shared<BTNode> BTTask_Wait::Clone()
{
    return make_shared<BTTask_Wait>(*this);
}
