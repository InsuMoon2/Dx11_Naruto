#pragma once

#include "AnimNotifyState.h"

NS_BEGIN(Client)

class ANS_ComboWindow : public AnimNotifyState
{
public:
    string Get_TypeName() const override;

    void On_Begin(const FAnimNotifyContext& context) override;
    void On_Tick(const FAnimNotifyContext& context) override;
    void On_End(const FAnimNotifyContext& context) override;
};

NS_END
