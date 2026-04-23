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
};

NS_END
