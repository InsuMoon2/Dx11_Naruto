#pragma once

#include "AnimNotifyState.h"

NS_BEGIN(Client)

class ANS_SmearEffect final : public AnimNotifyState
{
    GENERATED_BODY(ANS_SmearEffect)

public:
    string Get_TypeName() const override { return "ANS_SmearEffect"; }

    void On_Begin(const FAnimNotifyContext& context) override;
    void On_Tick(const FAnimNotifyContext& context) override;
    void On_End(const FAnimNotifyContext& context) override;

private:
    float _captureInterval = 0.02f;

    float _lifespan = 0.14f;
    float _smearLength = 1.4f; // 이동 방향 뒤로 끌리는 최대 길이

    Vec4 _baseColor = Vec4(0.05f, 0.08f, 0.25f, 1.f);
    // 스미어 강조 색상
    Vec4 _edgeColor = Vec4(0.45f, 0.65f, 1.f, 1.f);


};

NS_END
