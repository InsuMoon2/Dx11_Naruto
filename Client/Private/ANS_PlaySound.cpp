#include "pch.h"
#include "ANS_PlaySound.h"
#include "AnimNotify_Factory.h"

REGISTER_ANIM_NOTIFY_STATE(ANS_PlaySound)
IMPLEMENT_REFLECTION(ANS_PlaySound)

bool ANS_PlaySound::Register_Properties()
{
    auto& info = GetStaticReflectionInfo();
    info.className = "ANS_PlaySound";
    info.properties.clear();

    // 재생할 사운드 파일 경로 (wstring 경로를 string으로 직렬화)
    PROPERTY_STRING_JSON("사운드 파일", "sound_file", _soundFile);

    // 볼륨 (0.0 ~ 1.0)
    PROPERTY_FLOAT_JSON("볼륨", "volume", _volume, 0.f, 1.f);

    // 피치 배율 (1.0 = 원본)
    PROPERTY_FLOAT_JSON("피치", "pitch", _pitch, 0.1f, 4.f);

    // 사운드 채널 선택
    PROPERTY_ENUM_JSON("채널", "channel", _channel, ESoundChannel);

    // 구간 종료 시 사운드 정지 여부 (루프 사운드 전용)
    PROPERTY_BOOL_JSON("종료 시 정지", "stop_on_end", _stopOnEnd);

    return true;
}

void ANS_PlaySound::On_Begin(const FAnimNotifyContext& context)
{
    // 프리뷰 모드 혹은 wrap(루프 재생에서 중간 진입) 시에는 재생하지 않는다.
    if (context.isPreview || context.wrapped)
        return;

    if (_soundFile.empty())
        return;

    const wstring wSoundFile = Utils::ToWString(_soundFile);

    // 피치가 기본값(1.0f)이면 일반 재생, 그 외에는 피치 적용 재생을 사용한다.
    if (fabsf(_pitch - 1.f) < FLT_EPSILON)
        GAME->Play_Sound(wSoundFile, _channel, _volume);
    else
        GAME->Play_Sound_Pitched(wSoundFile, _channel, _volume, _pitch);
}

void ANS_PlaySound::On_Tick(const FAnimNotifyContext& context)
{
    // ANS_PlaySound는 매 틱에 추가 처리 없이 넘어간다.
}

void ANS_PlaySound::On_End(const FAnimNotifyContext& context)
{
    // stopOnEnd 가 false 이면 사운드를 강제 정지하지 않는다.
    if (!_stopOnEnd)
        return;

    if (_soundFile.empty())
        return;

    const wstring wSoundFile = Utils::ToWString(_soundFile);
    GAME->Stop_Sound(wSoundFile);
}
