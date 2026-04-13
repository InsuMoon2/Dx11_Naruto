#include "pch.h"
#include "AN_Trail_Start.h"
#include "Trail_Component.h"
#include "GameObject.h"
#include "AnimNotify_Factory.h"

NS_BEGIN(Client)

REGISTER_ANIM_NOTIFY(AN_Trail_Start)
IMPLEMENT_REFLECTION(AN_Trail_Start)

bool AN_Trail_Start::Register_Properties()
{
    auto& info = GetStaticReflectionInfo();
    info.className = "AN_Trail_Start";

    PROPERTY_STRING_JSON("위쪽 뼈 이름",          "top_bone",  _topBoneName);
    PROPERTY_STRING_JSON("아랫쪽 뼈 이름(옵션)",  "bot_bone",  _bottomBoneName);
    PROPERTY_FLOAT_JSON("궤적 잔류 시간",          "lifespan",  _lifespan, 0.1f, 5.0f);
    PROPERTY_FLOAT_JSON("굵기 (뼈 1개일때)",       "width",     _width,    0.1f, 10.0f);

    return true;
}

void AN_Trail_Start::Execute(const FAnimNotifyContext& context)
{
    if (context.isPreview || !context.owner) return;

    if (auto trailCom = context.owner->Get_Component<Trail_Component>())
    {
        trailCom->Start_Trail(_topBoneName, _bottomBoneName, _lifespan, _width);
    }
}

NS_END
