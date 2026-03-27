#include "pch.h"
#include "BTTask_SetAnimState.h"
#include "Blackboard.h"

IMPLEMENT_REFLECTION(BTTask_SetAnimState)

bool BTTask_SetAnimState::Register_Properties()
{
    auto& info = GetStaticReflectionInfo();
    info.className = "BTTask_SetAnimState";

    PROPERTY_STRING("State Name", _stateName);

    return true;
}

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

json BTTask_SetAnimState::Serialize_ToJson()
{
    json j = BTTask::Serialize_ToJson();

    j["state_name"] = _stateName;

    return j;
}

void BTTask_SetAnimState::Deserialize_FromJson(const json& data)
{
    _stateName = data.value("state_name", "Idle"); // 디폴트 Idle로
}

Shared<BTTask_SetAnimState> BTTask_SetAnimState::Create()
{
    return make_shared<BTTask_SetAnimState>();
}

Shared<BTNode> BTTask_SetAnimState::Clone()
{
    auto clone = make_shared<BTTask_SetAnimState>(*this);
    clone->Initialize();
    return clone;
}
