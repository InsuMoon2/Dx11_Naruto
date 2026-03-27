#include "pch.h"
#include "BTTask_SetAnimState.h"
#include "Blackboard.h"

BTTask_SetAnimState::BTTask_SetAnimState(const string& stateName)
    : _stateName(stateName)
{
}

BTTask_SetAnimState::BTTask_SetAnimState(const BTTask_SetAnimState& rhs)
    : BTTask(rhs)
    , _stateName(rhs._stateName)
{
}

void BTTask_SetAnimState::Initialize()
{
    BTTask::Initialize();
}

EBTNodeResult BTTask_SetAnimState::Update(float timeDelta)
{
    auto blackboard = _blackboard.lock();
    if (!blackboard)
    {
        _lastResult = EBTNodeResult::Failed;
        return EBTNodeResult::Failed;
    }

    blackboard->Set_ValueAsString("AnimState", _stateName);

    _lastResult = EBTNodeResult::Succeeded;

    return EBTNodeResult::Succeeded;
}

void BTTask_SetAnimState::OnDraw_Inspector()
{
    BTTask::OnDraw_Inspector();

    char buf[64] = {};
}

json BTTask_SetAnimState::Serialize_ToJson()
{
    return BTTask::Serialize_ToJson();
}

void BTTask_SetAnimState::Deserialize_FromJson(const json& data)
{
    BTTask::Deserialize_FromJson(data);
}

Shared<BTNode> BTTask_SetAnimState::Clone()
{
    return nullptr;
}
