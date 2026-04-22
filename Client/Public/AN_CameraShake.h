#pragma once

#include "AnimNotify.h"

NS_BEGIN(Client)

class AN_CameraShake : public AnimNotify
{
    GENERATED_BODY(AN_CameraShake)

public:
    string Get_TypeName() const override { return "AN_CameraShake"; }

    void Execute(const FAnimNotifyContext& context) override;

private:
    string _tag = "";                               
    float  _durationSec = 0.12f;                    
    float  _frequency = 30.f;                       
    float  _blendInSec = 0.01f;                     
    float  _blendOutSec = 0.07f;                    
    Vec3   _localPosAmplitude = Vec3(0.03f, 0.02f, 0.01f); 
    Vec3   _localRotAmplitudeDeg = Vec3(1.0f, 0.6f, 0.3f); 
};

NS_END
