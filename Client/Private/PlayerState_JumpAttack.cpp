#include "pch.h"
#include "PlayerState_JumpAttack.h"

#include "AnimationStateComponent.h"
#include "PlayerStateMachine.h"
#include "InputComponent.h"
#include "MovementComponent.h"
#include "transform.h"
#include "GameObject.h"
#include "EquipmentComponent.h"
#include "ComboProfile_Manager.h"
#include "MyPlayer.h"
#include "Weapon.h"

PlayerState_JumpAttack::PlayerState_JumpAttack()
{
    
}

void PlayerState_JumpAttack::Enter(PlayerStateMachine* state)
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
    movement->Set_GravityEnabled(false);

    Vec3 velocity = movement->Get_Velocity();
    velocity.y = 0.f;
    movement->Set_Velocity(velocity);

    _gravityRestored = false;

    // 무기타입 + 공중 여부로 프로파일 선택되게
    Select_Profile(state);

    // 노티파이에서 세팅
    _comboWindowOpen = false;
    _hasBufferedAttack = false;

    Play_CurrentComboClip(state);
}

void PlayerState_JumpAttack::Update(PlayerStateMachine* state, float timeDelta)
{
    auto input = state->Get_Input();
    auto movement = state->Get_Movement();
    CHECK_NULL(input);
    CHECK_NULL(movement);

    if (input->Get_Frame().attackDown)
    {
        if (_comboWindowOpen
                && _activeProfile
                && _comboIndex < _activeProfile->maxCombo - 1)
        {
            Advance_Combo();

            return; 
        }
    }

    // 착지 감지
    if (movement->Is_OnGround())
    {
        Reset_Combo();

        movement->Set_GravityEnabled(true);
        _gravityRestored = true;

        bool hasInput = input->Has_MoveInput();
        state->Change_State(hasInput ? EPlayerState::Run : EPlayerState::Idle);

        return;
    }

    if (state->Is_AnimStateFinished())
    {
        Reset_Combo();

        movement->Set_GravityEnabled(true);
        _gravityRestored = true;

        state->Change_State(EPlayerState::JumpFall);

        return;
    }

}

void PlayerState_JumpAttack::Exit(PlayerStateMachine* state)
{
    if (!state) return;

    auto movement = state->Get_Movement();
    if (movement)
    {
        movement->Set_OrientRotationToMovement(true);

        if (!_gravityRestored)
        {
            movement->Set_GravityEnabled(true);
            _gravityRestored = true;
        }
    }

    auto input = state->Get_Input();
    if (input)
        input->Set_InputMode(EPlayerInputMode::Normal);

    _cachedStateMachine = nullptr;

    auto owner = state->Get_Owner();
    if (owner)
    {
        auto container = dynamic_pointer_cast<ContainerObject>(owner);
        if (container)
        {
            auto weapon = dynamic_pointer_cast<Weapon>(container->Get_PartObject(ContainerObject::EPartSlot::Weapon));
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
}

void PlayerState_JumpAttack::Open_ComboWindow()
{
    _comboWindowOpen = true;

    if (_hasBufferedAttack &&
        _activeProfile &&
        _comboIndex < _activeProfile->maxCombo - 1)
    {
        Advance_Combo();
    }
}

void PlayerState_JumpAttack::Close_ComboWindow()
{
    if (!_comboWindowOpen)
        return;

    _comboWindowOpen = false;

    Reset_Combo();

    if (_cachedStateMachine)
    {
        auto movement = _cachedStateMachine->Get_Movement();
        if (movement)
        {
            movement->Set_GravityEnabled(true);
            _gravityRestored = true;
        }

        _cachedStateMachine->Change_State(EPlayerState::JumpFall);
    }
}

void PlayerState_JumpAttack::Buffer_AttackInput()
{
    //if (_comboWindowOpen)
        _hasBufferedAttack = true;
}

void PlayerState_JumpAttack::Reset_Combo()
{
    _comboIndex = 0;
    _comboWindowOpen = false;
    _hasBufferedAttack = false;
    _activeProfile = nullptr;
    _activeProfileType = EAttackProfileType::Hand_Aerial;
}

void PlayerState_JumpAttack::Advance_Combo()
{
    _comboIndex++;
    _comboWindowOpen = false;
    _hasBufferedAttack = false;

    if (_cachedStateMachine)
        Play_CurrentComboClip(_cachedStateMachine);
}

void PlayerState_JumpAttack::Select_Profile(PlayerStateMachine* state)
{
    // EquipmentComponent에서 무기타입 가져오기
    auto owner = state->Get_Owner();
    CHECK_NULL(owner);

    auto equipment = owner->Get_Component<EquipmentComponent>();

    // 프로파일 타입 결정
    if (equipment)
    {
        _activeProfileType = equipment->Find_AttackProfileType(true);
    }
    else
    {
        _activeProfileType = EAttackProfileType::Hand_Aerial;
    }

    // 프로파일 조회
    _activeProfile = GET_SINGLE(ComboProfile_Manager)->Find(_activeProfileType);

    if (!_activeProfile)
    {
        LOG_WARN("PlayerState_JumpAttack: 프로파일 없음 -> profileType={}",
            magic_enum::enum_name(_activeProfileType));
    }
}

void PlayerState_JumpAttack::Play_CurrentComboClip(PlayerStateMachine* state)
{
    if (!_activeProfile || _activeProfile->combos.empty())
    {
        state->Get_AnimationState()->Play_State("Attack_Air_01");
        return;
    }

    int32 safeIndex = min(_comboIndex,
        static_cast<int32>(_activeProfile->combos.size()) - 1);

    const string& animKey = _activeProfile->combos[safeIndex].animStateKey;

    state->Get_AnimationState()->Play_State(animKey);
}

Shared<PlayerState_JumpAttack> PlayerState_JumpAttack::Create()
{
    auto state = make_shared<PlayerState_JumpAttack>();

    return state;
}
