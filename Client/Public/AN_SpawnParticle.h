#pragma once

#include "AnimNotify.h" 

NS_BEGIN(Client)

class AN_SpawnParticle : public AnimNotify
{
    GENERATED_BODY(AN_SpawnParticle)

public:
    string Get_TypeName() const override { return "AN_SpawnParticle"; }

    void Execute(const FAnimNotifyContext& context) override;

private:
    string _effectAssetName = "Rasengan_WindE";
    string _boneName = "R_Hand_Weapon_cnt_tr";

    Vec3 _localOffset = Vec3::Zero;
    Vec3 _localRotation = Vec3::Zero;
    Vec3 _localScale = Vec3(1.f, 1.f, 1.f);

    bool _loopOverride = false;

    bool _attachToBone = false;
    bool _useInitialBoneTransform = false;

};

NS_END
