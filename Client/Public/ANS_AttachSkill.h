#pragma once

#include "AnimNotifyState.h"

NS_BEGIN(Client)

class SkillObject_Projectile;

class ANS_AttachSkill : public AnimNotifyState
{
    GENERATED_BODY(ANS_AttachSkill)

public:
    string Get_TypeName() const override;

    void On_Begin(const FAnimNotifyContext& context) override;
    void On_Tick(const FAnimNotifyContext& context)  override;
    void On_End(const FAnimNotifyContext& context)   override;

private:
    Weak<SkillObject_Projectile> _attachedSkill;

    Protocol::OBJECT_TYPE _spawnObjectType = Protocol::OBJECT_TYPE_SKILL_RASENSHURIKEN;

    string _boneName = "R_Hand_Weapon_cnt_tr"; // 에디터에서 세팅해야함

    Collision_Preset _collisionPreset = Collision_Preset::Projectile;
};

NS_END
