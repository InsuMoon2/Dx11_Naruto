#pragma once

#include "AnimNotify.h"

NS_BEGIN(Client)

class AN_LightningTrail_Start : public AnimNotify
{
    GENERATED_BODY(AN_LightningTrail_Start)

public:
    string Get_TypeName() const override { return "AN_LightningTrail_Start"; }

public:
    void Execute(const FAnimNotifyContext& context) override;

private:
    string _boneName = "L_Hand_Weapon_cnt_tr"; 
    float _lifespan = 0.18f;                   
    float _width = 0.16f;                      
    int32 _lineCount = 5;                      
};

NS_END
