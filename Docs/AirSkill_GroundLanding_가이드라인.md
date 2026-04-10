# Rasengan Air 스킬 — 착지 전환 + 공중/지상 대쉬 분기 상세 가이드라인

> 대상 스킬: `Skill_Rasengan_Air`  
> 연관 상태: `Skill_Rasengan`, `Skill_Rasengan_End`  
> 관련 클래스: `PlayerState_Skill`, `AnimationStateComponent`, `ResourceLoader`

---

## 0. 전체 시나리오 정의

### 시나리오 A — 공중 차징 → 착지 → 지상 대쉬

```
공중 스킬 발동
  → Skill_Rasengan_Air: Start → Loop (공중 부유, Hold 유지)
  → 바닥에 닿음
  → 중력 복원 + Play_StateLoopOnly("Skill_Rasengan") (Start 스킵, Loop 바로)
  → Hold 해제
  → Skill_Rasengan: End 재생 + 대쉬 시작
  → 거리 도달 → Play_State("Skill_Rasengan_End")  ← attackEndAnimStateName
  → Skill_Rasengan_End 끝 → Idle
```

### 시나리오 B — 공중 차징 → 착지 없이 바로 뗌 → 공중 대쉬

```
공중 스킬 발동
  → Skill_Rasengan_Air: Start → Loop (공중 부유, Hold 잠깐)
  → Hold 해제
  → Skill_Rasengan_Air: End 재생 시작 (= 공중 돌진 애니메이션)
  + 대쉬 이동 시작
  → 거리 도달 → Stop_Dash(), airAttackEndAnimStateName 비어있으면 End 클립 자연 유지
  → End 클립 종료 (Non-Loop 확인 필수) → Idle
```

---

## 1. 현재 문제 지점

### 문제 1 — `Begin_AttackPhase()`가 Air/지상 구분 없이 `attackEndAnimStateName` 사용

```cpp
// 현재 코드 (322~338번 줄)
void PlayerState_Skill::Begin_AttackPhase(PlayerStateMachine* state)
{
    _subPhase = ESkillSubPhase::Attacking;

    auto movement = state->Get_Movement();
    if (movement) movement->Stop_Dash();

    auto skillData = GET_SINGLE(SkillDataManager)->Get_SkillData(_mySkill_Id);
    if (skillData && !skillData->attackEndAnimStateName.empty())
    {
        // → 공중 대쉬 후에도 "Skill_Rasengan_End"를 재생해버림
        state->Get_AnimationState()->Play_State(skillData->attackEndAnimStateName);
    }
}
```

**결과**: 시나리오 B에서 공중 대쉬 중 Air Sequence End 클립이 재생되고 있는데,
거리 도달 시 `Skill_Rasengan_End`로 교체되어버림 → 어색한 전환.

### 문제 2 — `FSkillData`에 Air 전용 마무리 필드가 없음

지상용 `attackEndAnimStateName`만 있고, 공중용 마무리를 별도로 지정할 수 없음.

---

## 2. 변경 파일 목록

| # | 파일 | 변경 내용 |
|---|---|---|
| 1 | `Client/Public/Client_Struct.h` | `FSkillData`에 `airAttackEndAnimStateName`, `airLandedAnimStateName` 추가 |
| 2 | `Client/Public/PlayerState_Skill.h` | `_hasLanded` 플래그 멤버 추가 |
| 3 | `Client/Private/PlayerState_Skill.cpp` | `Enter()` 리셋, `Update_Charging()` 착지 감지, `Begin_AttackPhase()` 분기 |
| 4 | `Client/Private/ResourceLoader.cpp` | `Build_AllResourceJobs()` 파싱 2개 추가 |
| 5 | `Client/Bin/Resources/Data/json/DT_SkillData.json` | 두 필드 세팅 |

---

## 3. 파일별 상세 수정

---

### [변경 1] `Client/Public/Client_Struct.h`

**위치**: `FSkillData` 구조체, 공중 스킬 관련 필드 영역 (현재 34~36번 줄)

**현재 코드:**

```cpp
// 대쉬 루프 이후 애니메이션
string  attackEndAnimStateName = "";

// 공중 스킬
string  airAnimStateName = "";
bool    airGravityOff = false;
```

**[변경] 두 필드 추가:**

```cpp
// 대쉬 루프 이후 애니메이션 (지상 대쉬 후 마무리)
string  attackEndAnimStateName = "";

// 공중 스킬
string  airAnimStateName = "";
bool    airGravityOff = false;

// [추가] 공중 차징 중 착지 시 전환할 AnimState 키
// - 비어있으면 착지해도 아무 처리 안 함 (공중 고정형 스킬)
// - 지상 버전 스킬 이름 세팅 (예: "Skill_Rasengan")
string  airLandedAnimStateName = "";

// [추가] 공중 대쉬 후 마무리 AnimState 키
// - 비어있으면 현재 재생 중인 Sequence End 클립 자연 유지 (별도 교체 없음)
// - 공중 전용 마무리 상태가 있다면 해당 이름 세팅
string  airAttackEndAnimStateName = "";
```

---

### [변경 2] `Client/Public/PlayerState_Skill.h`

**위치**: `private` 멤버 변수 영역 (`_isEnding` 바로 아래, 56~57번 줄)

**현재 코드:**

```cpp
private:
    EPlayerState _myStateId = EPlayerState::END; // 스킬별로 ID 타입 세팅해주기
    int32        _mySkill_Id = 0;

    float        _channelingTimer = 0.f;         // 루프를 도는 지속시간 체크용
    bool         _isEnding = false;
```

**[변경]:**

```cpp
private:
    EPlayerState _myStateId = EPlayerState::END; // 스킬별로 ID 타입 세팅해주기
    int32        _mySkill_Id = 0;

    float        _channelingTimer = 0.f;         // 루프를 도는 지속시간 체크용
    bool         _isEnding = false;

    // [추가] Air 스킬 차징 중 착지 감지 후 지상 AnimState로 교체 완료 여부
    // - Enter() 시 false 초기화
    // - 착지 감지 시 true → 이후 재교체 방지
    // - Begin_AttackPhase() 에서도 참조: true = 지상 마무리, false = 공중 마무리
    bool         _hasLanded = false;
```

---

### [변경 3] `Client/Private/PlayerState_Skill.cpp`

#### 3-A. `Enter()` — `_hasLanded` 리셋

**현재 코드 (56~61번 줄):**

```cpp
    _channelingTimer = 0.f;
    _isEnding = false;
    _subPhase = ESkillSubPhase::Charging;

    _dashStartPos = transform->Get_WorldPosition();
```

**[변경]:**

```cpp
    _channelingTimer = 0.f;
    _isEnding        = false;
    _subPhase        = ESkillSubPhase::Charging;
    _hasLanded       = false; // [추가] Enter 시 착지 플래그 초기화

    _dashStartPos = transform->Get_WorldPosition();
```

---

#### 3-B. `Update_Charging()` — 착지 감지 블록 삽입

**현재 전체 코드 (167~199번 줄):**

```cpp
void PlayerState_Skill::Update_Charging(PlayerStateMachine* state, float timeDelta)
{
    EAnimPhase phase = state->Get_AnimPhase();
    if (phase != EAnimPhase::Loop)
        return;

    _channelingTimer += timeDelta;

    auto skillData = GET_SINGLE(SkillDataManager)->Get_SkillData(_mySkill_Id);
    float maxDuration = skillData ? skillData->loopDurationSec : 0.f;
    bool isHold = skillData ? skillData->isHoldSkill : false;
    bool shouldEnd = false;

    if (isHold)
    {
        auto input = state->Get_Input();
        int32 currentSlot = state->Get_ActiveSkillSlot();

        if (!input->Get_Frame().useSkillPress[currentSlot] || _channelingTimer >= maxDuration)
            shouldEnd = true;
    }
    else
    {
        if (_channelingTimer >= maxDuration)
            shouldEnd = true;
    }

    if (shouldEnd)
    {
        state->Request_AnimStateEnd();
        Begin_DashPhase(state);
    }
}
```

**[변경] 최종 완성형:**

```cpp
void PlayerState_Skill::Update_Charging(PlayerStateMachine* state, float timeDelta)
{
    EAnimPhase phase = state->Get_AnimPhase();
    if (phase != EAnimPhase::Loop)
        return; // 아직 Start 재생 중이면 대기

    // skillData, movement 상단 1회 조회 (착지 블록 + shouldEnd 블록 공유)
    auto skillData = GET_SINGLE(SkillDataManager)->Get_SkillData(_mySkill_Id);
    auto movement  = state->Get_Movement();

    // ─────────────────────────────────────────────────────────────────────────
    // [추가] Air 스킬 차징 Loop 중 착지 감지 → 지상 AnimState로 교체
    //
    // 시나리오 A 전용 처리:
    //   Loop 중 바닥에 착지하면 Skill_Rasengan_Air → Skill_Rasengan (Loop만) 교체
    //   이후 Hold 해제 시 Begin_DashPhase → 지상 대쉬로 이어짐
    //
    // _hasLanded = true로 세팅해두면 Begin_AttackPhase()에서
    //   지상 attackEndAnimStateName 을 사용하게 됨
    // ─────────────────────────────────────────────────────────────────────────
    if (!_hasLanded && movement && skillData && movement->Is_OnGround())
    {
        if (!skillData->airLandedAnimStateName.empty())
        {
            // 착지 즉시 중력 복원 (airGravityOff가 켜져 있었던 경우 대비)
            movement->Set_GravityEnabled(true);

            // 지상 Loop AnimState로 교체 (Start 스킵, Loop 바로 재생)
            state->Get_AnimationState()->Play_StateLoopOnly(
                skillData->airLandedAnimStateName);

            _hasLanded = true; // 이후 프레임에서 재진입 방지
        }
    }
    // ─────────────────────────────────────────────────────────────────────────

    _channelingTimer += timeDelta;

    float maxDuration = skillData ? skillData->loopDurationSec : 0.f;
    bool  isHold      = skillData ? skillData->isHoldSkill     : false;
    bool  shouldEnd   = false;

    if (isHold)
    {
        auto input        = state->Get_Input();
        int32 currentSlot = state->Get_ActiveSkillSlot();

        if (!input->Get_Frame().useSkillPress[currentSlot] || _channelingTimer >= maxDuration)
            shouldEnd = true;
    }
    else
    {
        if (_channelingTimer >= maxDuration)
            shouldEnd = true;
    }

    if (shouldEnd)
    {
        state->Request_AnimStateEnd();
        Begin_DashPhase(state);
    }
}
```

---

#### 3-C. `Begin_AttackPhase()` — 착지 여부 기반 분기

**현재 코드 (322~338번 줄):**

```cpp
void PlayerState_Skill::Begin_AttackPhase(PlayerStateMachine* state)
{
    _subPhase = ESkillSubPhase::Attacking;

    // 대쉬 멈추고,
    auto movement = state->Get_Movement();
    if (movement)
        movement->Stop_Dash();

    // Attack End 애니메이션 재생
    auto skillData = GET_SINGLE(SkillDataManager)->Get_SkillData(_mySkill_Id);
    if (skillData && !skillData->attackEndAnimStateName.empty())
    {
        // 이름을 데이터랑 Enum값이랑 잘 맞춰야함
        state->Get_AnimationState()->Play_State(skillData->attackEndAnimStateName);
    }
}
```

**[변경] 최종 완성형:**

```cpp
void PlayerState_Skill::Begin_AttackPhase(PlayerStateMachine* state)
{
    _subPhase = ESkillSubPhase::Attacking;

    // 대쉬 멈추고
    auto movement = state->Get_Movement();
    if (movement)
        movement->Stop_Dash();

    auto skillData = GET_SINGLE(SkillDataManager)->Get_SkillData(_mySkill_Id);
    if (!skillData) return;

    // ─────────────────────────────────────────────────────────────────────────
    // [변경] 착지 여부(_hasLanded)로 마무리 AnimState 분기
    //
    // _hasLanded == true  → 시나리오 A: 공중 → 착지 → 지상 대쉬
    //   attackEndAnimStateName 사용 (예: "Skill_Rasengan_End")
    //
    // _hasLanded == false → 시나리오 B: 공중 → 바로 뗌 → 공중 대쉬
    //   airAttackEndAnimStateName 사용
    //   비어있으면 현재 재생 중인 Air Sequence End 클립이 자연 유지됨
    //   (별도 Play_State 호출 없음 = Stop_Dash 후 End 클립 끝까지 재생)
    // ─────────────────────────────────────────────────────────────────────────
    const string& endStateName = _hasLanded
        ? skillData->attackEndAnimStateName
        : skillData->airAttackEndAnimStateName;

    if (!endStateName.empty())
    {
        state->Get_AnimationState()->Play_State(endStateName);
    }
    // endStateName이 비어있으면 → 현재 재생 중인 End 클립 자연 유지
}
```

---

### [변경 4] `Client/Private/ResourceLoader.cpp`

**위치**: `Build_AllResourceJobs()` 함수, Skill 파싱 블록 (542번 줄 근처)

**현재 코드:**

```cpp
job.skillData.airAnimStateName = item.value("airAnimStateName", string{});

if (item.contains("airGravityOff"))
{
    const auto& gravityValue = item["airGravityOff"];
    if (gravityValue.is_boolean())
        job.skillData.airGravityOff = gravityValue.get<bool>();
    else if (gravityValue.is_number())
        job.skillData.airGravityOff = (gravityValue.get<float>() != 0.f);
}

job.skillIconSrvIndex = iconSrvIndex;
```

**[변경] 두 필드 파싱 추가:**

```cpp
job.skillData.airAnimStateName = item.value("airAnimStateName", string{});

if (item.contains("airGravityOff"))
{
    const auto& gravityValue = item["airGravityOff"];
    if (gravityValue.is_boolean())
        job.skillData.airGravityOff = gravityValue.get<bool>();
    else if (gravityValue.is_number())
        job.skillData.airGravityOff = (gravityValue.get<float>() != 0.f);
}

// [추가] 공중 차징 착지 시 전환할 지상 AnimState 이름
job.skillData.airLandedAnimStateName    = item.value("airLandedAnimStateName",    string{});

// [추가] 공중 대쉬 후 마무리 AnimState 이름 (비어있으면 Sequence End 자연 재생)
job.skillData.airAttackEndAnimStateName = item.value("airAttackEndAnimStateName", string{});

job.skillIconSrvIndex = iconSrvIndex;
```

---

### [변경 5] `Client/Bin/Resources/Data/json/DT_SkillData.json`

**현재 Rasengan 항목:**

```json
{
    "SkillID": 1001,
    "SkillName": "Rasengan",
    "Cooldown": 8,
    "AnimStateName": "Skill_Rasengan",
    "LoopDurationSec": 5,
    "IsHoldSkill": true,
    "hasDashPhase": true,
    "dashSpeed": 14,
    "maxDashDistance": 12,
    "targetStopDistance": 1.5,
    "attackEndAnimStateName": "Skill_Rasengan_End",
    "airAnimStateName": "Skill_Rasengan_Air",
    "airGravityOff": true
}
```

**[변경] 두 필드 추가:**

```json
{
    "SkillID": 1001,
    "SkillName": "Rasengan",
    "Cooldown": 8,
    "AnimStateName": "Skill_Rasengan",
    "LoopDurationSec": 5,
    "IsHoldSkill": true,
    "hasDashPhase": true,
    "dashSpeed": 14,
    "maxDashDistance": 12,
    "targetStopDistance": 1.5,
    "attackEndAnimStateName": "Skill_Rasengan_End",
    "airAnimStateName": "Skill_Rasengan_Air",
    "airGravityOff": true,
    "airLandedAnimStateName": "Skill_Rasengan",
    "airAttackEndAnimStateName": ""
}
```

> `airAttackEndAnimStateName`을 `""`로 두면 공중 대쉬 후  
> 현재 재생 중인 `Skill_Rasengan_Air`의 End 클립이 자연스럽게 마무리됨.

---

## 4. 전체 실행 흐름 다이어그램

### 시나리오 A — 공중 차징 → 착지 → 지상 대쉬

```
[공중 스킬 발동]
  Check_Skill_Input(): isAir=true → Change_State(Skill_Rasengan_Air)
    Enter():
      - GravityEnabled(false), Y속도=0
      - Play_State("Skill_Rasengan_Air")  → Start 재생
      - _hasLanded = false

[Update_Charging - Loop 진입 후]
  매 프레임 Is_OnGround() 감지
    → true (착지!)
        GravityEnabled(true)
        Play_StateLoopOnly("Skill_Rasengan")  → Skill_Rasengan Loop 바로 재생
        _hasLanded = true

  Hold 해제
    → Request_AnimStateEnd()  ← Skill_Rasengan: Loop → End 전환
    → Begin_DashPhase()        ← 지상 대쉬 시작

[Update_Dashing]
  이동 중...
  거리 도달 or 타겟 근접
    → Begin_AttackPhase()
        _hasLanded == true
        → endStateName = attackEndAnimStateName = "Skill_Rasengan_End"
        → Play_State("Skill_Rasengan_End")

[Update_Attacking]
  Skill_Rasengan_End 끝나면
    → Change_State(Idle)
```

---

### 시나리오 B — 공중 차징 → 공중 대쉬

```
[공중 스킬 발동]
  Enter():
    - GravityEnabled(false), Y속도=0
    - Play_State("Skill_Rasengan_Air")  → Start 재생
    - _hasLanded = false

[Update_Charging - Loop 진입 후]
  Is_OnGround() == false (계속 공중)
  Hold 해제
    → Request_AnimStateEnd()  ← Skill_Rasengan_Air: Loop → End 전환 (공중 돌진 애니)
    → Begin_DashPhase()        ← 공중 대쉬 시작

[Update_Dashing]
  공중 이동 + Air End 클립 재생 중...
  거리 도달 or 타겟 근접
    → Begin_AttackPhase()
        _hasLanded == false
        → endStateName = airAttackEndAnimStateName = "" (비어있음)
        → Play_State 호출 없음 → 현재 Air End 클립 자연 유지

[Update_Attacking]
  Air End 클립 끝나면 (Non-Loop 확인 필수!)
    → Change_State(Idle)
```

---

## 5. 주의사항

### 5-1. `Skill_Rasengan_Air` End 클립 Non-Loop 확인 필수

시나리오 B에서 `Update_Attacking()`이 `Is_AnimStateFinished()`로 Idle 전환을 감지하려면  
`Skill_Rasengan_Air`의 **End 클립이 `loop = false`** 여야 한다.  
에디터에서 AnimationStateComponent 인스펙터 → `Skill_Rasengan_Air` → end 클립 → `loop: false` 확인.

---

### 5-2. `Skill_Rasengan` AnimState가 에디터에 등록되어 있어야 한다

착지 시 `Play_StateLoopOnly("Skill_Rasengan")` 호출 → `Find_State("Skill_Rasengan")`이 `nullptr`이면  
교체가 일어나지 않음.  
에디터에서 플레이어 프리팹 → AnimationStateComponent → `Skill_Rasengan` 키 존재 여부 확인.

---

### 5-3. `_channelingTimer` 리셋 여부 (선택)

착지 시 Air에서 누르고 있던 시간이 그대로 누적되므로,  
착지 직후 남은 차징 시간이 짧아질 수 있음.  
지상 차징 시간을 초기화하고 싶다면:

```cpp
// _hasLanded = true; 바로 위에 추가
_channelingTimer = 0.f; // 착지 후 타이머 리셋 (선택)
_hasLanded = true;
```

---

### 5-4. 치도리 Air 등 다른 스킬 적용 방법

코드 변경 없이 **JSON에 두 필드만 추가**하면 됨.

```json
{
    "SkillID": 1004,
    "SkillName": "Chidori",
    "attackEndAnimStateName": "Skill_Chidori_End",
    "airAnimStateName":       "Skill_Chidori_Air",
    "airGravityOff": true,
    "airLandedAnimStateName":    "Skill_Chidori",
    "airAttackEndAnimStateName": ""
}
```

---

## 6. 최종 변경 요약

```
변경 파일 5곳:

① Client/Public/Client_Struct.h
   FSkillData에 두 필드 추가:
     string airLandedAnimStateName    = "";  // 착지 시 전환 대상
     string airAttackEndAnimStateName = "";  // 공중 대쉬 후 마무리 (비어있으면 End 클립 자연 유지)

② Client/Public/PlayerState_Skill.h
   private 멤버 추가:
     bool _hasLanded = false;  // 착지 감지 플래그, Begin_AttackPhase 분기에서도 사용

③ Client/Private/PlayerState_Skill.cpp
   - Enter():
       _hasLanded = false;  (초기화)

   - Update_Charging():
       if (!_hasLanded && movement && skillData && movement->Is_OnGround())
       {
           if (!skillData->airLandedAnimStateName.empty())
           {
               movement->Set_GravityEnabled(true);
               state->Get_AnimationState()->Play_StateLoopOnly(
                   skillData->airLandedAnimStateName);
               _hasLanded = true;
           }
       }

   - Begin_AttackPhase():
       const string& endStateName = _hasLanded
           ? skillData->attackEndAnimStateName
           : skillData->airAttackEndAnimStateName;
       if (!endStateName.empty())
           state->Get_AnimationState()->Play_State(endStateName);

④ Client/Private/ResourceLoader.cpp
   Build_AllResourceJobs() Skill 파싱 블록에 두 줄 추가:
     job.skillData.airLandedAnimStateName    = item.value("airLandedAnimStateName",    string{});
     job.skillData.airAttackEndAnimStateName = item.value("airAttackEndAnimStateName", string{});

⑤ Client/Bin/Resources/Data/json/DT_SkillData.json
   Rasengan 항목에 두 필드 추가:
     "airLandedAnimStateName":    "Skill_Rasengan"
     "airAttackEndAnimStateName": ""
```
