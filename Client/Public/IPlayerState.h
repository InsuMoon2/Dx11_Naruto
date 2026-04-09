#pragma once

#include "Client_Enum.h"
#include "Delegate.h"

NS_BEGIN(Client)

class PlayerStateMachine;

class IPlayerState
{
public:
    virtual ~IPlayerState() = default;

public:
    virtual void Enter(PlayerStateMachine* state) = 0;
    virtual void Update(PlayerStateMachine* state, float timeDelta) = 0;
    virtual void Exit(PlayerStateMachine* state) = 0;

    virtual EPlayerState Get_StateID() const = 0;

    virtual bool Has_SuperArmor() const { return false; }
};

NS_END
