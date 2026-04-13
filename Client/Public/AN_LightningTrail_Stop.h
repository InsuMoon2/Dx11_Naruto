#pragma once

#include "AnimNotify.h"

NS_BEGIN(Client)

class AN_LightningTrail_Stop : public AnimNotify
{
    GENERATED_BODY(AN_LightningTrail_Stop)

public:
    string Get_TypeName() const override { return "AN_LightningTrail_Stop"; }

public:
    void Execute(const FAnimNotifyContext& context) override;
};

NS_END
