#pragma once
#include "AnimNotify.h"

NS_BEGIN(Client)

class AN_Gravity : public AnimNotify
{
    GENERATED_BODY(AN_Gravity);

public:
    string Get_TypeName() const override { return "AN_Gravity"; }

    void Execute(const FAnimNotifyContext& context) override;

private:
    bool _checkGravity = false;
};

NS_END
