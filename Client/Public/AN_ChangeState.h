#pragma once

#include "AnimNotify.h"

class AN_ChangeState : public AnimNotify
{
    GENERATED_BODY(AN_ChangeState)

public:
    string Get_TypeName() const override;
    void Execute(const FAnimNotifyContext& context) override;

private:
    EPlayerState _nextState = EPlayerState::Idle;

};

