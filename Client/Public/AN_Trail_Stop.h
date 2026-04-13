#pragma once
#include "AnimNotify.h"

NS_BEGIN(Client)

class AN_Trail_Stop : public AnimNotify
{
    GENERATED_BODY(AN_Trail_Stop)

public:
    string Get_TypeName() const override { return "AN_Trail_Stop"; }

public:
    virtual void Execute(const FAnimNotifyContext& context) override;
};

NS_END
