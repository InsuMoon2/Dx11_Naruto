#pragma once

#include "AnimNotifyState.h"

NS_BEGIN(Client)

class AttachedEffectObject;

class ANS_SpawnParticle : public AnimNotifyState
{
    GENERATED_BODY(ANS_SpawnParticle)

public:
    string Get_TypeName() const override { return "ANS_SpawnParticle"; }

    void On_Begin(const FAnimNotifyContext& context) override;
    void On_Tick(const FAnimNotifyContext& context)  override;
    void On_End(const FAnimNotifyContext& context)   override;

private:
    string _effectAssetName = "R_Hand_Weapon_cnt_tr";

    string _boneName = "";

    Vec3 _localOffset   = Vec3::Zero;
    Vec3 _localRotation = Vec3::Zero;
    Vec3 _localScale    = Vec3(1.f, 1.f, 1.f);

    bool _loopOverride = false;

    Weak<AttachedEffectObject> _spawnedEffect;
};

NS_END
