#include "pch.h"
#include "AnimNotifyState.h"

json AnimNotifyState::Serialize_Payload() const
{
    return json::object();
}

void AnimNotifyState::Deserialize_Payload(const json& payload)
{
}
