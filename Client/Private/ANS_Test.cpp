#include "pch.h"
#include "ANS_Test.h"
#include "AnimNotify_Factory.h"

REGISTER_ANIM_NOTIFY_STATE(ANS_Test)

string ANS_Test::Get_TypeName() const
{
    return "ANS_Test";
}

void ANS_Test::On_Begin(const FAnimNotifyContext& context)
{
    LOG_INFO("ANS Test : Begin !");
}

void ANS_Test::On_Tick(const FAnimNotifyContext& context)
{
    LOG_INFO("ANS Test : Tick !");
}

void ANS_Test::On_End(const FAnimNotifyContext& context)
{
    LOG_INFO("ANS Test : End !");
}

json ANS_Test::Serialize_Payload() const
{
    return AnimNotifyState::Serialize_Payload();
}

void ANS_Test::Deserialize_Payload(const json& payload)
{
    AnimNotifyState::Deserialize_Payload(payload);
}

void ANS_Test::Free()
{
    AnimNotifyState::Free();
}
