#pragma once

#include "AnimNotify.h"

NS_BEGIN(Engine)

class ENGINE_DLL AN_PlaySound : public AnimNotify
{
    GENERATED_BODY(AN_PlaySound)

public:
    string Get_TypeName() const override;
    void   Execute(const FAnimNotifyContext& context) override;

private:
    string _soundFile;
    ESoundChannel _channel = ESoundChannel::Effect;

    float _volume = 1.f;

};

NS_END
