#pragma once

#include "Component_Inspector.h"
#include "Client_Enum.h"

NS_BEGIN(Engine)
NS_END

NS_BEGIN(Client)
class PlayerStateMachine;
struct FStateAnimationDesc;
NS_END

NS_BEGIN(Editor)

class PlayerStateMachine_Inspector : public Component_Inspector
{
public:
    explicit PlayerStateMachine_Inspector() = default;
    virtual ~PlayerStateMachine_Inspector() = default;

public:
    void    Draw_Inspector(shared_ptr<Component> component) override;
    uint32  Get_ComponentType() const override { return Protocol::COMPONENT_TYPE_PLAYER_STATE; }

private:
    static const char* Get_StateLabel(EPlayerState state);

private:
    EPlayerState _forceState = EPlayerState::Idle;

};

NS_END
