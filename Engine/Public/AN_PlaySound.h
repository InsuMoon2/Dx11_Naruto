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
    // 등록된 사운드. Sound 클래스를 따로 만들어줄까.
    // 경로말고 다른거로 

    ESoundChannel _channel = ESoundChannel::END;

    float _volume = 1.f;

};

NS_END
