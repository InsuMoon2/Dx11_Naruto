#pragma once

#include "AnimNotifyState.h"

NS_BEGIN(Client)

// 애니메이션 구간 시작(On_Begin) 시 사운드를 1회 재생하는 ANS.
// stopOnEnd = true 로 설정하면 구간 종료(On_End) 시 동일 파일 사운드를 정지한다 (루프 사운드 용).
// pitch != 1.f 일 경우 Play_Sound_Pitched를 통해 피치가 적용된 상태로 재생된다.
class ANS_PlaySound : public AnimNotifyState
{
    GENERATED_BODY(ANS_PlaySound)

public:
    string Get_TypeName() const override { return "ANS_PlaySound"; }

    // 구간 진입 시 사운드를 1회 재생한다.
    void On_Begin(const FAnimNotifyContext& context) override;

    // ANS_PlaySound는 매 틱에 추가 처리를 하지 않는다.
    void On_Tick(const FAnimNotifyContext& context)  override;

    // stopOnEnd == true 이면 동일 사운드 파일을 정지 요청한다.
    void On_End(const FAnimNotifyContext& context)   override;

private:
    // 재생할 사운드 파일 경로 (예: L"Effects/Fireball_Hit.wav")
    // 에디터에서 직렬화되는 값이므로 wstring 대신 string으로 저장하고 내부에서 변환한다.
    string _soundFile = "";

    // 재생 볼륨 (0.0 ~ 1.0)
    float _volume = 1.f;

    // 피치 배율 (1.0 = 원본, 0.5 = 한 옥타브 아래, 2.0 = 한 옥타브 위)
    // 1.0f이면 일반 Play_Sound를 사용하고, 그 외에는 Play_Sound_Pitched를 사용한다.
    float _pitch = 1.f;

    // 사용할 사운드 채널 (Effect / Player / Monster 등)
    ESoundChannel _channel = ESoundChannel::Effect;

    // true이면 On_End 시 동일 사운드 파일 정지를 요청한다 (루프 사운드 전용)
    bool _stopOnEnd = false;
};

NS_END
