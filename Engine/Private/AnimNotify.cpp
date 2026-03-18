#include "pch.h"
#include "AnimNotify.h"

json AnimNotify::Serialize_Payload() const
{
    return json::object();
}

void AnimNotify::Deserialize_Payload(const json& payload)
{
}
