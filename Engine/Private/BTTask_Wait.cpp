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

    //LOG_INFO("[BT] {} (id={}) 실행 중", _name, _debugId);

    _lastResult = EBTNodeResult::InProgress;

    return EBTNodeResult::InProgress;
}

void BTTask_Wait::OnDraw_Inspector()
{
    BTTask::OnDraw_Inspector();

    //ImGui::InputFloat("Wait Time##WaitTimeUnique", &_waitTime, 0.1f, 1.f, "%.2f");
}

json BTTask_Wait::Serialize_ToJson()
{
    json j = BTTask::Serialize_ToJson();

    j["wait_time"] = _waitTime;

    return j;
}

void BTTask_Wait::Deserialize_FromJson(const json& data)
{
    _waitTime = data.value("wait_time", 1.0f);
}

Shared<BTNode> BTTask_Wait::Clone()
{
    auto clone = make_shared<BTTask_Wait>(*this);

    clone->Initialize();

    return clone;
}
