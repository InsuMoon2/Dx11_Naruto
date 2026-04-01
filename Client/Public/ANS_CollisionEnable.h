#pragma once
#include "AnimNotifyState.h"

NS_BEGIN(Client)

class ANS_CollisionEnable : public AnimNotifyState
{
    GENERATED_BODY(ANS_CollisionEnable)

public:
    string Get_TypeName() const override;

    void On_Begin(const FAnimNotifyContext& context) override;
    void On_Tick(const FAnimNotifyContext& context)  override;
    void On_End(const FAnimNotifyContext& context)   override;

public:
    json Serialize_Payload() const override;
    void Deserialize_Payload(const json& payload) override;

private:
    string _target = "hitbox";

    bool _useRightHand = true;
    bool _useLeftHand = false;
    bool _useRightFoot = false;
    bool _useLeftFoot = false;
};

NS_END
