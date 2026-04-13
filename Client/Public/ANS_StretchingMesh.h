#pragma once

#include "AnimNotifyState.h"

NS_BEGIN(Client)

class ANS_StretchingMesh : public AnimNotifyState
{
    GENERATED_BODY(ANS_StretchingMesh)

public:
    string Get_TypeName() const override { return "ANS_StretchingMesh"; }

    virtual void On_Begin(const FAnimNotifyContext& context) override;
    virtual void On_Tick(const FAnimNotifyContext& context)  override;
    virtual void On_End(const FAnimNotifyContext& context)   override;

private:
    string  _effectAssetName = "Chidori_Point"; 
    string  _boneName        = "L_Hand_Weapon_cnt_tr"; 
    
    float   _originalLength  = 5.0f; 
    Vec3    _thickness       = Vec3(0.5f, 0.5f, 1.0f); 

    Weak<GameObject> _spawnedEffect;
};

NS_END
