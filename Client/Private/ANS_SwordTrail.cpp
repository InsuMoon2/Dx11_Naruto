#include "pch.h"
#include "ANS_SwordTrail.h"
#include "AnimNotify_Factory.h"
#include "SwordTrail_Component.h"
#include "GameObject.h"

NS_BEGIN(Client)

REGISTER_ANIM_NOTIFY_STATE(ANS_SwordTrail)
IMPLEMENT_REFLECTION(ANS_SwordTrail)

bool ANS_SwordTrail::Register_Properties()
{
    auto& info = GetStaticReflectionInfo();
    info.className = "ANS_SwordTrail";

    PROPERTY_STRING_JSON("위쪽 뼈 이름", "top_bone", _topBoneName);
    PROPERTY_STRING_JSON("아랫쪽 뼈 이름(선택)", "bot_bone", _bottomBoneName);
    PROPERTY_FLOAT_JSON("라이프 타임", "lifespan", _lifespan, 0.05f, 3.0f);
    PROPERTY_FLOAT_JSON("굵기", "width", _width, 0.1f, 10.0f);
    PROPERTY_INT_JSON("텍스처 인덱스", "texture_index", _textureIndex, 0, 16);

    return true;
}

void ANS_SwordTrail::On_Begin(const FAnimNotifyContext& context)
{
    if (context.isPreview || !context.owner)
        return;

    if (auto trailCom = context.owner->Get_Component<SwordTrail_Component>())
        trailCom->Start_SwordTrail(_topBoneName, _bottomBoneName, _lifespan, _width, _textureIndex);
}

void ANS_SwordTrail::On_End(const FAnimNotifyContext& context)
{
    if (context.isPreview || !context.owner)
        return;

    if (auto trailCom = context.owner->Get_Component<SwordTrail_Component>())
        trailCom->Stop_SwordTrail();
}

NS_END
