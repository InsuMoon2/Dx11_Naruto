#include "pch.h"
#include "PlayerState_Hit.h"
#include "PlayerStateMachine.h"
#include "MovementComponent.h"
#include "InputComponent.h"
#include "MyPlayer.h"
#include "PlayerState_Replacement.h"
#include "SkillComponent.h"
#include "Weapon.h"

void PlayerState_Hit::Enter(PlayerStateMachine* state)
{
    if (!state) return;

    auto input = state->Get_Input();
    if (input)
        input->Set_InputMode(EPlayerInputMode::BlockAll);

    auto movement = state->Get_Movement();
    if (movement)
        movement->Set_OrientRotationToMovement(false);

    auto owner = state->Get_Owner();
    if (owner)
    {
        auto myPlayer = dynamic_pointer_cast<MyPlayer>(owner);
        if (myPlayer)
            myPlayer->Disable_All_Hitboxes();
            
        auto container = dynamic_pointer_cast<ContainerObject>(owner);
        if (container)
        {
            auto weapon = dynamic_pointer_cast<Weapon>(container->Get_PartObject(ContainerObject::EPartSlot::Weapon));
            if (weapon)
                weapon->Set_ColliderActive(false);
        }
    }

    const auto& pendingHit = state->Get_PendingHitReaction();
    const EPlayerState hitState = Resolve_HitState(pendingHit.type);

    state->Play_AnimState(hitState);

    state->Consume_PendingHitReaction();
}

void PlayerState_Hit::Update(PlayerStateMachine* state, float timeDelta)
{
    if (!state)
        return;

    auto input = state->Get_Input();
    auto movement = state->Get_Movement();
    if (!input || !movement)
        return;

    const auto& frame = input->Get_Frame();

    if (frame.replacementDown)
{
    auto skillCom = state->Get_Owner()->Get_Component<SkillComponent>();
    auto replacementState = state->Get_State<PlayerState_Replacement>(EPlayerState::Replacement);

    if (skillCom && replacementState &&
        skillCom->Can_ActivateSubSkill(ESubSkillType::Replacement))
    {
        if (replacementState->Prepare_Replacement(state))
        {
            skillCom->Start_SubSkillCooldown(ESubSkillType::Replacement);
            state->Change_State(EPlayerState::Replacement);
            return;
        }
    }
}

    if (state->Is_AnimStateFinished())
    {
        state->Change_State(EPlayerState::Idle);
    }
}

void PlayerState_Hit::Exit(PlayerStateMachine* state)
{
    if (!state) return;

    auto input = state->Get_Input();
    if (input)
        input->Set_InputMode(EPlayerInputMode::Normal);

    auto movement = state->Get_Movement();
    if (movement)
        movement->Set_OrientRotationToMovement(true);
}

EPlayerState PlayerState_Hit::Resolve_HitState(EHitReactionType type)
{
    switch (type)
    {
    case EHitReactionType::Launch:
        return EPlayerState::Hit_Launch;

    case EHitReactionType::BlowOff:
        return EPlayerState::Hit_BlowOff;

    case EHitReactionType::Down:
        return EPlayerState::Hit_Down;

    case EHitReactionType::Stagger:
    case EHitReactionType::Default:
    default:
        return EPlayerState::Hit;
    }
}

Shared<PlayerState_Hit> PlayerState_Hit::Create()
{
    return make_shared<PlayerState_Hit>();
}
