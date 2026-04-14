#pragma once

#include "AnimNotifyState.h"

NS_BEGIN(Client)

class ANS_SwordTrail : public AnimNotifyState
{
    GENERATED_BODY(ANS_SwordTrail)

public:
    string Get_TypeName() const override { return "ANS_SwordTrail"; }

public:
    void On_Begin(const FAnimNotifyContext& context) override;
    void On_Tick(const FAnimNotifyContext& context) override {}
    void On_End(const FAnimNotifyContext& context) override;

private:
    string _topBoneName = "";   
    string _bottomBoneName = "";
    float  _lifespan = 0.18f;   
    float  _width = 2.5f;

    int32 _textureIndex = 0;
};

NS_END
