# 무기 타입별 애니메이션 상태 분기 가이드라인

> 작성일: 2026-03-29
> 목적: 무기 타입이 바뀌면 `Idle`, `Run` 등 일반 상태의 애니메이션도 무기 variant로 자동 전환

---

## 핵심 아이디어

FSM 상태 enum은 건드리지 않는다. `EPlayerState::Idle`은 그대로 `Idle`이다.
실제 재생할 애니메이션 키만 **무기 타입에 따라** `"Idle"` → `"BigSword_Idle"`로 해석한다.

```
[FSM: EPlayerState::Idle]
    └─ Play_AnimState(Idle)
        └─ Resolve_StateNameByWeapon(owner, "Idle")
            ├─ WeaponType == Hand      → "Idle" (기본)
            └─ WeaponType == BigSword  → "BigSword_Idle" (등록돼있으면)
                                        → "Idle" (없으면 fallback)
```

---

## 왜 이 구조가 필요한가

### 문제: 각 상태 클래스마다 분기하면 확장이 불가능

```cpp
// ❌ 이렇게 하면 안 됨
void PlayerState_Idle::Enter(PlayerStateMachine* state)
{
    auto equipment = ...;
    if (equipment->Get_CurrentWeaponType() == BigSword)
        state->Play_AnimState(EPlayerState::BigSword_Idle);  // enum 추가 필요
    else
        state->Play_AnimState(EPlayerState::Idle);
}
// → Run, Jump, Dash, Land 전부 이런 식으로 예외 처리가 퍼진다!
```

### 해결: 해석 함수 하나로 모든 경로를 커버

| 경로 | 상태명 해석 위치 |
|------|--------------|
| 로컬 FSM → `Play_AnimState(Idle)` | `Resolve_StateNameByWeapon` |
| 네트워크 → `Apply_NetworkState()` → `To_AnimationStateName` | `Resolve_StateNameByWeapon` |

**두 경로 모두 같은 함수를 거치므로, 원격 플레이어도 동일하게 무기별 애니메이션을 재생한다.**

---

## 수정 대상 파일 요약

| 파일 | 작업 |
|------|------|
| `AnimationStateComponent.h` | **[MODIFY]** `Resolve_StateNameByWeapon` static 함수 선언 추가 |
| `AnimationStateComponent.cpp` | **[MODIFY]** `Resolve_StateNameByWeapon` 구현 + `Apply_NetworkState` 수정 |
| `PlayerStateMachine.cpp` | **[MODIFY]** `Play_AnimState`, `Play_DirectionalAnimState`, `Play_AnimStateLoopOnly`, `Find_AnimStateDesc`에서 해석 함수 사용 |

---

## 1. [MODIFY] AnimationStateComponent.h — 해석 함수 선언 추가

```cpp
// private: 영역 기존 함수들 바로 아래에 추가
private:
    static string To_AnimationStateName(EPlayerState state);

    // [추가] 무기 타입에 따라 상태 이름을 variant로 해석하는 공용 함수.
    // owner의 EquipmentComponent를 조회하여 BigSword면 "BigSword_" 접두어를 붙인다.
    // _stateAnimations에 해당 variant가 등록되어 있으면 그 이름을 반환하고,
    // 없으면 기본 상태명으로 fallback한다.
    // 
    // 사용처:
    //   1) PlayerStateMachine::Play_AnimState 등 로컬 FSM 경로
    //   2) AnimationStateComponent::Apply_NetworkState 네트워크 수신 경로
    //
    // 주의: owner가 nullptr이면 기본 상태명을 그대로 반환한다.
    string Resolve_StateNameByWeapon(Engine::GameObject* owner, EPlayerState state) const;

    static bool Requires_ForceRestart(EPlayerState state);
    // ... 나머지 기존 함수들 ...
```

> **참고:** `static`이 아니라 `const` 멤버 함수로 만드는 이유는,
> 내부에서 `_stateAnimations`에 variant 키가 등록돼 있는지 확인해야 하기 때문이다.
> `Find_State()`를 호출해서 검증한다.

---

## 2. [MODIFY] AnimationStateComponent.cpp — Resolve_StateNameByWeapon 구현

```cpp
#include "EquipmentComponent.h"  // [추가] 인클루드 필요

// 무기 타입에 따라 애니메이션 상태 키를 variant로 해석한다.
// 예: weaponType == BigSword이고, "BigSword_Idle"이 _stateAnimations에 등록돼 있으면
//     "BigSword_Idle"을 반환. 없으면 "Idle"로 fallback.
// Hand 타입은 항상 기본 상태명을 그대로 반환한다.
string AnimationStateComponent::Resolve_StateNameByWeapon(
    Engine::GameObject* owner, EPlayerState state) const
{
    // 1) 기본 상태명 생성 (enum 이름 그대로)
    string baseName = To_AnimationStateName(state);
    if (baseName.empty())
        return baseName;

    // 2) owner에서 EquipmentComponent를 가져온다
    if (!owner)
        return baseName;

    auto equipment = owner->Get_Component<EquipmentComponent>();
    if (!equipment)
        return baseName;

    // 3) Hand 타입이면 기본 상태명 그대로
    EWeaponType weaponType = equipment->Get_CurrentWeaponType();
    if (weaponType == EWeaponType::Hand)
        return baseName;

    // 4) BigSword 타입 → "BigSword_" + 기본 상태명 후보 생성
    string variantName = "BigSword_" + baseName;  // "BigSword_Idle", "BigSword_Run" 등

    // 5) variant가 _stateAnimations에 등록되어 있으면 사용, 없으면 fallback
    if (Find_State(variantName) != nullptr)
        return variantName;

    // fallback: variant가 없으면 기본 상태명으로
    return baseName;
}
```

---

## 3. [MODIFY] AnimationStateComponent.cpp — Apply_NetworkState 수정

`Apply_NetworkState()`에서 기존 `To_AnimationStateName`을 `Resolve_StateNameByWeapon`으로 교체한다.

### 변경 전 (라인 561)
```cpp
const string stateName = To_AnimationStateName(_replicatedState.state);
```

### 변경 후
```cpp
// [변경] 무기 타입에 따라 variant 상태명으로 해석한다.
// 원격 플레이어도 BigSword 장착 시 BigSword_Idle 등이 재생된다.
const string stateName = Resolve_StateNameByWeapon(
    Get_Owner().get(), _replicatedState.state);
```

> **중요:** 이 한 줄만 바꾸면 네트워크 경로도 자동으로 무기 variant를 타게 된다.

---

## 4. [MODIFY] PlayerStateMachine.cpp — 로컬 FSM 경로 수정

`To_AnimationStateName(stateID)` 호출을 `_animationState->Resolve_StateNameByWeapon(...)` 으로 교체한다.

### 변경 전
```cpp
// 라인 303~306
bool PlayerStateMachine::Play_AnimState(EPlayerState stateID)
{
    return _animationState->Play_State(To_AnimationStateName(stateID));
}

// 라인 308~311
bool PlayerStateMachine::Play_DirectionalAnimState(EPlayerState stateID, EMoveInputDirection dir)
{
    return _animationState->Play_DirectionalState(To_AnimationStateName(stateID), dir);
}

// 라인 313~316
bool PlayerStateMachine::Play_AnimStateLoopOnly(EPlayerState stateID)
{
    return _animationState->Play_StateLoopOnly(To_AnimationStateName(stateID));
}

// 라인 333~337
const FStateAnimationDesc* PlayerStateMachine::Find_AnimStateDesc(EPlayerState stateID) const
{
    return _animationState ?
        _animationState->Find_State(To_AnimationStateName(stateID)) : nullptr;
}
```

### 변경 후
```cpp
// [변경] 무기 타입에 따라 variant 상태명으로 해석한다.
bool PlayerStateMachine::Play_AnimState(EPlayerState stateID)
{
    string resolved = _animationState->Resolve_StateNameByWeapon(
        Get_Owner().get(), stateID);

    return _animationState->Play_State(resolved);
}

// [변경] Directional도 동일하게 variant 해석
bool PlayerStateMachine::Play_DirectionalAnimState(EPlayerState stateID, EMoveInputDirection dir)
{
    string resolved = _animationState->Resolve_StateNameByWeapon(
        Get_Owner().get(), stateID);

    return _animationState->Play_DirectionalState(resolved, dir);
}

// [변경] LoopOnly도 동일하게 variant 해석
bool PlayerStateMachine::Play_AnimStateLoopOnly(EPlayerState stateID)
{
    string resolved = _animationState->Resolve_StateNameByWeapon(
        Get_Owner().get(), stateID);

    return _animationState->Play_StateLoopOnly(resolved);
}

// [변경] Find도 동일하게 variant 해석
const FStateAnimationDesc* PlayerStateMachine::Find_AnimStateDesc(EPlayerState stateID) const
{
    if (!_animationState)
        return nullptr;

    string resolved = _animationState->Resolve_StateNameByWeapon(
        Get_Owner().get(), stateID);

    return _animationState->Find_State(resolved);
}
```

---

## 5. 에디터 데이터 등록 (AnimationStateComponent에 state key 추가)

기존 `Idle`, `Run` 등은 그대로 유지하고, 새로운 variant를 **별도 state key**로 추가 등록한다.

| state key | mode | 설명 |
|-----------|------|------|
| `Idle` | Single | 맨손 대기 (기존) |
| `BigSword_Idle` | Single | 대검 대기 (신규 등록) |
| `Run` | Sequence | 맨손 달리기 start/loop/end (기존) |
| `BigSword_Run` | Sequence | 대검 달리기 start/loop/end (필요 시 신규) |
| `Jump` | Sequence | 맨손 점프 (기존) |
| `BigSword_Jump` | Sequence | 대검 점프 (필요 시 신규) |
| `Dash` | DirectionalSingle | 맨손 대쉬 (기존) |
| `BigSword_Dash` | DirectionalSingle | 대검 대쉬 (필요 시 신규) |

> **핵심:** `BigSword_XXX`가 등록 안 돼 있으면 자동으로 `XXX`로 fallback.
> 모든 상태에 variant를 만들 필요 없이, **애니메이션이 다른 상태만** 추가하면 된다.

---

## 6. 공격 상태는 영향 없음

`PlayerState_Attack`과 `PlayerState_JumpAttack`은 `ComboProfile_Manager`에서 `animStateKey`를 직접 가져와서 `Play_State("Attack_Sword_01")` 처럼 string으로 호출한다.

이 경로는 `Play_AnimState(EPlayerState)` 를 거치지 않으므로 `Resolve_StateNameByWeapon`의 영향을 받지 않는다. **기존과 동일하게 동작.**

---

## 7. 네트워크 무기 타입 동기화 (주의사항)

현재 `Apply_NetworkState()`는 `Get_Owner()`에서 `EquipmentComponent`를 읽어 무기 타입을 판단한다.
원격 플레이어의 `EquipmentComponent._currentWeaponType`이 **네트워크로 동기화되어 있어야** 원격에서도 `BigSword_Idle`이 재생된다.

### 현재 상태
패킷에 `weaponType`이 아직 포함되어 있지 않다면, 원격 플레이어는 항상 `Hand` 상태로 보임.

### 해결 방안 (추후)
`Write_ToObjectInfo`에서 `equipment->Get_CurrentWeaponType()`을 실어보내고,
`Read_FromObjectInfo`에서 `EquipmentComponent`에 세팅하면 된다.
**이번 작업 범위에서는 로컬 플레이어만 먼저 동작하게 하고, 네트워크 동기화는 다음 단계에서 한다.**

---

## 8. 전체 흐름 다이어그램 (확정)

```
[로컬 경로]
PlayerState_Idle::Enter
  └─ state->Play_AnimState(EPlayerState::Idle)
      └─ PlayerStateMachine::Play_AnimState(Idle)
          └─ _animationState->Resolve_StateNameByWeapon(owner, Idle)
              ├─ Hand     → "Idle"
              └─ BigSword → "BigSword_Idle" (있으면) or "Idle" (fallback)
          └─ _animationState->Play_State("BigSword_Idle")

[네트워크 경로]
AnimationStateComponent::Apply_NetworkState
  └─ Resolve_StateNameByWeapon(Get_Owner().get(), state)
      ├─ Hand     → "Idle"
      └─ BigSword → "BigSword_Idle" (있으면) or "Idle" (fallback)
  └─ Play_State("BigSword_Idle")
```

---

## 9. 테스트 체크리스트

| # | 시나리오 | 예상 결과 |
|---|---------|----------|
| 1 | 격투형 Idle | `Idle` 애니메이션 재생 |
| 2 | Tab → 대검형 Idle | `BigSword_Idle` 재생 |
| 3 | 대검형 Run | `BigSword_Run` 등록돼있으면 재생, 없으면 `Run` fallback |
| 4 | 대검형 Attack | ComboProfile에서 `Attack_Sword_01` 직접 재생 (이 시스템 영향 X) |
| 5 | Tab 후 즉시 전환 | 현재 상태를 유지하면서 다음 상태 진입 시 variant 적용 |
| 6 | (추후) 원격 플레이어 | weaponType 동기화 후 BigSword_Idle 확인 |
