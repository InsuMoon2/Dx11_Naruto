#include "pch.h"
#include "BTTask_Wait.h"

BTTask_Wait::BTTask_Wait(float waitTime)
    : _waitTime(waitTime), _accTime(0.f)
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
        return EBTNodeResult::Succeeded;
    }

    return EBTNodeResult::InProgress;

}
