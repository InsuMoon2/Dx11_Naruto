#pragma once

#include "AnimNotify.h"

NS_BEGIN(Client)

class AN_StopParticle : public AnimNotify
{
    GENERATED_BODY(AN_StopParticle)

public:
    string Get_TypeName() const override { return "AN_StopParticle"; }

public:
    void Execute(const FAnimNotifyContext& context) override;

private:
    string _effectAssetName = "";
    bool _stopAllFromOwner = false;
};

NS_END
