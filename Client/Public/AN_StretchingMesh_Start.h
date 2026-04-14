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
    string _effectAssetName = "Lightning_Follow";
    string _boneName = "L_Hand_Weapon_cnt_tr";

    float _originalLength = 1.0f;

    Vec3 _thickness = Vec3(5.f, 5.f, 5.f);

    Vec3 _rotationOffset = Vec3(90.f, 0.f, 0.f);

    Vec3 _localOffsetStep = Vec3(0.08f, 0.f, 0.f);
    Vec3 _rotationOffsetStep = Vec3(0.f, 0.f, 8.f);
    Vec3 _thicknessStep = Vec3(0.15f, 0.f, 0.15f);
    int32 _spawnCount = 1;
};

NS_END
