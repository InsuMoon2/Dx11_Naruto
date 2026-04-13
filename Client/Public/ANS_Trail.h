#pragma once

#include "AnimNotifyState.h"

NS_BEGIN(Client)

class ANS_Trail : public AnimNotifyState
{
    GENERATED_BODY(ANS_Trail)

public:
    string Get_TypeName() const override { return "ANS_Trail"; }

public:
    virtual void On_Begin(const FAnimNotifyContext& context) override;
    virtual void On_Tick(const FAnimNotifyContext& context) override {}
    virtual void On_End(const FAnimNotifyContext& context) override;

private:
    string _topBoneName = "";
    string _bottomBoneName = "";
    float  _lifespan = 0.5f;
    float  _width = 3.f; // 뼈가 하나일때 굵기

};

NS_END
