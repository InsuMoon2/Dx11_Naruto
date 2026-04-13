#include "pch.h"
#include "ANS_Trail.h"
#include "AnimNotify_Factory.h"
#include "Trail_Component.h"
#include "GameObject.h"

NS_BEGIN(Client)

REGISTER_ANIM_NOTIFY_STATE(ANS_Trail)
IMPLEMENT_REFLECTION(ANS_Trail)

bool ANS_Trail::Register_Properties()
{
    auto& info = GetStaticReflectionInfo();
    info.className = "ANS_Trail";

    PROPERTY_STRING_JSON("위쪽 뼈 이름", "top_bone", _topBoneName);
    PROPERTY_STRING_JSON("아랫쪽 뼈 이름(선택)", "bot_bone", _bottomBoneName);
    PROPERTY_FLOAT_JSON("라이프 타임", "lifespan", _lifespan, 0.1f, 5.0f);

    PROPERTY_FLOAT_JSON("굵기 (뼈 1개일때)", "width", _width, 0.1f, 10.0f);

    return true;
}

void ANS_Trail::On_Begin(const FAnimNotifyContext& context)
{
    if (context.isPreview || !context.owner) return;

    if (auto trailCom = context.owner->Get_Component<Trail_Component>())
    {
        trailCom->Start_Trail(_topBoneName, _bottomBoneName, _lifespan, _width);
    }
}

void ANS_Trail::On_End(const FAnimNotifyContext& context)
{
    if (context.isPreview || !context.owner) return;

    if (auto trailCom = context.owner->Get_Component<Trail_Component>())
    {
        trailCom->Stop_Trail();
    }
}

NS_END
