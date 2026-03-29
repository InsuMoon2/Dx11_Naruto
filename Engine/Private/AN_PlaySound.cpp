#include "pch.h"
#include "AN_PlaySound.h"
#include "AnimNotify_Factory.h"

REGISTER_ANIM_NOTIFY(AN_PlaySound)
IMPLEMENT_REFLECTION(AN_PlaySound)

bool AN_PlaySound::Register_Properties()
{
    auto& info = GetStaticReflectionInfo();
    info.className = "AN_PlaySound";
    info.properties.clear();

    // TODO 사운드 추가
    // PROPERTY_STRING ? 음.. 사운드 매니저에 등록된거?

    // ImGui로 검색까지 가능하게 만들어줘야할거같은데

    PROPERTY_ENUM("채널", _channel, ESoundChannel);
    PROPERTY_FLOAT("볼륨", _volume, 0.1f, 1.f); 

    return true;
}

string AN_PlaySound::Get_TypeName() const
{
    return "AN_PlaySound";
}

void AN_PlaySound::Execute(const FAnimNotifyContext& context)
{
    // 사운드는 프리뷰에서도 재생
    //if (context.isPreview) return;

    if (!context.owner || !context.model)
        return;

    LOG_INFO("AN_PlaySound 재생");
}
