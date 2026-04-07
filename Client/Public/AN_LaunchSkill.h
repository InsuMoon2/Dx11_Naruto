#pragma once

#include "AnimNotify.h"

NS_BEGIN(Client)

class AN_LaunchSkill final : public AnimNotify
{
    GENERATED_BODY(AN_LaunchSkill)

public:
    string Get_TypeName() const override { return "AN_LaunchSkill"; }
    
    void Execute(const FAnimNotifyContext& context) override;

private:
    Protocol::OBJECT_TYPE _launchObjectType = Protocol::OBJECT_TYPE_SKILL_RASENSHURIKEN;

    bool _aimAtTarget = false;
};

NS_END
