#include "pch.h"
#include "PlayerState_Idle.h"
#include "PlayerStateMachine.h"
#include "InputComponent.h"
#include "MovementComponent.h"
#include "Model.h"
#include "GameObject.h"

PlayerState_Idle::PlayerState_Idle()
{
}

PlayerState_Idle::~PlayerState_Idle()
{
}

void PlayerState_Idle::Enter(PlayerStateMachine* state)
{
    auto owner = state->Get_Owner();

    auto modelCom = owner->Get_Component(Protocol::COMPONENT_TYPE_MODEL_SASKE);
    if (!modelCom) return;

    auto model = static_pointer_cast<Model>(modelCom);
    if (!model) return;

    const auto* animDesc = state->Find_StateAnimation(EPlayerState::Idle);
    if (!animDesc || animDesc->animationName.empty())
        return;

    model->Set_Animation(animDesc->animationName, animDesc->loop);

}

void PlayerState_Idle::Update(PlayerStateMachine* state, float timeDelta)
{
    auto input = state->Get_Input();
    auto movement = state->Get_Movement();
    const auto& frame = input->Get_Frame();

    if (frame.superJumpUp && frame.superJumpCharge > 0.f)
    {
        state->Change_State(EPlayerState::SuperJump);
        return;
    }

    if (frame.jumpDown)
    {
        state->Change_State(EPlayerState::Jump);
        return;
    }

    if (Vec2(frame.moveX, frame.moveY).LengthSquared() > FLT_EPSILON)
    {
        state->Change_State(EPlayerState::Run);
        return;
    }

    // 감속 처리용
    auto cmd = state->Init_MoveCommand();
    movement->Apply_Command(cmd);
    movement->Update(timeDelta);

}

void PlayerState_Idle::Exit(PlayerStateMachine* state)
{

}

Shared<PlayerState_Idle> PlayerState_Idle::Create()
{
    return make_shared<PlayerState_Idle>();
}
