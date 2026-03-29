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
