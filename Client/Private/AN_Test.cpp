#include "pch.h"
#include "AN_Test.h"
#include "AnimNotify_Factory.h"

REGISTER_ANIM_NOTIFY(AN_Test)

string AN_Test::Get_TypeName() const
{
    return "AN_Test";
}

void AN_Test::Execute(const FAnimNotifyContext& context)
{
    if (context.isPreview) return;

    if (!context.owner || !context.model)
        return;

    LOG_INFO("AN_Test : Execute Test");
}

json AN_Test::Serialize_Payload() const
{
    return AnimNotify::Serialize_Payload();
}

void AN_Test::Deserialize_Payload(const json& payload)
{
    AnimNotify::Deserialize_Payload(payload);
}

void AN_Test::Free()
{
    AnimNotify::Free();
}
