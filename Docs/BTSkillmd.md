## 플레이어 FSM / 몬스터 BT 공용 스킬 구조 플랜 (Data-Driven & AnimNotify 중심)

### Summary
- 핵심은 **기존의 `SkillAction_*` 형태의 하드코딩된 중간 제어/타이머 클래스를 완전히 제거**하는 것입니다.
- C++ 로직이 스킬의 타이밍이나 스폰을 통제하지 않으며, 모든 타이밍과 오프셋 정보는 애니메이션 프레임에 박힌 **`AnimNotify_SpawnSkill`**에 온전히 위임합니다.
- 상위 제어층(`Player FSM`, `Monster BT`)은 그저 스킬명과 일치하는 애니메이션을 재생(또는 블랙보드에 상태명만 전달)하고, 종료를 대기하는 단순한 "명령하달자" 역할만 수행합니다.
- 궁극적으로 **스킬 개수가 늘어나도 C++ 껍데기 클래스가 단 하나도 추가되지 않는 완벽한 데이터 주도(Data-Driven) 아키텍처**를 달성합니다.

---

### 구조 결정 (Architecture)

1. **공용 스폰 주체 (AnimNotify)**
   - **`ANS_SpawnSkill` / `ANS_PlayEffect`** 등 기존에 구현된 노티파이 시스템을 사용합니다.
   - 나선수리검 애니메이션 등 스폰 타겟 프레임에 도달하면 콜백이 터져 게임엔진(`GameObject_Factory`나 `SkillDataManager`)에 스폰을 요청합니다.
   
2. **플레이어 제어층 (Player FSM)**
   - `PlayerState_CastSkill_Generic` 같은 공용 상태를 운용합니다.
   - 키보드 입력이 들어오면 `EPlayerState::Skill` 상태로 전이되며, 어떤 스킬인지에 대한 데이터(콤보 매니저 또는 스킬 매니저)를 참고하여 애니메이션만 재생합니다.
   
3. **몬스터 제어층 (Monster BT)**
   - `BTTask_UseSkill` 등 범용 태스크 1개만 운용합니다.
   - 스킬에 해당하는 `AnimState` 문자열을 블랙보드에 기록하여 애니메이션 재생 상태로만 전환합니다.
   
4. **결과 처리 (공용 결과 반환)**
   - 상위 FSM과 BT는 중간 과정을 모니터링하지 않고 오직 **"애니메이션이 종료되었는가(`Is_CurrentStateFinished`)"** 만을 확인하여 `Idle` 복귀나 `Succeeded` 결과를 뱉습니다.

---

### 핵심 동작 원리 (Flow)

기존 FSM/BT가 스킬 시간(1.2f 등)을 재던 것과 달리, **오직 애니메이션의 재생률만 확인**합니다. FSM과 BT의 로직이 완전히 동일해지며, 결합도가 제거됩니다.

```cpp
// 1. [상위 레이어] 플레이어 FSM이나 몬스터 BT 노드에서 "스킬 사용"을 명령함
void BTTask_UseSkill::Initialize()
{
    // 스킬에 맞는 애니메이션 상태 문자열 설정 (Monster용)
    // 예: "Skill_Rasenshuriken"
    _blackboard.lock()->Set_ValueAsString("AnimState", _skillAnimName);
}

// 2. [애니메이션 레이어] AnimationStateComponent 진행 중... 목표 프레임 도달
// 모델의 프레임 정보에 따라 ANS_SpawnSkill 노티파이가 자동으로 호출됨
void ANS_SpawnSkill::Notify(...)
{
    // 3. [스킬 레이어] 
    // 나선환, 나선수리검 등 미리 정의된 스킬 프리팹/객체를 생성 요청
    // 주인의 트랜스폼(손 본 뼈다귀 등)을 부모로 두도록 설정
    GAME->Create_SkillObject(skillName, owner);
}

// 4. [상위 레이어] 애니메이션이 끝날 때까지 대기
EBTNodeResult BTTask_UseSkill::Update(float timeDelta)
{
    if (owner->Get_AnimationState()->Is_CurrentStateFinished())
        return EBTNodeResult::Succeeded;
        
    return EBTNodeResult::InProgress;
}
```

---

### 플레이어 FSM 및 무기 스킬 연동 (MyPlayer)

플레이어는 `PlayerState_Attack` 혹은 범용 `PlayerState_Skill` 하나에서 로직이 파생되어야 합니다. 수많은 `PlayerState_Skill_XXX` 클래스들을 만들지 마십시오.

```cpp
// [PlayerState_Skill.cpp]
void PlayerState_Skill::Enter(PlayerStateMachine* state)
{
    // 1. 플레이어가 장착 중인 스킬이나 발동된 스킬 아이디 확인
    const FSkillData* skillData = _player->Get_CurrentSkillData();

    // 2. 해당 스킬과 연결된 애니메이션을 즉시 Play
    state->Get_AnimationState()->Play_Animation(skillData->animStateName);
}

void PlayerState_Skill::Update(PlayerStateMachine* state, float timeDelta)
{
    // 3. 콤보나 스킬 종료 판정은 AnimationComponent에게만 묻습니다.
    if (state->Get_AnimationState()->Is_CurrentStateFinished())
    {
        state->ChangeState(EPlayerState::Idle);
    }
}
```

---

### 몬스터 BT 연동 (AIController / BT)

몬스터 쪽은 더 직관적입니다. 플레이어보다 로직이 더욱 얇아야 하며 블랙보드 경유 방식과 완벽히 호환됩니다.

```cpp
// [BTTask_UseSkill 범용 태스크]
// 기존 BTTask_UseRasengan 같이 개별적으로 파지 않고 범용 클래스만 씁니다.
void BTTask_UseSkill::Initialize()
{
    BTTask::Initialize();
    auto owner = _owner.lock();
    auto blackboard = _blackboard.lock();
    
    // 이 노드가 보유한 범용 스킬명 변수(_animStateName)를 블랙보드에 세팅
    blackboard->Set_ValueAsString("AnimState", _animStateName);
}

EBTNodeResult BTTask_UseSkill::Update(float timeDelta)
{
    auto owner = _owner.lock();
    
    // 블랙보드의 AnimRequestEnd가 True로 바뀌었거나 
    // 애니메이션이 자체 종료되었을 경우 즉각 성공을 알리고 종료
    if (owner->Get_Component<AnimationStateComponent>()->Is_CurrentStateFinished())
        return EBTNodeResult::Succeeded;

    return EBTNodeResult::InProgress;
}
```

---

### 구현 변경 포인트 (마이그레이션)

1. **클래스 싹 다 지우기 (Cleanup)**
   - `SkillAction_Rasengan` 또는 이를 위한 컨텍스트 구조체들 전면 폐기.
   - `PlayerState_Skill_Rasengan`, `BTTask_UseRasengan` 같은 스킬 전용 제어 클래스 폐기.
2. **범용 클래스로 통폐합**
   - `PlayerState_CastSkill`, `BTTask_CastSkill` 도입.
3. **노티파이 점검 (AnimNotify_SpawnSkill)**
   - 이미 존재하는 노티파이가 다중 인자(스킬명, 스폰 위치 뼈 이름 등)를 잘 받아올 수 있도록 확인.
4. **데이터베이스 점검 (SkillDataManager / FSkillData)**
   - 파싱 중인 JSON 등에 "어떤 애니메이션을 틀어줄까"에 대한 이름 리스트가 제대로 들어가 있는지 검증.

---

### Test Plan
- **플레이어 스킬 스폰 검증:**
  - 플레이어가 나선수리검 입력 시 `PlayerState_CastSkill` 진입 $\rightarrow$ 나선수리검 모션 재생 $\rightarrow$ 손 위치에 나선수리검 노티파이 정상 발생 확인.
  - 모션이 완전히 끝나면 스무스하게 `Idle` 로 복귀.
- **몬스터 스킬 스폰 검증:**
  - BT 노드에서 `BTTask_UseSkill`("Skill_Rasenshuriken") 발동 $\rightarrow$ 동일하게 노티파이 재생되어 오브젝트 스폰 확인.
  - 타겟에게 대미지가 제대로 전달되며 `Succeeded` 반환 후 다음 BT 흐름으로 연결.
- **아키텍처 부하 검사:**
  - 새 스킬(예: 치도리)을 추가할 때, C++ 코드를 한 줄도 치지 않고 JSON/애니메이션 세팅만으로 "치도리가 나가는가?" (완벽한 데이터 주도 증명).

---

### Assumptions / Defaults
- **모든 스킬 생성은 `AnimNotify`가 전담한다.** (단 하나 예외 없이 이 규칙을 준수한다.)
- 스폰 위치의 기준 오프셋 행렬이나 파라미터는 노티파이나 `SkillData.json`에서 함께 묶어서 들고 있다.
- 스킬 자체 동작 기믹(투사체 운동 등)은 **스폰된 스킬 객체 자체의 Update**가 수행하며, 애니메이션은 이를 간섭하지 않고 생성까지만 책임진다.
