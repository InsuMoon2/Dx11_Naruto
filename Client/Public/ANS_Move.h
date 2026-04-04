#pragma once

#include "AnimNotifyState.h"

NS_BEGIN(Client)

class ANS_Move : public AnimNotifyState
{
    GENERATED_BODY(ANS_Move)

public:
    string Get_TypeName() const override;
    
    void On_Begin(const FAnimNotifyContext& context) override;
    void On_Tick(const FAnimNotifyContext& context) override;
    void On_End(const FAnimNotifyContext& context) override;

    json Serialize_Payload() const override;
    void Deserialize_Payload(const json& payload) override;

private:
    float   _speed = 5.f;

    // 캐릭터 정면방향으로
    bool    _useForwardDir = true;

    // 타겟팅 방향으로
    bool    _useTargetDir = false;

    Vec3    _localDirection = Vec3::Zero;
};

NS_END
