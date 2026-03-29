# 무기 스왑(Tab) 시스템 설계 가이드라인

> 작성일: 2026-03-29
> 목적: Tab 키 입력으로 격투형(Hand) ↔ 검술형(BigSword) 무기를 실시간 교체하는 기능 구현

---

## 전체 흐름 요약

```
[Tab 입력] → InputComponent (toggleWeaponDown 캡처)
          → PlayerStateMachine::Check_WeaponToggle()
          → Player::Apply_CustomizingPart(Weapon, ...)
          → EquipmentComponent::Equip_Part / Unequip_Part
          → _currentWeaponType 자동 갱신
          → 다음 공격 시 ComboProfileManager가 변경된 무기 타입 기준으로 프로파일 선택
```

---

## 1. InputComponent — Tab 키 입력 캡처

### [MODIFY] Client/Public/InputComponent.h

`FInputFrame` 구조체에 `toggleWeaponDown` 추가:

```cpp
struct FInputFrame
{
    float moveX = 0.f;      
    float moveY = 0.f;      
    float lookYaw = 0.f;    // Mouse X
    float lookPitch = 0.f;  // Mouse Y

    bool  dashDown = false;

    bool  jumpDown = false;
    bool  superJumpPress = false;
    bool  superJumpUp = false;
    float superJumpCharge = 0.f;

    bool  jumpDash = false;
    bool  attackDown = false;

    // [추가] Tab 키로 무기를 교체하는 입력. 눌린 프레임에만 true.
    bool  toggleWeaponDown = false;

    bool  useSkillDown[2] = { false, false };
    bool  useSkillPress[2] = {};
};
```

`FInputGate` 구조체에 `allowWeaponToggle` 추가:

```cpp
struct FInputGate
{
    bool allowMove = true;
    bool allowLook = true;
    bool allowDash = true;
    bool allowJump = true;
    bool allowSuperJump = true;
    bool allowSkill = true;
    bool allowJumpDash = true;
    bool allowAttack = true;

    // [추가] 무기 교체 허용 여부. BlockAll 등에서 false로 설정.
    bool allowWeaponToggle = true;

    void Disable_AllInput()
    {
        allowMove = false;
        allowLook = false;
        allowDash = false;
        allowJump = false;
        allowSuperJump = false;
        allowSkill = false;
        allowJumpDash = false;
        allowAttack = false;
        allowWeaponToggle = false;  // [추가]
    }

    void Enable_AllInput()
    {
        allowMove = true;
        allowLook = true;
        allowDash = true;
        allowJump = true;
        allowSuperJump = true;
        allowSkill = true;
        allowJumpDash = true;
        allowAttack = true;
        allowWeaponToggle = true;   // [추가]
    }
};
```

### [MODIFY] Client/Private/InputComponent.cpp — Update_Input

기존 `rawAttackDown` 아래에 추가:

```cpp
void InputComponent::Update_Input(float timeDelta)
{
    _frame = {};    

    if (!GAME->Is_GameInputEnabled())
    {
        _superJumpCharge = 0.f;
        return;
    }

    // ... 기존 raw 입력 변수들 ...

    const bool rawAttackDown = INPUT->KeyDown(KEY_TYPE::LBUTTON);

    // [추가] Tab 키 감지
    const bool rawToggleWeaponDown = INPUT->KeyDown(KEY_TYPE::TAB);

    // ... 기존 gate 적용 로직들 ...

    if (_inputGate.allowAttack)
    {
        _frame.attackDown = rawAttackDown;
    }

    // [추가] 무기 교체 gate 적용
    if (_inputGate.allowWeaponToggle)
    {
        _frame.toggleWeaponDown = rawToggleWeaponDown;
    }
}
```

### [MODIFY] Client/Private/InputComponent.cpp — Get_InputGate_Preset

각 모드에 `allowWeaponToggle` 값 추가:

```cpp
InputComponent::FInputGate InputComponent::Get_InputGate_Preset(EPlayerInputMode mode)
{
    FInputGate gate{};

    switch (mode)
    {
    case EPlayerInputMode::Normal:
        // ... 기존 값들 ...
        gate.allowWeaponToggle = true;   // [추가]
        break;

    case EPlayerInputMode::LookOnly:
        // ... 기존 값들 ...
        gate.allowWeaponToggle = false;  // [추가] 탐색 중엔 교체 불가
        break;

    case EPlayerInputMode::MoveAndLook:
        // ... 기존 값들 ...
        gate.allowWeaponToggle = true;   // [추가]
        break;

    case EPlayerInputMode::BlockAll:
        // ... 기존 값들 ...
        gate.allowWeaponToggle = false;  // [추가]
        break;
    }

    return gate;
}
```

---

## 2. PlayerStateMachine — 무기 교체 판정

### [MODIFY] Client/Public/PlayerStateMachine.h

private 영역에 함수 선언 추가:

```cpp
private:
    // [추가] Tab 키 입력 시 무기를 교체하는 글로벌 판정 함수.
    // Check_Global_Transitions에서 호출된다.
    bool Check_WeaponToggle();
```

### [MODIFY] Client/Private/PlayerStateMachine.cpp

`Check_Global_Transitions()` 함수 안에서 호출:

```cpp
bool PlayerStateMachine::Check_Global_Transitions()
{
    auto currentId = Get_CurrentStateID();
    if (currentId == EPlayerState::Dead)
        return false;

    // ... 기존 글로벌 판정들 ...

    // [추가] 무기 스왑 감지 — 상태 전이를 일으키지 않으므로 리턴값 무시
    Check_WeaponToggle();

    return false;
}
```

`Check_WeaponToggle()` 함수 본문:

```cpp
// Tab 키 입력 시 무기를 교체한다.
// 현재 Hand면 BigSword로, BigSword면 Hand로 토글한다.
// 상태 전이는 일으키지 않고 장비 상태만 변경하므로,
// 다음 공격 진입 시 자동으로 변경된 프로파일이 선택된다.
bool PlayerStateMachine::Check_WeaponToggle()
{
    auto input = Get_Input();
    if (!input)
        return false;

    const auto& frame = input->Get_Frame();

    if (!frame.toggleWeaponDown)
        return false;

    // 오너를 MyPlayer로 캐스팅
    auto player = dynamic_pointer_cast<MyPlayer>(Get_Owner());
    if (!player)
        return false;

    auto equipment = player->Get_Equipment();
    if (!equipment)
        return false;

    auto currentWeapon = equipment->Get_CurrentWeaponType();

    if (currentWeapon == EWeaponType::Hand)
    {
        // 격투 → 대검으로 교체
        player->Apply_CustomizingPart(EPartSlot::Weapon, TEXT("Model_BigSword"));
        LOG_INFO("무기 변경: 격투 -> 대검");
    }
    else
    {
        // 대검 → 격투(맨손)로 교체. "None"을 넘기면 Unequip 처리.
        player->Apply_CustomizingPart(EPartSlot::Weapon, TEXT("None"));
        LOG_INFO("무기 변경: 대검 -> 격투");
    }

    // 상태 전이는 일으키지 않음
    return false;
}
```

> **참고:** `Get_Owner()`가 `MyPlayer`가 아니라 `Player` 기반이라면 캐스팅 대상을 맞춰야 합니다.
> `Player`에 이미 `Apply_CustomizingPart`와 `_equipment` 멤버가 있으므로,
> `Player`로 캐스팅해도 됩니다. 프로젝트의 소유권 구조에 맞게 선택하세요.

---

## 3. Player / MyPlayer — Get_Equipment 접근자 (필요 시)

`PlayerStateMachine`에서 `EquipmentComponent`에 접근하기 위해 getter가 필요할 수 있습니다.
이미 `_equipment` 멤버가 `Player.h`에 있으므로 public getter만 추가하면 됩니다.

### [MODIFY] Client/Public/Player.h

```cpp
public:
    // [추가] 장비 컴포넌트 접근자. 무기 교체 판정에서 사용.
    Shared<EquipmentComponent> Get_Equipment() const { return _equipment; }
```

---

## 4. 동작 확인 체크리스트

| 단계 | 확인 내용 |
|------|----------|
| ① 빌드 | 컴파일 에러 없이 빌드 성공 |
| ② Tab 토글 | Idle 상태에서 Tab 누르면 대검 모델이 나타남/사라짐 반복 |
| ③ 로그 확인 | 콘솔에 `무기 변경: 격투 -> 대검` / `대검 -> 격투` 로그 출력 |
| ④ 콤보 전환 | 대검 장착 후 공격 시 `Attack_Sword_01` 콤보 진입 |
| ⑤ 맨손 복귀 | 다시 Tab 후 공격 시 `Attack_01` 콤보 진입 |

---

## 5. 향후 확장 포인트 (지금은 미구현)

- **교체 애니메이션:** 검을 뽑는/집어넣는 모션이 필요하다면 `EPlayerState::WeaponSwap` 상태를 추가하고, 해당 상태의 애니메이션이 끝난 뒤에 실제 `Apply_CustomizingPart`를 호출하도록 구성
- **공격 중 교체 제한:** `Check_WeaponToggle()` 함수 안에서 `Get_CurrentStateID() == EPlayerState::Attack` 이면 `return false;` 를 추가하면 공격 도중 교체를 막을 수 있음
- **네트워크 동기화:** 리모트 플레이어에게도 무기 교체를 반영하려면 `Build_NetworkInfo`에서 현재 `weaponType`을 패킷에 실어 보내야 함
