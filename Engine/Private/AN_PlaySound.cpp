#include "pch.h"
#include "AN_PlaySound.h"
#include "AnimNotify_Factory.h"
#include "GameInstance.h"
#include "Utils.h"

REGISTER_ANIM_NOTIFY(AN_PlaySound)
IMPLEMENT_REFLECTION(AN_PlaySound)

bool AN_PlaySound::Register_Properties()
{
    auto& info = GetStaticReflectionInfo();
    info.className = "AN_PlaySound";
    info.properties.clear();

    PROPERTY_STRING_JSON("사운드", "sound_file", _soundFile);
    PROPERTY_ENUM_JSON("채널", "channel", _channel, ESoundChannel);
    PROPERTY_FLOAT_JSON("볼륨", "volume", _volume, 0.f, 1.f);

    return true;
}

string AN_PlaySound::Get_TypeName() const
{
    return "AN_PlaySound";
}

void AN_PlaySound::Execute(const FAnimNotifyContext& context)
{
    if (!context.owner || !context.model)
        return;

    if (_soundFile.empty())
    {
        LOG_WARN("사운드 파일이 없음. clip='{}'", context.clipName);
        return;
    }

    if (!GAME->Has_Sound(Utils::ToWString(_soundFile)))
    {
        LOG_WARN("사운드 파일 못찾음. sound='{}', clip='{}'",
            _soundFile, context.clipName);
        return;
    }

    if (!GAME->Play_Sound(Utils::ToWString(_soundFile), _channel, _volume))
    {
        LOG_WARN("사운드 재생 실패. sound='{}', channel={}, volume={}",
            _soundFile, static_cast<int32>(_channel), _volume);
    }
}
