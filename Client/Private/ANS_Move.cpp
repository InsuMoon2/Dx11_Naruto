#include "pch.h"
#include "ANS_Move.h"
#include "GameObject_Factory.h"
#include "AnimNotify_Factory.h"

REGISTER_ANIM_NOTIFY_STATE(ANS_Move)
IMPLEMENT_REFLECTION(ANS_Move)

bool ANS_Move::Register_Properties()
{
    auto& info = GetStaticReflectionInfo();
    info.className = "ANS_Move";
    info.properties.clear();

    PROPERTY_FLOAT("Speed", _speed, 0.f, 50.f);
    PROPERTY_BOOL("Use Forward Dir", _useForwardDir);
    PROPERTY_BOOL("Use Target Dir", _useTargetDir);

    return true;
}

string ANS_Move::Get_TypeName() const
{
    return "ANS_Move";
}

void ANS_Move::On_Begin(const FAnimNotifyContext& context)
{
}

void ANS_Move::On_Tick(const FAnimNotifyContext& context)
{
}

void ANS_Move::On_End(const FAnimNotifyContext& context)
{
}

json ANS_Move::Serialize_Payload() const
{
    return AnimNotifyState::Serialize_Payload();
}

void ANS_Move::Deserialize_Payload(const json& payload)
{
    AnimNotifyState::Deserialize_Payload(payload);
}
