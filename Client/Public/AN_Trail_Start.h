#pragma once

#include "AnimNotify.h"

NS_BEGIN(Client)

class AN_Trail_Start : public AnimNotify
{
    GENERATED_BODY(AN_Trail_Start)

public:
    string Get_TypeName() const override { return "AN_Trail_Start"; }

public:
    virtual void Execute(const FAnimNotifyContext& context) override;

private:
    string _topBoneName    = "";    
    string _bottomBoneName = "";    
    float  _lifespan       = 0.5f;  
    float  _width          = 3.f;   
};


NS_END
