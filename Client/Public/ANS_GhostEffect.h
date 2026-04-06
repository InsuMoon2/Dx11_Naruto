#pragma once

#include "AnimNotifyState.h"

NS_BEGIN(Client)

class ANS_GhostEffect : public AnimNotifyState
{
    GENERATED_BODY(ANS_GhostEffect)

public:
    string Get_TypeName() const override { return "ANS_GhostEffect"; }

    void On_Begin(const FAnimNotifyContext& context) override;
    void On_Tick(const FAnimNotifyContext& context)  override;
    void On_End(const FAnimNotifyContext& context)   override;

public:
    json Serialize_Payload() const override;
    void Deserialize_Payload(const json& payload) override;

private:
    float _captureInterval = 0.05f;
    float _lifespan = 0.3f;
    Vec4  _ghostColor = Vec4(0.02f, 0.02f, 0.02f, 1.f);
    Vec4  _rimColor = Vec4(0.1f, 0.4f, 1.0f, 1.f);

};

NS_END
