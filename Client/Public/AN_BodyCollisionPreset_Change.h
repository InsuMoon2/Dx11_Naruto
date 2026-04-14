#pragma once

#include "AnimNotify.h"
#include "Collision_Define.h"

NS_BEGIN(Client)

class AN_BodyCollisionPreset_Change : public AnimNotify
{
    GENERATED_BODY(AN_BodyCollisionPreset_Change)

public:
    string Get_TypeName() const override { return "AN_BodyCollisionPreset_Change"; }

public:
    void Execute(const FAnimNotifyContext& context) override;

private:
    Collision_Preset _collisionPreset = Collision_Preset::Player;
};

NS_END
