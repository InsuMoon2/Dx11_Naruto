#pragma once

#include "Client_Enum.h"

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
};

NS_END
