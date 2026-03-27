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

    input->Set_InputMode(EPlayerInputMode::LookOnly);
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
    if (input && input->Get_Frame().attackDown)
    {
        Buffer_AttackInput();
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
    _hasBufferedAttack = false;
}

void PlayerState_Attack::Close_ComboWindow()
{
    _comboWindowOpen = false;

    if (_hasBufferedAttack && _comboIndex < MAX_COMBO - 1)
    {
        // 콤보가 끝나기 전이라면 다음 콤보로 이동
        _comboIndex++;
        _hasBufferedAttack = false;

        if (_cachedStateMachine)
        {
            _cachedStateMachine->Force_Enter_State(EPlayerState::Attack_1);
        }
    }
    else
    {
        // 콤보 종료, Idle로 다시 세팅
        Reset_Combo();

        if (_cachedStateMachine)
        {
            _cachedStateMachine->Change_State(EPlayerState::Idle);
        }
    }
}

void PlayerState_Attack::Buffer_AttackInput()
{
    if (_comboWindowOpen)
        _hasBufferedAttack = true;
}

void PlayerState_Attack::Reset_Combo()
{
    _comboIndex = 0;
    _comboWindowOpen = false;
    _hasBufferedAttack = false;
}

Shared<PlayerState_Attack> PlayerState_Attack::Create()
{
    auto state = make_shared<PlayerState_Attack>();

    return state;
}
