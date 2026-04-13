#include "pch.h"
#include "AN_LightningTrail_Start.h"
#include "AnimNotify_Factory.h"
#include "GameObject.h"
#include "LightningTrail_Component.h"

NS_BEGIN(Client)

REGISTER_ANIM_NOTIFY(AN_LightningTrail_Start)
IMPLEMENT_REFLECTION(AN_LightningTrail_Start)

bool AN_LightningTrail_Start::Register_Properties()
{
    auto& info = GetStaticReflectionInfo();
    info.className = "AN_LightningTrail_Start";

    PROPERTY_STRING_JSON("장착 뼈 이름", "bone_name", _boneName);
    PROPERTY_FLOAT_JSON("잔류 시간", "lifespan", _lifespan, 0.03f, 2.0f);
    PROPERTY_FLOAT_JSON("번개 두께", "width", _width, 0.01f, 2.0f);
    PROPERTY_INT_JSON("번개 줄 개수", "line_count", _lineCount, 1, 10);

    return true;
}

void AN_LightningTrail_Start::Execute(const FAnimNotifyContext& context)
{
    if (context.isPreview || !context.owner)
        return;

    auto trailCom = context.owner->Get_Component<LightningTrail_Component>();
    if (!trailCom)
        return;

    const uint32 safeLineCount = static_cast<uint32>(max(1, _lineCount));

    // 치도리 돌진 구간에서 왼손 본 기준 다중 번개 트레일 방출을 시작
    trailCom->Start_LightningTrail(_boneName, _lifespan, _width, safeLineCount);
}

NS_END
