#include "pch.h"
#include "PlayerState_Attack.h"

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
#include "CombatStat.h"

PlayerState_Attack::PlayerState_Attack()
{
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

    Select_Profile(state);

    _comboWindowOpen = false;
    _hasBufferedAttack = false;

    Play_CurrentComboClip(state);
}

void PlayerState_Attack::Update(PlayerStateMachine* state, float timeDelta)
{
    auto input = state->Get_Input();
    CHECK_NULL(input);

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
    if (input)
        input->Set_InputMode(EPlayerInputMode::Normal);

    _cachedStateMachine = nullptr;
    _comboIndex = 0;
    _comboWindowOpen = false;
    _hasBufferedAttack = false;
    _activeProfile = nullptr;
    _activeProfileType = EAttackProfileType::Hand_Ground;

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

void PlayerState_Attack::Open_ComboWindow()
{
    _comboWindowOpen = true;

    if (_hasBufferedAttack &&
        _activeProfile &&
        _comboIndex < _activeProfile->maxCombo - 1)
    {
        Advance_Combo();
    }
}

void PlayerState_Attack::Close_ComboWindow()
{
    if (!_comboWindowOpen)
        return;

    _comboWindowOpen = false;
    _hasBufferedAttack = false;
}

void PlayerState_Attack::Buffer_AttackInput()
{
    _hasBufferedAttack = true;
}

void PlayerState_Attack::Reset_Combo()
{
    _comboIndex = 0;
    _comboWindowOpen = false;
    _hasBufferedAttack = false;
    _activeProfile = nullptr;
    _activeProfileType = EAttackProfileType::Hand_Ground;
}

void PlayerState_Attack::Advance_Combo()
{
    _comboIndex++;
    _comboWindowOpen = false;
    _hasBufferedAttack = false;

    if (_cachedStateMachine)
        Play_CurrentComboClip(_cachedStateMachine);
}

const FComboEntry* PlayerState_Attack::Get_CurrentComboEntry() const
{
    if (!_activeProfile || _activeProfile->combos.empty())
        return nullptr;

    int32 safeIdx = min(_comboIndex,
        static_cast<int32>(_activeProfile->combos.size()) - 1);

    if (safeIdx < 0)
        return nullptr;

    return &_activeProfile->combos[safeIdx];
}

void PlayerState_Attack::Select_Profile(PlayerStateMachine* state)
{
    auto owner = state->Get_Owner();
    CHECK_NULL(owner);

    auto equipment = owner->Get_Component<EquipmentComponent>();

    bool isAerial = false;
    auto movement = state->Get_Movement();
    if (movement)
        isAerial = !movement->Is_OnGround();

    if (state->Is_ForceGroundAttack())
    {
        isAerial = false;
        state->Set_ForceGroundAttack(false);
    }

    if (equipment)
    {
        _activeProfileType = equipment->Find_AttackProfileType(isAerial);
    }
    else
    {
        _activeProfileType = isAerial
            ? EAttackProfileType::Hand_Aerial
            : EAttackProfileType::Hand_Ground;
    }

    _activeProfile = GET_SINGLE(ComboProfile_Manager)->Find(_activeProfileType);

    if (!_activeProfile)
    {
        LOG_WARN("PlayerState_Attack: 프로파일 없음 -> profileType={}",
            magic_enum::enum_name(_activeProfileType));
    }
}

void PlayerState_Attack::Play_CurrentComboClip(PlayerStateMachine* state)
{
    if (!_activeProfile || _activeProfile->combos.empty())
    {
        state->Get_AnimationState()->Play_State("Attack_1");
        return;
    }

    int32 safeIndex = min(_comboIndex,
        static_cast<int32>(_activeProfile->combos.size()) - 1);

    const string& animKey = _activeProfile->combos[safeIndex].animStateKey;

    state->Get_AnimationState()->Play_State(animKey);
}

Shared<PlayerState_Attack> PlayerState_Attack::Create()
{
    auto state = make_shared<PlayerState_Attack>();

    return state;
}
