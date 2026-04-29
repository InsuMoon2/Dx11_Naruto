#pragma once
#include "AnimNotifyState.h"

NS_BEGIN(Client)

class ANS_CollisionEnable : public AnimNotifyState
{
    GENERATED_BODY(ANS_CollisionEnable)

public:
    string Get_TypeName() const override;

    void On_Begin(const FAnimNotifyContext& context) override;
    void On_Tick(const FAnimNotifyContext& context) override;
    void On_End(const FAnimNotifyContext& context) override;

private:
    string _target = "hitbox";

    bool _useRightHand = true;
    bool _useLeftHand = false;
    bool _useRightFoot = false;
    bool _useLeftFoot = false;

    bool _useHitReactionOverride = false;

    EHitReactionType _overrideHitReactionType = EHitReactionType::Default;

    bool    _useLaunchOverride = false;
    float   _overrideLaunchPower = 0.f;
    float   _overrideLaunchUp = 0.f;

    bool    _useHitSoundOverride = false; // 현재 타격 구간에서 콤보 프로파일 대신 notify가 직접 피격음을 지정할지 여부다.
    int32   _overrideHitSound = 0; // useHitSoundOverride가 켜졌을 때 사용할 정수 기반 피격 사운드 ID다.
    string  _overrideHitSoundFile = ""; // useHitSoundOverride가 켜졌을 때 사용할 직접 피격 사운드 파일명이다.
};

NS_END
