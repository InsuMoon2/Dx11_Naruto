#include "pch.h"
#include "ANS_SmearEffect.h"

#include "AnimNotify_Factory.h"
#include "GameObject.h"
#include "SmearEffect_Component.h"

REGISTER_ANIM_NOTIFY_STATE(ANS_SmearEffect);
IMPLEMENT_REFLECTION(ANS_SmearEffect);

bool ANS_SmearEffect::Register_Properties()
{
    auto& info = GetStaticReflectionInfo();
    info.className = "ANS_SmearEffect";
    info.properties.clear();

    PROPERTY_FLOAT_JSON("Capture Interval", "interval", _captureInterval, 0.005f, 0.2f);
    PROPERTY_FLOAT_JSON("Smear Lifespan", "lifespan", _lifespan, 0.01f, 1.0f);
    PROPERTY_FLOAT_JSON("Smear Length", "smear_length", _smearLength, 0.05f, 10.0f);
    PROPERTY_COLOR_JSON("Base Color", "base_color", _baseColor);
    PROPERTY_COLOR_JSON("Edge Color", "edge_color", _edgeColor);

    return true;
}

void ANS_SmearEffect::On_Begin(const FAnimNotifyContext& context)
{
    if (context.isPreview || !context.owner)
        return;

    auto smearCom = context.owner->Get_Component<SmearEffect_Component>();
    if (!smearCom)
        return;

    smearCom->Start_Smear(
        _captureInterval,
        _lifespan,
        _smearLength,
        _baseColor,
        _edgeColor);
}

void ANS_SmearEffect::On_Tick(const FAnimNotifyContext& context)
{
}

void ANS_SmearEffect::On_End(const FAnimNotifyContext& context)
{
    if (context.isPreview || !context.owner)
        return;

    auto smearCom = context.owner->Get_Component<SmearEffect_Component>();
    if (!smearCom)
        return;

    smearCom->Stop_Smear();
}
