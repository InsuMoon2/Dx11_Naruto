#include "pch.h"
#include "PlayerState_Attack.h"
#include "PlayerStateMachine.h"
#include "InputComponent.h"
#include "MovementComponent.h"
#include "transform.h"
#include "GameObject.h"
#include "AnimationStateComponent.h"

PlayerState_Attack::PlayerState_Attack()
{
    _comboAnimStates =
    {
        EPlayerState::Attack_1,
        EPlayerState::Attack_2,
        EPlayerState::Attack_3,
        EPlayerState::Attack_4
    };
}

void PlayerState_Attack::Enter(PlayerStateMachine* state)
{
    if (!state) return;

    _cachedStateMachine = state;

    auto input = state->Get_Input();
    auto movement = state->Get_Movement();
    auto owner = state->Get_Owner();

    if (!input || !movement || !owner)
        return;

    input->Set_InputMode(EPlayerInputMode::Normal);  
    movement->Set_OrientRotationToMovement(false);

    int32 safeIndex = min(_comboIndex, static_cast<int32>(_comboAnimStates.size()) - 1);
    state->Play_AnimState(_comboAnimStates[safeIndex]);

    // 노티파이에서 세팅
    _comboWindowOpen = false;
    _hasBufferedAttack = false;
}

void PlayerState_Attack::Update(PlayerStateMachine* state, float timeDelta)
{
    EAnimPhase phase = state->Get_AnimPhase();

    auto input = state->Get_Input();
    CHECK_NULL(input);

    if (input->Get_Frame().attackDown)
    {
        if (_comboWindowOpen && _comboIndex < MAX_COMBO - 1)
        {
            Advance_Combo();

            return; 
        }
    }

    if (state->Is_AnimStateFinished())
    {
        Reset_Combo();
        state->Change_State(EPlayerState::Idle);
    }

}

void PlayerState_Attack::Exit(PlayerStateMachine* state)
{
    if (!state) return;

    auto movement = state->Get_Movement();
    if (movement)
    {
        movement->Set_OrientRotationToMovement(true);
    }

    auto input = state->Get_Input();
    input->Set_InputMode(EPlayerInputMode::Normal);

    _cachedStateMachine = nullptr;
}

void PlayerState_Attack::Open_ComboWindow()
{
    _comboWindowOpen = true;

    if (_hasBufferedAttack && _comboIndex < MAX_COMBO - 1)
    {
        Advance_Combo();
    }
}

void PlayerState_Attack::Close_ComboWindow()
{
    if (!_comboWindowOpen)
        return;

    _comboWindowOpen = false;

    Reset_Combo();
}

void PlayerState_Attack::Buffer_AttackInput()
{
    //if (_comboWindowOpen)
        _hasBufferedAttack = true;
}

void PlayerState_Attack::Reset_Combo()
{
    _comboIndex = 0;
    _comboWindowOpen = false;
    _hasBufferedAttack = false;
}

void PlayerState_Attack::Advance_Combo()
{
    _comboIndex++;
    _comboWindowOpen = false;
    _hasBufferedAttack = false;

    if (_cachedStateMachine)
    {
        int32 safeIndex = min(_comboIndex, static_cast<int32>(_comboAnimStates.size()) - 1);
        _cachedStateMachine->Play_AnimState(_comboAnimStates[safeIndex]);
    }
}

Shared<PlayerState_Attack> PlayerState_Attack::Create()
{
    auto state = make_shared<PlayerState_Attack>();

    return state;
}
