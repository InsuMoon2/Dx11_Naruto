#pragma once

#include "AnimNotify.h"

NS_BEGIN(Client)

class AN_EquipMeleeSkill : public AnimNotify
{
    GENERATED_BODY(AN_EquipMeleeSkill)

public:
    string Get_TypeName() const override { return "AN_EquipMeleeSkill"; }
    void   Execute(const FAnimNotifyContext& context) override;

private:
    string _boneName = "R_Hand_Weapon_cnt_tr";
    Protocol::OBJECT_TYPE _spawnObjectType = Protocol::OBJECT_TYPE_SKILL_RASENGAN;
    Collision_Preset _collisionPreset = Collision_Preset::Player_Attack;

    Vec3 _attachOffset = Vec3::Zero;
};

NS_END
