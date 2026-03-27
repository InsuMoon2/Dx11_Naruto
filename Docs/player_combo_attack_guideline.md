# 플레이어 콤보 공격 구현 가이드라인

> 마지막 갱신: 2026-03-27  
> 확정 방안: **A — 단일 `EPlayerState::Attack` + 내부 콤보 인덱스**  
> 분석 대상: `PlayerStateMachine`, `IPlayerState`, `AnimationStateComponent`, `InputComponent`, `AnimNotify/AnimNotifyState`, `MovementComponent`

---

## 1. 현재 구조 요약

### FSM

- `PlayerStateMachine`은 `umap<EPlayerState, Shared<IPlayerState>>`로 상태를 관리한다.
- `Change_State(EPlayerState)` — **동일 상태 재진입은 무시**된다 (==체크).
- `Force_Enter_State(EPlayerState)` — **Exit 호출 없이 강제 Enter**만 수행. 콤보 연결에 사용.
- 각 `IPlayerState`는 `Enter/Update/Exit(PlayerStateMachine*)` 시그니처.

### 입력

- `InputComponent::FInputFrame`에 입력이 매 프레임 캡처된다.
- **현재 공격 전용 입력 필드 없음** → 추가 필요.
- `FInputGate`로 카테고리별 on/off 제어.

### 애니메이션

- `AnimationStateComponent`가 `umap<string, FStateAnimationDesc>` 맵 보유.
- `FStateAnimationDesc::mode` — `Single`(단일 클립), `Sequence`(Start→Loop→End), `DirectionalSingle`.
- 상태 이름은 `magic_enum::enum_name(EPlayerState)` 결과를 그대로 사용 (예: `"Attack"`).
- 콤보 공격은 **`Single` 모드**를 사용. 각 콤보 클립을 별도 키(`"Attack_01"`, `"Attack_02"`, `"Attack_03"`)로 등록.

### AnimNotify 시스템

- 즉발형: `AnimNotify` → `Execute(FAnimNotifyContext&)`. 등록: `REGISTER_ANIM_NOTIFY(클래스명)`.
- 구간형: `AnimNotifyState` → `On_Begin/On_Tick/On_End(FAnimNotifyContext&)`. 등록: `REGISTER_ANIM_NOTIFY_STATE(클래스명)`.
- `FAnimNotifyContext`에 `owner(GameObject*)`, `model`, `clipName`, `currentTimeSec`, `isPreview` 등 제공.
- 클라이언트 노티파이 접두사: `AN_`(즉발), `ANS_`(구간).

---

## 2. 핵심 설계

### 콤보 인덱스 기반 흐름

```
Attack 상태 하나 안에서 _comboIndex(0, 1, 2)를 관리.
각 인덱스마다 별도 클립("Attack_01", "Attack_02", "Attack_03")을 재생.
콤보 전환은 Force_Enter_State(Attack)로 재진입하여 다음 클립 재생.
```

### ANS_ComboWindow (구간형 노티파이) 기반 입력 예약

```
[===== Attack_01 클립 =====]
    [--ANS_AttackTrace--]           ← 무기 히트 판정 활성 구간 (데미지)
         [---ANS_ComboWindow---]    ← 다음 공격 입력 예약 가능 구간
```

- `On_Begin` → 콤보 윈도우 열림 (입력 예약 가능)
- `On_End` → 윈도우 닫힘 → 예약된 입력이 있으면 다음 콤보, 없으면 Idle 복귀

> [!TIP]
> 즉발형 `AnimNotify` 2개(Open/Close)보다 **구간형 `AnimNotifyState` 1개가 더 안전**하다. 애니메이션이 중간에 끊겨도 `On_End`가 호출되어 윈도우가 안전하게 닫힌다.

---

## 3. 파일별 수정/생성 목록

| 작업 | 파일 | 위치 | 내용 |
|---|---|---|---|
| [변경] | `InputComponent.h/cpp` | `Client` | `FInputFrame`에 `attackDown` 추가, `FInputGate`에 `allowAttack` 추가 |
| [추가] | `PlayerState_Attack.h/cpp` | `Client/Public`, `Client/Private` | 콤보 상태 클래스 |
| [추가] | `ANS_ComboWindow.h/cpp` | `Client/Public`, `Client/Private` | 콤보 입력 구간 노티파이 |
| [추가] | `ANS_AttackTrace.h/cpp` | `Client/Public`, `Client/Private` | 무기 히트 판정 구간 노티파이 |
| [변경] | `PlayerStateMachine.h/cpp` | `Client` | Attack 상태 등록, `Get_CurrentState()` 접근자 추가 |
| [변경] | `PlayerState_Idle.cpp` | `Client/Private` | `Update`에서 `attackDown` → Attack 전이 |
| [변경] | `PlayerState_Run.cpp` | `Client/Private` | `Update`에서 `attackDown` → Attack 전이 |

---

## 4. 구현 상세

### 4-1. InputComponent — 공격 입력 추가

```cpp
// InputComponent.h — FInputFrame 내부에 추가
struct FInputFrame
{
    // ... 기존 필드 전부 유지 ...

    bool attackDown = false;    // 공격 키 Down (좌클릭 or 특정 키)
};
```

```cpp
// InputComponent.h — FInputGate 내부에 추가
struct FInputGate
{
    // ... 기존 필드 전부 유지 ...

    bool allowAttack = true;

    void Disable_AllInput()
    {
        // ... 기존 코드 ...
        allowAttack = false;
    }

    void Enable_AllInput()
    {
        // ... 기존 코드 ...
        allowAttack = true;
    }
};
```

```cpp
// InputComponent.cpp — Update_Input() 내부
// _inputGate.allowAttack && 좌클릭(DIK_LBUTTON 또는 VK_LBUTTON) Down 감지 시
// _frame.attackDown = true;
```

### 4-2. PlayerState_Attack — 콤보 상태 클래스

```cpp
// Client/Public/PlayerState_Attack.h
#pragma once
#include "IPlayerState.h"

NS_BEGIN(Client)

// 콤보 공격 상태.
// 내부에서 콤보 인덱스를 관리하고,
// ANS_ComboWindow 노티파이에 의해 콤보 전환 여부가 결정된다.
class PlayerState_Attack : public IPlayerState
{
public:
    PlayerState_Attack();
    ~PlayerState_Attack() override = default;

public:
    void Enter(PlayerStateMachine* state) override;
    void Update(PlayerStateMachine* state, float timeDelta) override;
    void Exit(PlayerStateMachine* state) override;

    EPlayerState Get_StateID() const override { return EPlayerState::Attack; }

public:
    // --- ANS_ComboWindow에서 호출되는 인터페이스 ---

    // 콤보 입력 예약 구간 시작 (ANS_ComboWindow::On_Begin)
    void Open_ComboWindow();

    // 콤보 입력 예약 구간 종료 (ANS_ComboWindow::On_End)
    // 내부에서 버퍼된 입력에 따라 다음 콤보 or Idle 복귀를 결정
    void Close_ComboWindow();

    // 공격 키 입력 예약 (Update에서 호출)
    void Buffer_AttackInput();

    // 콤보 인덱스 리셋
    void Reset_Combo();

    int32 Get_ComboIndex() const { return _comboIndex; }

private:
    static constexpr int32 MAX_COMBO = 3;   // 최대 3연타 (데이터화 가능)

    int32   _comboIndex = 0;                // 현재 콤보 단계 (0, 1, 2)
    bool    _comboWindowOpen = false;       // 현재 입력 예약 가능한가
    bool    _hasBufferedAttack = false;     // 윈도우 내에 공격 입력이 들어왔는가

    // 콤보 단계별 애니메이션 키 이름 (AnimationStateComponent에 등록된 key)
    vector<string> _comboClipNames;

    // Close_ComboWindow에서 상태 전이용 (Enter에서 캐싱)
    PlayerStateMachine* _cachedStateMachine = nullptr;

public:
    static Shared<PlayerState_Attack> Create();
};

NS_END
```

```cpp
// Client/Private/PlayerState_Attack.cpp
#include "pch.h"
#include "PlayerState_Attack.h"
#include "PlayerStateMachine.h"
#include "InputComponent.h"
#include "MovementComponent.h"
#include "AnimationStateComponent.h"
#include "Model.h"
#include "Transform.h"
#include "GameObject.h"

PlayerState_Attack::PlayerState_Attack()
{
    // 콤보 단계별 재생할 클립 키
    // AnimationStateComponent의 _stateAnimations에 이 이름들이 등록되어야 한다
    _comboClipNames = { "Attack_01", "Attack_02", "Attack_03" };
}

void PlayerState_Attack::Enter(PlayerStateMachine* state)
{
    if (!state) return;

    _cachedStateMachine = state;

    auto input = state->Get_Input();
    auto movement = state->Get_Movement();

    // 공격 중 이동 차단, 시점 이동만 허용
    if (input)
        input->Set_InputMode(EPlayerInputMode::LookOnly);

    // 이동에 의한 자동 회전 끄기 (공격 방향 고정)
    if (movement)
        movement->Set_OrientRotationToMovement(false);

    // 현재 콤보 인덱스에 해당하는 클립 재생
    int32 safeIndex = min(_comboIndex, static_cast<int32>(_comboClipNames.size()) - 1);
    state->Get_AnimationState()->Play_State(_comboClipNames[safeIndex]);

    // 윈도우 초기화 — 노티파이가 열어줄 때까지 닫혀있음
    _comboWindowOpen = false;
    _hasBufferedAttack = false;
}

void PlayerState_Attack::Update(PlayerStateMachine* state, float timeDelta)
{
    // 공격 입력이 들어오면 버퍼링
    auto input = state->Get_Input();
    if (input && input->Get_Frame().attackDown)
    {
        Buffer_AttackInput();
    }

    // 현재 클립이 완전히 끝났는데 아직 상태가 안 바뀌었으면 Idle 복귀 (보험)
    if (state->Is_AnimStateFinished())
    {
        Reset_Combo();
        state->Change_State(EPlayerState::Idle);
    }
}

void PlayerState_Attack::Exit(PlayerStateMachine* state)
{
    if (!state) return;

    // 입력/이동 원복
    auto movement = state->Get_Movement();
    if (movement)
        movement->Set_OrientRotationToMovement(true);

    auto input = state->Get_Input();
    if (input)
        input->Set_InputMode(EPlayerInputMode::Normal);

    _cachedStateMachine = nullptr;
}

void PlayerState_Attack::Open_ComboWindow()
{
    // ANS_ComboWindow::On_Begin에서 호출됨
    _comboWindowOpen = true;
    _hasBufferedAttack = false;
}

void PlayerState_Attack::Close_ComboWindow()
{
    // ANS_ComboWindow::On_End에서 호출됨
    _comboWindowOpen = false;

    if (_hasBufferedAttack && _comboIndex < MAX_COMBO - 1)
    {
        // --- 다음 콤보로 전이 ---
        _comboIndex++;
        _hasBufferedAttack = false;

        // 같은 EPlayerState::Attack이지만 새 클립으로 재진입해야 함
        // Change_State는 동일 상태를 무시하므로 Force_Enter_State 사용
        if (_cachedStateMachine)
            _cachedStateMachine->Force_Enter_State(EPlayerState::Attack);
    }
    else
    {
        // --- 콤보 종료 → Idle 복귀 ---
        Reset_Combo();

        if (_cachedStateMachine)
            _cachedStateMachine->Change_State(EPlayerState::Idle);
    }
}

void PlayerState_Attack::Buffer_AttackInput()
{
    // 콤보 윈도우가 열려있을 때만 입력 예약 허용
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
    return make_shared<PlayerState_Attack>();
}
```

### 4-3. PlayerStateMachine — Attack 등록 + 접근자

```cpp
// PlayerStateMachine.h — public 영역에 추가
Shared<IPlayerState> Get_CurrentState() const { return _currentState; }
```

```cpp
// PlayerStateMachine.cpp — Initialize_Prototype()에 추가
#include "PlayerState_Attack.h"

// 기존 Register_State들과 같은 위치에:
Register_State(EPlayerState::Attack, PlayerState_Attack::Create());
```

### 4-4. Idle/Run에서 Attack 진입

```cpp
// PlayerState_Idle.cpp — Update() 내부, 기존 전이 체크 뒤에 추가
// dashDown 체크 다음, moveInput 체크 이전에 넣으면 자연스럽다

if (frame.attackDown)
{
    state->Change_State(EPlayerState::Attack);
    return;
}
```

```cpp
// PlayerState_Run.cpp — Update() 내부, 동일하게 추가
if (frame.attackDown)
{
    state->Change_State(EPlayerState::Attack);
    return;
}
```

> [!NOTE]
> `Check_Skill_Input()`이 `Check_Global_Transitions()`에서 먼저 실행되므로, 스킬 입력이 공격보다 우선순위가 높다. 별도 처리 불필요.

### 4-5. ANS_ComboWindow — 콤보 입력 구간 노티파이

```cpp
// Client/Public/ANS_ComboWindow.h
#pragma once
#include "AnimNotifyState.h"

NS_BEGIN(Client)

// 콤보 공격 중 다음 입력을 예약할 수 있는 구간을 정의하는 노티파이.
// 에디터 Animation View에서 각 Attack 클립에 배치한다.
class ANS_ComboWindow : public AnimNotifyState
{
public:
    string Get_TypeName() const override;

    void On_Begin(const FAnimNotifyContext& context) override;
    void On_Tick(const FAnimNotifyContext& context)  override;
    void On_End(const FAnimNotifyContext& context)   override;

public:
    json Serialize_Payload() const override;
    void Deserialize_Payload(const json& payload) override;
    void Free() override;
};

NS_END
```

```cpp
// Client/Private/ANS_ComboWindow.cpp
#include "pch.h"
#include "ANS_ComboWindow.h"
#include "AnimNotify_Factory.h"
#include "GameObject.h"
#include "PlayerStateMachine.h"
#include "PlayerState_Attack.h"

// 팩토리 자동 등록 → 에디터 노티파이 타입 목록에 표시됨
REGISTER_ANIM_NOTIFY_STATE(ANS_ComboWindow)

string ANS_ComboWindow::Get_TypeName() const
{
    return "ANS_ComboWindow";
}

void ANS_ComboWindow::On_Begin(const FAnimNotifyContext& context)
{
    if (context.isPreview) return;  // 에디터 프리뷰에서는 게임 로직 실행 안함

    auto* owner = context.owner;
    if (!owner) return;

    auto psm = owner->Get_Component<PlayerStateMachine>();
    if (!psm || psm->Get_CurrentStateID() != EPlayerState::Attack) return;

    // PlayerState_Attack으로 캐스팅하여 콤보 윈도우 열기
    auto attackState = dynamic_pointer_cast<PlayerState_Attack>(psm->Get_CurrentState());
    if (attackState)
        attackState->Open_ComboWindow();
}

void ANS_ComboWindow::On_Tick(const FAnimNotifyContext& context)
{
    // 틱마다 처리할 로직은 없음 (필요하면 여기서 확장)
}

void ANS_ComboWindow::On_End(const FAnimNotifyContext& context)
{
    if (context.isPreview) return;

    auto* owner = context.owner;
    if (!owner) return;

    auto psm = owner->Get_Component<PlayerStateMachine>();
    if (!psm || psm->Get_CurrentStateID() != EPlayerState::Attack) return;

    auto attackState = dynamic_pointer_cast<PlayerState_Attack>(psm->Get_CurrentState());
    if (attackState)
        attackState->Close_ComboWindow();
}

json ANS_ComboWindow::Serialize_Payload() const
{
    return AnimNotifyState::Serialize_Payload();
}

void ANS_ComboWindow::Deserialize_Payload(const json& payload)
{
    AnimNotifyState::Deserialize_Payload(payload);
}

void ANS_ComboWindow::Free()
{
    AnimNotifyState::Free();
}
```

### 4-6. ANS_AttackTrace — 무기 히트 판정 노티파이

```cpp
// Client/Public/ANS_AttackTrace.h
#pragma once
#include "AnimNotifyState.h"

NS_BEGIN(Client)

// 무기가 실제로 데미지를 줄 수 있는 활성 프레임 구간.
// On_Begin: 무기 콜리전(충돌체) 활성화
// On_Tick:  오버랩 검사 → 대상에 데미지
// On_End:   무기 콜리전 비활성화
class ANS_AttackTrace : public AnimNotifyState
{
public:
    string Get_TypeName() const override;

    void On_Begin(const FAnimNotifyContext& context) override;
    void On_Tick(const FAnimNotifyContext& context)  override;
    void On_End(const FAnimNotifyContext& context)   override;

public:
    json Serialize_Payload() const override;
    void Deserialize_Payload(const json& payload) override;
    void Free() override;
};

NS_END
```

```cpp
// Client/Private/ANS_AttackTrace.cpp
#include "pch.h"
#include "ANS_AttackTrace.h"
#include "AnimNotify_Factory.h"
#include "GameObject.h"

REGISTER_ANIM_NOTIFY_STATE(ANS_AttackTrace)

string ANS_AttackTrace::Get_TypeName() const
{
    return "ANS_AttackTrace";
}

void ANS_AttackTrace::On_Begin(const FAnimNotifyContext& context)
{
    if (context.isPreview) return;

    // TODO: owner의 Weapon PartObject에서 콜리전 활성화
    // auto weapon = owner->Find_PartObject("Weapon");
    // weapon->Set_CollisionEnabled(true);
    LOG_INFO("ANS_AttackTrace : Begin — 무기 콜리전 활성화");
}

void ANS_AttackTrace::On_Tick(const FAnimNotifyContext& context)
{
    if (context.isPreview) return;

    // TODO: 매 틱 오버랩 검사 → 히트 대상에 데미지
}

void ANS_AttackTrace::On_End(const FAnimNotifyContext& context)
{
    if (context.isPreview) return;

    // TODO: 무기 콜리전 비활성화
    LOG_INFO("ANS_AttackTrace : End — 무기 콜리전 비활성화");
}

json ANS_AttackTrace::Serialize_Payload() const
{
    return AnimNotifyState::Serialize_Payload();
}

void ANS_AttackTrace::Deserialize_Payload(const json& payload)
{
    AnimNotifyState::Deserialize_Payload(payload);
}

void ANS_AttackTrace::Free()
{
    AnimNotifyState::Free();
}
```

---

## 5. 에디터 작업 순서

1. **AnimationStateComponent에 클립 등록** (Inspector)
   - key: `"Attack_01"`, mode: `Single`, animationName: 실제 Attack 1타 클립명
   - key: `"Attack_02"`, mode: `Single`, animationName: 실제 Attack 2타 클립명
   - key: `"Attack_03"`, mode: `Single`, animationName: 실제 Attack 3타 클립명

2. **각 클립에 ANS_AttackTrace 배치** (Animation View)
   - 무기가 실제로 휘둘러지는 프레임 구간

3. **각 클립에 ANS_ComboWindow 배치** (Animation View)
   - 타격 모션이 끝나고 칼을 거두기 시작하는 시점 ~ 클립 끝나기 약 5프레임 전
   - ANS_AttackTrace보다 뒤에 시작해야 자연스러움

4. **저장 → `Data/json/AnimNotifies/`에 JSON 기록**

---

## 6. 네트워크 동기화

현재 `AnimationStateComponent::Capture_FromStateMachine` → proto → 리모트 `Apply_NetworkState` 흐름.

A 방안에서는 `EPlayerState::Attack` 하나로만 동기화되므로, **콤보 인덱스를 별도 필드로 proto에 추가**해야 리모트 플레이어에서도 정확한 클립이 재생된다.

```protobuf
// Protocol.proto (또는 Struct.proto) — ObjectInfo에 추가
int32 combo_index = XX; // 현재 콤보 인덱스 (0, 1, 2)
```

```cpp
// AnimationStateComponent::FAnimReplicatedState에 추가
int32 comboIndex = 0;
```

```cpp
// Write_ToObjectInfo / Read_FromObjectInfo에서 combo_index 필드 읽기/쓰기
```

---

## 7. 전체 런타임 흐름

```
[Idle/Run::Update]
      │ frame.attackDown == true
      ▼
Change_State(Attack)
      │
      ▼
[PlayerState_Attack::Enter]
      │ comboIndex=0 → Play_State("Attack_01")
      ▼
[Attack_01 클립 재생 중]
      │
      │ ─ ANS_AttackTrace::On_Begin ─ (무기 콜리전 ON)
      │ ─ ANS_AttackTrace::On_End ─── (무기 콜리전 OFF)
      │
      │ ─ ANS_ComboWindow::On_Begin ─ Open_ComboWindow()
      │    │
      │    │  attackDown 감지? → Buffer_AttackInput() (예약)
      │    │
      │ ─ ANS_ComboWindow::On_End ─── Close_ComboWindow()
      │
      ├── 예약 있음 → comboIndex++ → Force_Enter_State(Attack)
      │                             → Enter → Play_State("Attack_02")
      │                             → 같은 흐름 반복
      │
      └── 예약 없음 → Reset_Combo()
                     → Change_State(Idle)
```

---

## 8. Force_Enter_State 사용 시 주의점

현재 `Force_Enter_State`는 **Exit을 호출하지 않고 Enter만 실행**한다.
콤보 연결 시 이전 Enter에서 설정한 상태(InputMode, OrientRotation 등)를 다시 세팅하므로 문제없다.

단, `Exit`에서 리소스 해제나 상태 초기화를 하는 로직이 있다면 누락될 수 있다.
`PlayerState_Attack::Exit`은 InputMode/OrientRotation 원복만 하므로, 콤보 연결 시에는 Exit을 건너뛰어도 안전하다 (Enter에서 다시 세팅하기 때문).

> [!IMPORTANT]
> 만약 콤보 전환 시에도 Exit을 실행하고 싶다면, `Force_Enter_State`대신 
> `PlayerStateMachine`에 `Reenter_CurrentState()` 같은 함수를 만들어서
> Exit → Enter 순서로 호출하도록 할 수 있다.
