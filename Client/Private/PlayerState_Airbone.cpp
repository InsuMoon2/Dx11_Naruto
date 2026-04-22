#include "pch.h"
#include "PlayerState_Airbone.h"

#include "AnimationStateComponent.h"
#include "PlayerStateMachine.h"
#include "InputComponent.h"
#include "MovementComponent.h"
#include "GameObject.h"
#include "MyPlayer.h"
#include "Weapon.h"

PlayerState_Airbone::PlayerState_Airbone()
{
}

void PlayerState_Airbone::Enter(PlayerStateMachine* state)
{
    if (!state)
        return;

    auto input = state->Get_Input();
    auto movement = state->Get_Movement();
    auto owner = state->Get_Owner();

    if (!input || !movement || !owner)
        return;

    _startedInAir = !movement->Is_OnGround();

    input->Set_InputMode(EPlayerInputMode::Normal);
    movement->Set_OrientRotationToMovement(false);

    Vec3 velocity = movement->Get_Velocity();
    velocity.x = 0.f;
    velocity.z = 0.f;
    movement->Set_Velocity(velocity);

    state->Play_AnimState(EPlayerState::Attack_Airbone);
}

void PlayerState_Airbone::Update(PlayerStateMachine* state, float timeDelta)
{
    auto input = state->Get_Input();
    auto movement = state->Get_Movement();
    if (!input || !movement)
        return;

    if (!state->Is_AnimStateFinished())
        return;

    if (_startedInAir || !movement->Is_OnGround())
    {
        state->Change_State(EPlayerState::JumpFall);
        return;
    }

    state->Change_State(input->Has_MoveInput() ? EPlayerState::Run : EPlayerState::Idle);
}

void PlayerState_Airbone::Exit(PlayerStateMachine* state)
{
    if (!state)
        return;

    auto movement = state->Get_Movement();
    if (movement)
        movement->Set_OrientRotationToMovement(true);

    auto input = state->Get_Input();
    if (input)
        input->Set_InputMode(EPlayerInputMode::Normal);

    _startedInAir = false;

    auto owner = state->Get_Owner();
    if (!owner)
        return;

    auto container = dynamic_pointer_cast<ContainerObject>(owner);
    if (container)
    {
        auto weapon = dynamic_pointer_cast<Weapon>(
            container->Get_PartObject(ContainerObject::EPartSlot::Weapon));
        if (weapon)
            weapon->Set_ColliderActive(false);
    }

    auto myPlayer = dynamic_pointer_cast<MyPlayer>(owner);
    if (myPlayer)
    {
        myPlayer->Disable_Hitbox(EHitboxTarget::RightHand);
        myPlayer->Disable_Hitbox(EHitboxTarget::LeftHand);
        myPlayer->Disable_Hitbox(EHitboxTarget::RightFoot);
        myPlayer->Disable_Hitbox(EHitboxTarget::LeftFoot);
    }
}

Shared<PlayerState_Airbone> PlayerState_Airbone::Create()
{
    return make_shared<PlayerState_Airbone>();
}
