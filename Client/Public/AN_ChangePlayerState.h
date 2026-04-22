#pragma once

#include "AnimNotify.h"

NS_BEGIN(Client)

class AN_ChangePlayerState : public AnimNotify
{
    GENERATED_BODY(AN_ChangePlayerState)

public:
    string Get_TypeName() const override { return "AN_ChangePlayerState"; }

public:
    void Execute(const FAnimNotifyContext& context) override;

private:
    EPlayerState _targetState = EPlayerState::Idle;
};

NS_END
