#pragma once

#include "AnimNotify.h"

NS_BEGIN(Client)

class AN_Test final : public AnimNotify
{
public:
    string Get_TypeName() const override;
    void Execute(const FAnimNotifyContext& context) override;

};

NS_END
