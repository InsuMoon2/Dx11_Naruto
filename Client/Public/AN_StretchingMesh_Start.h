#pragma once
#include "AnimNotify.h"

NS_BEGIN(Client)

class AN_StretchingMesh_Start : public AnimNotify
{
    GENERATED_BODY(AN_StretchingMesh_Start)

public:
    string Get_TypeName() const override { return "AN_StretchingMesh_Start"; }

public:
    virtual void Execute(const FAnimNotifyContext& context) override;

private:
    string _effectAssetName = "Chidori_Charge";
    string _boneName = "L_Hand_Weapon_cnt_tr";
    float _originalLength = 1.0f;
    Vec3 _thickness = Vec3(1.f, 1.f, 1.f);
};

NS_END
