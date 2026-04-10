#pragma once

#include "AnimNotify.h"

NS_BEGIN(Client)

class AN_ClearMeleeSkill : public AnimNotify
{
    GENERATED_BODY(AN_ClearMeleeSkill)

public:
    string Get_TypeName() const override { return "AN_ClearMeleeSkill"; }
    void   Execute(const FAnimNotifyContext& context) override;
};

NS_END
