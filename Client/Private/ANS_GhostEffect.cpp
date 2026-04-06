#include "pch.h"
#include "ANS_GhostEffect.h"
#include "AnimNotify_Factory.h"
#include "GameObject.h"
#include "GhostEffect_Component.h"

REGISTER_ANIM_NOTIFY_STATE(ANS_GhostEffect);
IMPLEMENT_REFLECTION(ANS_GhostEffect);

bool ANS_GhostEffect::Register_Properties()
{
    auto& info = GetStaticReflectionInfo();
    info.className = "ANS_GhostEffect";
    info.properties.clear();

    PROPERTY_FLOAT("Capture Interval", _captureInterval, 0.05f, 1.f);
    PROPERTY_FLOAT("Ghost Lifespan", _lifespan, 0.f, 10.f);

    return true;
}

void ANS_GhostEffect::On_Begin(const FAnimNotifyContext& context)
{
    if (context.isPreview || !context.owner)
        return;

    auto ghostCom = context.owner->Get_Component<GhostEffect_Component>();
    if (ghostCom)
    {
        ghostCom->Start_GhostEffect(_captureInterval, _lifespan, _ghostColor, _rimColor);
    }

}

void ANS_GhostEffect::On_Tick(const FAnimNotifyContext& context)
{
}

void ANS_GhostEffect::On_End(const FAnimNotifyContext& context)
{
    if (context.isPreview || !context.owner) return;

    auto ghostCom = context.owner->Get_Component<GhostEffect_Component>();
    if (ghostCom)
    {
        ghostCom->Stop_GhostEffect();
    }
}

json ANS_GhostEffect::Serialize_Payload() const
{
    json j;

    j["interval"] = _captureInterval;
    j["lifespan"] = _lifespan;
    j["ghost_color"] = { _ghostColor.x, _ghostColor.y, _ghostColor.z, _ghostColor.w };
    j["rim_color"] = { _rimColor.x, _rimColor.y, _rimColor.z, _rimColor.w };

    return j;
}

void ANS_GhostEffect::Deserialize_Payload(const json& payload)
{
    if (payload.contains("interval")) _captureInterval = payload["interval"];
    if (payload.contains("lifespan")) _lifespan = payload["lifespan"];

    if (payload.contains("ghost_color"))
    {
        _ghostColor.x = payload["ghost_color"][0];
        _ghostColor.y = payload["ghost_color"][1];
        _ghostColor.z = payload["ghost_color"][2];
        _ghostColor.w = payload["ghost_color"][3];
    }
    if (payload.contains("rim_color"))
    {
        _rimColor.x = payload["rim_color"][0];
        _rimColor.y = payload["rim_color"][1];
        _rimColor.z = payload["rim_color"][2];
        _rimColor.w = payload["rim_color"][3];
    }

    AnimNotifyState::Deserialize_Payload(payload);
}
