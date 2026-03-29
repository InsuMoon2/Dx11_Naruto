# 공중 콤보(JumpAttack) 시스템 구현 가이드라인

> 작성일: 2026-03-29
> 목적: Jump / DoubleJump 중 공격 입력 시 공중 전용 콤보를 수행하는 `PlayerState_JumpAttack` 구현

---

## 전체 흐름

```
[PlayerState_Jump / DoubleJump]
  └─ frame.attackDown?
      └─ Change_State(EPlayerState::JumpAttack)

[PlayerState_JumpAttack::Enter]
  └─ EquipmentComponent → Find_AttackProfileType(isAerial = true)
  └─ ComboProfileManager → Hand_Aerial / BigSword_Aerial 프로파일
  └─ 중력 OFF + Y속도 0 (체공)
  └─ 첫 타 재생

[PlayerState_JumpAttack::Update]
  └─ ANS_ComboWindow Open + attackDown → Advance_Combo (다음 타)
  └─ 콤보 종료(애니메이션 끝남) → 중력 복원 → Fall/Idle 전이
  └─ 착지 감지 → 중력 복원 → Idle/Run 전이

[PlayerState_JumpAttack::Exit]
  └─ 중력 반드시 복원 (안전장치)
```

---

## 1. EPlayerState — 이미 등록 완료

`Client_Enum.h`에 `JumpAttack`이 이미 존재합니다. 추가 변경 없음.

```cpp
// Attack 진입 판별용
Attack, JumpAttack,
```

---

## 2. [NEW] PlayerState_JumpAttack.h

`PlayerState_Attack`과 매우 유사한 구조이며, 중력 제어 로직만 추가됩니다.

```cpp
// Client/Public/PlayerState_JumpAttack.h
#pragma once

#include "IPlayerState.h"

NS_BEGIN(Client)

// 공중 콤보 공격 상태.
// Jump/DoubleJump 중 공격 입력 시 진입한다.
// 콤보 중에는 중력을 끄고 체공하며, 콤보가 끝나면 중력을 복원하여 낙하한다.
// 지상 Attack과 유사하지만 중력 제어 + 착지 판정이 추가되어 별도 상태로 분리한다.
class PlayerState_JumpAttack : public IPlayerState
{
public:
    PlayerState_JumpAttack();
    ~PlayerState_JumpAttack() override = default;

public:
    void Enter(PlayerStateMachine* state) override;
    void Update(PlayerStateMachine* state, float timeDelta) override;
    void Exit(PlayerStateMachine* state) override;

    EPlayerState Get_StateID() const override { return EPlayerState::JumpAttack; }

public:
    // ANS_ComboWindow에서 호출
    void    Open_ComboWindow();
    void    Close_ComboWindow();

    void    Buffer_AttackInput();

    int32   Get_ComboIndex() const { return _comboIndex; }

    void    Reset_Combo();
    void    Advance_Combo();

    EAttackProfileType Get_ActiveProfileType() const { return _activeProfileType; }

private:
    // 무기타입 + 공중 여부로 프로파일 선택
    void    Select_Profile(PlayerStateMachine* state);
    // 현재 콤보 인덱스에 해당하는 애니메이션 클립 재생
    void    Play_CurrentComboClip(PlayerStateMachine* state);

private:
    int32   _comboIndex = 0;
    bool    _comboWindowOpen = false;
    bool    _hasBufferedAttack = false;

private: // 콤보 프로파일
    const FComboProfile* _activeProfile = nullptr;
    EAttackProfileType   _activeProfileType = EAttackProfileType::Hand_Aerial;

    // Close_ComboWindow에서 상태 전이용
    PlayerStateMachine* _cachedStateMachine = nullptr;

private: // 중력 제어
    // Exit에서 중력 복원 여부를 판단하기 위한 플래그.
    // Update 도중 이미 복원했다면 Exit에서 중복 복원하지 않는다.
    bool    _gravityRestored = false;

public:
    static Shared<PlayerState_JumpAttack> Create();
};

NS_END
```

---

## 3. [NEW] PlayerState_JumpAttack.cpp

```cpp
// Client/Private/PlayerState_JumpAttack.cpp
#include "pch.h"
#include "PlayerState_JumpAttack.h"

#include "AnimationStateComponent.h"
#include "PlayerStateMachine.h"
#include "InputComponent.h"
#include "MovementComponent.h"
#include "Transform.h"
#include "GameObject.h"
#include "EquipmentComponent.h"
#include "ComboProfile_Manager.h"

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

    // 입력 모드: 공중 콤보 중에는 이동 불가, 시점만 허용
    input->Set_InputMode(EPlayerInputMode::LookOnly);
    movement->Set_OrientRotationToMovement(false);

    // ── 중력 OFF + 체공 ──
    // JumpDash와 동일한 패턴. 콤보 중에는 공중에 떠있는다.
    movement->Set_GravityEnabled(false);

    // Y속도를 0으로 밀어서 상승/하강을 멈춘다
    Vec3 velocity = movement->Get_Velocity();
    velocity.y = 0.f;
    movement->Set_Velocity(velocity);

    _gravityRestored = false;

    // ── 콤보 프로파일 선택 ──
    // isAerial = true 고정 → Hand_Aerial 또는 BigSword_Aerial
    Select_Profile(state);

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

    // ── 콤보 입력 처리 ──
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

    // ── 착지 감지 ──
    // 콤보 도중이라도 바닥에 닿으면 즉시 종료한다.
    // (높이가 낮은 곳에서 공중 공격을 시작한 경우)
    if (movement->Is_OnGround())
    {
        Reset_Combo();

        // 중력 복원
        movement->Set_GravityEnabled(true);
        _gravityRestored = true;

        bool hasInput = input->Has_MoveInput();
        state->Change_State(hasInput ? EPlayerState::Run : EPlayerState::Idle);
        return;
    }

    // ── 콤보 종료 판정 ──
    // 현재 타수의 애니메이션이 끝났고, 다음 콤보로 진행하지 않았다면 → 낙하
    if (state->Is_AnimStateFinished())
    {
        Reset_Combo();

        // 중력 복원 후 자유 낙하
        movement->Set_GravityEnabled(true);
        _gravityRestored = true;

        // HeightLand가 있으면 HeightLand로, 아니면 Jump 상태로 돌아간다
        // (착지 판정은 Jump/HeightLand 내부에서 처리)
        state->Change_State(EPlayerState::Jump);
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

        // ── 안전장치: 어떤 경로로 나가든 중력을 반드시 복원한다 ──
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
}

// ── ANS_ComboWindow 연동 ──

void PlayerState_JumpAttack::Open_ComboWindow()
{
    _comboWindowOpen = true;

    // 버퍼링된 입력이 있으면 즉시 다음 타수로
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

    // 콤보 윈도우가 닫혔는데 다음 입력이 없었다 → 콤보 종료
    Reset_Combo();

    // 중력 복원 후 Jump로 전이 (자유 낙하)
    if (_cachedStateMachine)
    {
        auto movement = _cachedStateMachine->Get_Movement();
        if (movement)
        {
            movement->Set_GravityEnabled(true);
            _gravityRestored = true;
        }

        _cachedStateMachine->Change_State(EPlayerState::Jump);
    }
}

void PlayerState_JumpAttack::Buffer_AttackInput()
{
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

// ── 프로파일 선택 ──

void PlayerState_JumpAttack::Select_Profile(PlayerStateMachine* state)
{
    auto owner = state->Get_Owner();
    CHECK_NULL(owner);

    auto equipment = owner->Get_Component<EquipmentComponent>();

    // 공중 공격이므로 isAerial = true 고정
    if (equipment)
    {
        _activeProfileType = equipment->Find_AttackProfileType(/*isAerial=*/ true);
    }
    else
    {
        _activeProfileType = EAttackProfileType::Hand_Aerial;
    }

    _activeProfile = GET_SINGLE(ComboProfile_Manager)->Find(_activeProfileType);

    if (!_activeProfile)
    {
        LOG_WARN("PlayerState_JumpAttack: 프로파일 없음 -> profileType={}",
            magic_enum::enum_name(_activeProfileType));
    }
}

// ── 애니메이션 재생 ──

void PlayerState_JumpAttack::Play_CurrentComboClip(PlayerStateMachine* state)
{
    if (!_activeProfile || _activeProfile->combos.empty())
    {
        // fallback: 기본 공중 공격 클립
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
    return make_shared<PlayerState_JumpAttack>();
}
```

---

## 4. [MODIFY] ANS_ComboWindow.cpp — JumpAttack 상태도 처리

현재 `ANS_ComboWindow`는 `EPlayerState::Attack`만 체크합니다.
`JumpAttack`에서도 콤보 윈도우가 동작하도록 분기를 추가해야 합니다.

```cpp
// ANS_ComboWindow.cpp — On_Begin

void ANS_ComboWindow::On_Begin(const FAnimNotifyContext& context)
{
    if (context.isPreview) return;

    auto* owner = context.owner;
    CHECK_NULL(owner);

    auto psm = owner->Get_Component<PlayerStateMachine>();
    if (!psm)
        return;

    auto currentId = psm->Get_CurrentStateID();

    // [변경] Attack 또는 JumpAttack일 때 콤보 윈도우를 연다
    if (currentId == EPlayerState::Attack)
    {
        auto attackState =
            dynamic_pointer_cast<PlayerState_Attack>(psm->Get_CurrentState());
        if (attackState) attackState->Open_ComboWindow();
    }
    else if (currentId == EPlayerState::JumpAttack)
    {
        auto jumpAttackState =
            dynamic_pointer_cast<PlayerState_JumpAttack>(psm->Get_CurrentState());
        if (jumpAttackState) jumpAttackState->Open_ComboWindow();
    }
}
```

```cpp
// ANS_ComboWindow.cpp — On_End

void ANS_ComboWindow::On_End(const FAnimNotifyContext& context)
{
    if (context.isPreview) return;

    auto* owner = context.owner;
    CHECK_NULL(owner);

    auto psm = owner->Get_Component<PlayerStateMachine>();
    if (!psm)
        return;

    auto currentId = psm->Get_CurrentStateID();

    // [변경] Attack 또는 JumpAttack일 때 콤보 윈도우를 닫는다
    if (currentId == EPlayerState::Attack)
    {
        auto attackState =
            dynamic_pointer_cast<PlayerState_Attack>(psm->Get_CurrentState());
        if (attackState) attackState->Close_ComboWindow();
    }
    else if (currentId == EPlayerState::JumpAttack)
    {
        auto jumpAttackState =
            dynamic_pointer_cast<PlayerState_JumpAttack>(psm->Get_CurrentState());
        if (jumpAttackState) jumpAttackState->Close_ComboWindow();
    }
}
```

> **인클루드 추가 필요:** `ANS_ComboWindow.cpp` 상단에 `#include "PlayerState_JumpAttack.h"` 추가

---

## 5. [MODIFY] PlayerState_Jump.cpp — 공격 입력 시 JumpAttack 전이

`Update()` 안에서 공중에 있을 때 공격 입력을 감지합니다.

```cpp
void PlayerState_Jump::Update(PlayerStateMachine* state, float timeDelta)
{
    auto input = state->Get_Input();
    auto movement = state->Get_Movement();
    const auto& frame = input->Get_Frame();

    auto cmd = state->Init_MoveCommand();
    cmd.jump = false;

    if (frame.jumpDash)
    {
        state->Change_State(EPlayerState::JumpDash);
        return;    // [추가] 여기 return 빠져있으면 아래 로직 계속 타니까 주의
    }

    if (frame.jumpDown && movement->Can_DoubleJump())
    {
        cmd.doublejump = true;
        state->Change_State(EPlayerState::DoubleJump);
        return;    // [추가] 동일
    }

    // [추가] 공중에서 공격 입력 → JumpAttack 전이
    if (frame.attackDown && !movement->Is_OnGround())
    {
        state->Change_State(EPlayerState::JumpAttack);
        return;
    }

    movement->Apply_Command(cmd);
    movement->Update(timeDelta);

    // ... 기존 착지 판정 로직 ...
}
```

---

## 6. [MODIFY] PlayerState_DoubleJump.cpp — 동일하게 공격 전이 추가

```cpp
void PlayerState_DoubleJump::Update(PlayerStateMachine* state, float timeDelta)
{
    auto input = state->Get_Input();
    auto anim = state->Get_AnimationState();
    auto movement = state->Get_Movement();
    const auto& frame = input->Get_Frame();

    if (frame.jumpDash)
    {
        state->Change_State(EPlayerState::JumpDash);
        return;
    }

    // [추가] 공중에서 공격 입력 → JumpAttack 전이
    if (frame.attackDown && !movement->Is_OnGround())
    {
        state->Change_State(EPlayerState::JumpAttack);
        return;
    }

    auto cmd = state->Init_MoveCommand();
    movement->Apply_Command(cmd);
    movement->Update(timeDelta);

    // ... 기존 착지 판정 로직 ...
}
```

---

## 7. [MODIFY] PlayerStateMachine.cpp — 상태 등록

```cpp
HRESULT PlayerStateMachine::Initialize_Prototype()
{
    // ... 기존 등록 ...
    Register_State(EPlayerState::Attack, PlayerState_Attack::Create());
    Register_State(EPlayerState::JumpDash, PlayerState_JumpDash::Create());

    // [추가] 공중 콤보 상태 등록
    Register_State(EPlayerState::JumpAttack, PlayerState_JumpAttack::Create());

    return S_OK;
}
```

> **인클루드 추가 필요:** `PlayerStateMachine.cpp` 상단에 `#include "PlayerState_JumpAttack.h"` 추가

---

## 8. 에디터 세팅 체크리스트 (AnimationStateComponent)

JSON의 `animStateKey`와 일치하는 이름으로 애니메이션 상태를 등록해야 합니다.

| animStateKey | 설명 | mode |
|---|---|---|
| `Attack_Air_01` | 맨손 공중 1타 | Single |
| `Attack_Air_02` | 맨손 공중 2타 | Single |
| `Attack_Air_03` | 맨손 공중 3타 (막타) | Single |
| `Attack_SwordAir_01` | 대검 공중 1타 | Single |
| `Attack_SwordAir_02` | 대검 공중 2타 (막타) | Single |

각 클립에 `ANS_ComboWindow` 노티파이를 세팅합니다 (막타 제외).

---

## 9. 수정 파일 요약

| 파일 | 작업 |
|------|------|
| `PlayerState_JumpAttack.h` | **[NEW]** 헤더 신규 생성 |
| `PlayerState_JumpAttack.cpp` | **[NEW]** 구현 신규 생성 |
| `ANS_ComboWindow.cpp` | **[MODIFY]** JumpAttack 분기 추가 + 인클루드 |
| `PlayerState_Jump.cpp` | **[MODIFY]** attackDown → JumpAttack 전이 추가 |
| `PlayerState_DoubleJump.cpp` | **[MODIFY]** attackDown → JumpAttack 전이 추가 |
| `PlayerStateMachine.cpp` | **[MODIFY]** Register_State(JumpAttack) + 인클루드 |

---

## 10. 지상 Attack과의 차이점 비교

| 항목 | PlayerState_Attack (지상) | PlayerState_JumpAttack (공중) |
|------|--------------------------|------------------------------|
| Enter 시 중력 | 건드리지 않음 | `Set_GravityEnabled(false)` + Y속도 0 |
| 입력 모드 | Normal | LookOnly (이동 차단) |
| Select_Profile | `isAerial = !Is_OnGround()` | `isAerial = true` 고정 |
| 콤보 종료 후 | → Idle | → Jump (자유 낙하) |
| 착지 감지 | 없음 | Update에서 `Is_OnGround()` 체크 |
| Exit 안전장치 | 없음 | `Set_GravityEnabled(true)` 반드시 복원 |
| Close_ComboWindow | → Idle | → Jump (중력 복원 후 낙하) |
