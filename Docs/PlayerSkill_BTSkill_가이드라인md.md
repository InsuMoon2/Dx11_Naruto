## `Docs/BTSkillmd.md` 현실형 대체안
### Summary
- 방향은 유지한다. `AnimNotify`를 스킬 스폰/이펙트 타이밍의 **주력 수단**으로 쓰는 건 맞다.
- 다만 “전부 데이터만으로 해결”은 지금 레포 기준으로 과하다. 현실안은 **데이터 주도 기본 + 범용 C++ 제어 + 선택적 스킬 보조 훅 허용**이다.
- 플레이어와 몬스터는 같은 스킬을 써도 **상위 제어층은 다르게**, **하위 스폰/보조 로직은 최대한 공유**한다.
- 즉, `PlayerStateMachine`과 `BehaviorTree`를 직접 호환시키는 게 아니라, 둘 다 같은 `SkillData / AnimNotify / 선택적 RuntimeHook`를 쓰게 만든다.

### 구조 결정
1. **데이터가 맡는 것**
- `FSkillData`의 `animStateName`, `loopDurationSec`, `isHoldSkill`는 계속 사용한다.
- 스폰 타이밍, 이펙트 타이밍, 콜리전 on/off는 애니메이션 프레임의 `AnimNotify`가 맡는다.
- 기본 스킬은 “애니메이션 재생 + 노티파이 발동 + 종료 대기”만으로 끝낸다.

2. **범용 C++가 맡는 것**
- 플레이어는 기존 [PlayerState_Skill.cpp](/d:/GitDesktop/Dx11_Naruto/Client/Private/PlayerState_Skill.cpp) 를 유지하되 범용 상태로 계속 쓴다.
- 몬스터는 새 `BTTask_UseSkill` 하나로 간다.
- `Single / Sequence / Hold / End 요청`은 범용 C++가 처리한다. 이건 현재 [AnimationStateComponent.h](/d:/GitDesktop/Dx11_Naruto/Client/Public/AnimationStateComponent.h) API와도 잘 맞는다.

3. **선택적 C++ 보조가 맡는 것**
- 정말 특수한 스킬만 `RuntimeHook`를 둔다.
- 예: 차징 중 전진, 사용 중 회전 고정, 목표 추적, 반복 발사, 종료 시 추가 폭발.
- 기본 스킬은 Hook 없이 돌아가고, 복잡한 스킬만 Hook을 단다.

### 핵심 인터페이스
- 새 공용 보조 인터페이스는 `Client`에 둔다.
- 추천 파일:
  - `Client/Public/SkillRuntimeHook.h`
  - `Client/Private/SkillRuntimeHook.cpp`
  - `Client/Public/BTTask_UseSkill.h`
  - `Client/Private/BTTask_UseSkill.cpp`

```cpp
// 플레이어 FSM과 몬스터 BT가 공통으로 넘기는 스킬 실행 컨텍스트.
// 상태 전환은 담지 않고, 실행 중 필요한 참조와 시간 정보만 담는다.
struct FSkillRuntimeContext
{
    // 현재 실행 중인 스킬 ID.
    int32 skillId = 0;

    // 스킬 사용자. Player / Monster 공용.
    Shared<GameObject> owner = nullptr;

    // 현재 사용자에게 붙은 애니메이션 상태 컴포넌트.
    Shared<AnimationStateComponent> animationState = nullptr;

    // 이동 제어가 필요한 스킬을 위한 이동 컴포넌트.
    Shared<MovementComponent> movement = nullptr;

    // 몬스터 BT에서만 사용하는 블랙보드. 플레이어면 nullptr 허용.
    Shared<Blackboard> blackboard = nullptr;

    // 스킬 진입 후 누적 시간.
    float elapsedTime = 0.f;

    // Sequence 스킬에서 End 요청을 이미 보냈는지 여부.
    bool hasRequestedEnd = false;
};

// 기본은 no-op 이고, 복잡한 스킬만 이 훅을 구현한다.
// PlayerState_Skill 과 BTTask_UseSkill 이 공통으로 호출한다.
class ISkillRuntimeHook
{
public:
    virtual ~ISkillRuntimeHook() = default;

public:
    // 스킬 진입 시 1회 호출된다.
    virtual void On_Enter(FSkillRuntimeContext& context) {}

    // 스킬 진행 중 매 프레임 호출된다.
    virtual void On_Tick(FSkillRuntimeContext& context, float timeDelta) {}

    // 스킬 종료 시 1회 호출된다.
    virtual void On_Exit(FSkillRuntimeContext& context) {}
};

// skillId 기준으로 선택적 런타임 훅을 찾아준다.
// 기본 스킬은 nullptr 를 반환하고, 특수 스킬만 구현체를 연결한다.
class SkillRuntimeHookRegistry : public Base
{
    DECLARE_SINGLETON(SkillRuntimeHookRegistry)

public:
    const ISkillRuntimeHook* Find_Hook(int32 skillId) const;

private:
    // 스킬별 선택적 런타임 훅 테이블.
    umap<int32, Shared<ISkillRuntimeHook>> _hookMap;
};
```

### 플레이어 FSM 쪽
- 현재 방향은 “스킬마다 별도 상태 클래스”가 아니라 **범용 `PlayerState_Skill` 유지**가 현실적이다.
- 단, 현재 [PlayerState_Skill.cpp:81](/d:/GitDesktop/Dx11_Naruto/Client/Private/PlayerState_Skill.cpp:81) 는 `Single`인데 `Is_AnimSequenceFinished()`를 보고 있어서 이건 현실안에서 반드시 고친다.
- 플레이어는 입력 해제와 홀드 종료를 C++가 맡고, 실제 생성은 `AN_SpawnSkill`가 맡는다.

```cpp
// 범용 플레이어 스킬 상태.
// 대부분의 스킬은 이 클래스 하나로 처리하고,
// 특수 동작만 RuntimeHook 으로 보강한다.
class PlayerState_Skill : public IPlayerState
{
public:
    explicit PlayerState_Skill(EPlayerState stateId, int32 skill_Id);

protected:
    // 현재 스킬 실행용 공통 컨텍스트.
    FSkillRuntimeContext _runtimeContext;

    // 현재 스킬이 Single / Sequence 인지 캐싱한다.
    EStateAnimationMode _animMode = EStateAnimationMode::Single;

    // Sequence loop 구간 누적 시간.
    float _channelingTimer = 0.f;

    // End 요청을 이미 보냈는지 기록한다.
    bool _isEnding = false;

    // 현재 상태가 담당하는 스킬 ID.
    int32 _mySkill_Id = 0;

protected:
    // Enter 시 공통 컨텍스트를 구성하고 Hook 진입을 호출한다.
    void Setup_RuntimeContext(PlayerStateMachine* state);

    // Single 스킬 종료를 처리한다.
    void Update_SingleSkill(PlayerStateMachine* state);

    // Sequence / Hold 스킬 종료를 처리한다.
    void Update_SequenceSkill(PlayerStateMachine* state, float timeDelta);
};
```

```cpp
void PlayerState_Skill::Setup_RuntimeContext(PlayerStateMachine* state)
{
    _runtimeContext = {};
    _runtimeContext.skillId = _mySkill_Id;
    _runtimeContext.owner = state ? state->Get_Owner() : nullptr;
    _runtimeContext.animationState = state ? state->Get_AnimationState() : nullptr;
    _runtimeContext.movement = state ? state->Get_Movement() : nullptr;

    if (const ISkillRuntimeHook* hook = GET_SINGLE(SkillRuntimeHookRegistry)->Find_Hook(_mySkill_Id))
        const_cast<ISkillRuntimeHook*>(hook)->On_Enter(_runtimeContext);
}

void PlayerState_Skill::Update_SingleSkill(PlayerStateMachine* state)
{
    // Single 상태는 단일 클립 종료만 보면 된다.
    if (state->Is_AnimStateFinished())
        state->Change_State(Get_NextState());
}

void PlayerState_Skill::Update_SequenceSkill(PlayerStateMachine* state, float timeDelta)
{
    const FSkillData* skillData = GET_SINGLE(SkillDataManager)->Get_SkillData(_mySkill_Id);
    const float maxDuration = skillData ? skillData->loopDurationSec : 0.f;
    const bool isHoldSkill = skillData ? skillData->isHoldSkill : false;

    if (const ISkillRuntimeHook* hook = GET_SINGLE(SkillRuntimeHookRegistry)->Find_Hook(_mySkill_Id))
        const_cast<ISkillRuntimeHook*>(hook)->On_Tick(_runtimeContext, timeDelta);

    if (state->Get_AnimPhase() == EAnimPhase::Loop && !_isEnding)
    {
        _channelingTimer += timeDelta;

        bool shouldEnd = false;

        if (isHoldSkill)
        {
            auto input = state->Get_Input();
            const int32 slot = state->Get_ActiveSkillSlot();

            // 플레이어 홀드 스킬은 버튼 해제 또는 최대 시간 도달 시 종료한다.
            if (!input || !input->Get_Frame().useSkillPress[slot] || _channelingTimer >= maxDuration)
                shouldEnd = true;
        }
        else if (_channelingTimer >= maxDuration)
        {
            shouldEnd = true;
        }

        if (shouldEnd)
        {
            state->Request_AnimStateEnd();
            _isEnding = true;
            _runtimeContext.hasRequestedEnd = true;
        }
    }

    if (state->Is_AnimSequenceFinished())
        state->Change_State(Get_NextState());
}
```

### 몬스터 BT 쪽
- 몬스터는 `BTTask_UseSkill` 하나로 통일한다.
- 이 태스크는 `AnimState`를 블랙보드에 쓰고, 필요하면 `AnimRequestEnd`를 켠다.
- 엔진이 아니라 `Client`에 두고, 시작 시 `GAME->Register_BTNode()`로 등록한다.
- 등록 위치는 [MainApp.cpp](/d:/GitDesktop/Dx11_Naruto/Client/Private/MainApp.cpp) 초기화 직후가 가장 현실적이다.

```cpp
// 몬스터가 범용적으로 스킬을 쓰는 BT 태스크.
// 애니메이션 시작, Sequence 종료 요청, 완료 판정만 담당한다.
class BTTask_UseSkill : public BTTask
{
    GENERATED_BT_REFLECTION(BTTask_UseSkill)

public:
    explicit BTTask_UseSkill();
    explicit BTTask_UseSkill(const BTTask_UseSkill& rhs);
    virtual ~BTTask_UseSkill() = default;

public:
    void Initialize() override;
    EBTNodeResult Update(float timeDelta) override;
    void OnTerminate(EBTNodeResult result) override;

private:
    // 이 태스크가 실행할 스킬 ID.
    int32 _skillId = 0;

    // 실제 재생할 애니메이션 상태 이름.
    // 비어 있으면 SkillData.animStateName 을 사용한다.
    string _animStateName = "";

    // 조기 종료를 BT가 줄 때 사용할 선택적 블랙보드 키.
    string _cancelKey = "";

    // 현재 실행용 공통 컨텍스트.
    FSkillRuntimeContext _runtimeContext;
};
```

```cpp
void BTTask_UseSkill::Initialize()
{
    BTTask::Initialize();

    auto owner = _owner.lock();
    auto blackboard = _blackboard.lock();
    if (!owner || !blackboard)
        return;

    const FSkillData* skillData = GET_SINGLE(SkillDataManager)->Get_SkillData(_skillId);

    if (_animStateName.empty() && skillData)
        _animStateName = skillData->animStateName;

    _runtimeContext = {};
    _runtimeContext.skillId = _skillId;
    _runtimeContext.owner = owner;
    _runtimeContext.blackboard = blackboard;
    _runtimeContext.animationState = owner->Get_Component<AnimationStateComponent>();
    _runtimeContext.movement = owner->Get_Component<MovementComponent>();

    // 몬스터는 블랙보드 경유로 애니메이션을 재생한다.
    blackboard->Set_ValueAsString("AnimState", _animStateName);

    if (const ISkillRuntimeHook* hook = GET_SINGLE(SkillRuntimeHookRegistry)->Find_Hook(_skillId))
        const_cast<ISkillRuntimeHook*>(hook)->On_Enter(_runtimeContext);
}

EBTNodeResult BTTask_UseSkill::Update(float timeDelta)
{
    auto owner = _owner.lock();
    auto blackboard = _blackboard.lock();
    auto anim = owner ? owner->Get_Component<AnimationStateComponent>() : nullptr;
    if (!owner || !blackboard || !anim)
        return EBTNodeResult::Failed;

    const FSkillData* skillData = GET_SINGLE(SkillDataManager)->Get_SkillData(_skillId);
    const FStateAnimationDesc* stateDesc = anim->Find_State(_animStateName);
    const EStateAnimationMode mode = stateDesc ? stateDesc->mode : EStateAnimationMode::Single;

    _runtimeContext.elapsedTime += timeDelta;

    if (const ISkillRuntimeHook* hook = GET_SINGLE(SkillRuntimeHookRegistry)->Find_Hook(_skillId))
        const_cast<ISkillRuntimeHook*>(hook)->On_Tick(_runtimeContext, timeDelta);

    if (mode == EStateAnimationMode::Sequence && !_runtimeContext.hasRequestedEnd)
    {
        bool shouldEnd = false;

        // 몬스터는 기본적으로 최대 loopDurationSec 에 도달하면 종료 요청한다.
        if (skillData && skillData->loopDurationSec > 0.f && _runtimeContext.elapsedTime >= skillData->loopDurationSec)
            shouldEnd = true;

        // 필요하면 BT가 cancelKey 로 조기 종료를 줄 수 있다.
        if (!shouldEnd && !_cancelKey.empty() && blackboard->HasKey(_cancelKey) && blackboard->Get_ValueAsBool(_cancelKey))
            shouldEnd = true;

        if (shouldEnd)
        {
            blackboard->Set_ValueAsBool("AnimRequestEnd", true);
            _runtimeContext.hasRequestedEnd = true;
        }

        if (anim->Is_CurrentStateSequenceFinished())
            return EBTNodeResult::Succeeded;

        return EBTNodeResult::InProgress;
    }

    if (anim->Is_CurrentStateFinished())
        return EBTNodeResult::Succeeded;

    return EBTNodeResult::InProgress;
}
```

```cpp
// Client 초기화 시점에 게임 전용 BT 노드를 등록한다.
// Engine 기본 노드와 분리되므로 게임 로직이 Engine 으로 새지 않는다.
GAME->Register_BTNode("Task", "Task_UseSkill", []()
{
    return BTTask_UseSkill::Create();
});
```

### `AN_SpawnSkill` 현실화
- 현재 [AN_SpawnSkill.cpp](/d:/GitDesktop/Dx11_Naruto/Client/Private/AN_SpawnSkill.cpp) 는 `owner transform + local offset`만 지원한다.
- 현실안에서는 이걸 유지하되, **본/소켓 기준 스폰**을 추가한다.
- 이건 “데이터 주도 강화”이면서도 C++ 보조가 최소인 좋은 확장이다.

```cpp
class AN_SpawnSkill : public AnimNotify
{
    GENERATED_BODY(AN_SpawnSkill)

private:
    // 생성할 실제 스킬 오브젝트 타입.
    Protocol::OBJECT_TYPE _spawnObjectType = Protocol::OBJECT_TYPE_SKILL_RASENSHURIKEN;

    // 스폰된 스킬이 참조할 스킬 ID.
    int32 _skill_Id = 0;

    // 본 이름이 비어 있으면 owner Transform 기준으로 스폰한다.
    string _sourceBoneName = "";

    // 본/소켓 기준 로컬 오프셋.
    Vec3 _localOffset = Vec3(0.f, 1.2f, 1.8f);

    // 실제 생성될 레이어 태그.
    string _layerTag = "Layer_SkillObject";

    // owner 전방을 발사 방향으로 쓸지 여부.
    bool _useOwnerForward = true;

private:
    // 본/소켓이 있으면 그 기준, 없으면 owner Transform 기준으로 월드 위치를 계산한다.
    static Vec3 Calculate_WorldSpawnPosition(Shared<GameObject> owner, const string& sourceBoneName, const Vec3& localOffset);
};
```

```cpp
Vec3 AN_SpawnSkill::Calculate_WorldSpawnPosition(Shared<GameObject> owner, const string& sourceBoneName, const Vec3& localOffset)
{
    auto transform = owner ? owner->Get_Component<Transform>() : nullptr;
    if (!transform)
        return Vec3::Zero;

    auto model = owner->Get_Component<Model>();
    if (model && !sourceBoneName.empty())
    {
        if (const Matrix* socketMatrix = model->Get_SocketBoneMatrixPtr(sourceBoneName))
        {
            const Matrix world = (*socketMatrix) * transform->Get_WorldMatrix();
            const Vec3 socketPos = world.Translation();

            return socketPos
                + transform->Get_WorldRight() * localOffset.x
                + transform->Get_WorldUp() * localOffset.y
                + transform->Get_WorldForward() * localOffset.z;
        }
    }

    return transform->Get_WorldPosition()
        + transform->Get_WorldRight() * localOffset.x
        + transform->Get_WorldUp() * localOffset.y
        + transform->Get_WorldForward() * localOffset.z;
}
```

### 구현 원칙
- 기본 스킬:
  - `SkillData`에서 애니메이션 이름 읽음
  - `PlayerState_Skill` 또는 `BTTask_UseSkill`이 재생 시작
  - `AN_SpawnSkill`이 생성 담당
  - 종료는 `Single / Sequence` 규칙대로 C++가 처리
- 특수 스킬:
  - 위 흐름은 유지
  - 필요한 부분만 `RuntimeHook`로 추가
- 금지할 것:
  - 모든 스킬마다 `PlayerState_Skill_XXX`, `BTTask_UseXXX`, `SkillAction_XXX`를 무조건 늘리는 방식
  - 반대로 “무조건 데이터만으로 해결”하려고 해서 종료/차징/추적 로직까지 노티파이에 우겨넣는 방식

### Test Plan
- 플레이어 `Single` 스킬:
  - 입력 시 범용 `PlayerState_Skill` 진입
  - `AN_SpawnSkill` 1회 발동
  - `Is_AnimStateFinished()` 후 `Idle` 복귀
- 플레이어 `Sequence / Hold` 스킬:
  - `Loop` 진입 후 `loopDurationSec` 또는 버튼 해제로 `Request_AnimStateEnd()`
  - `Is_AnimSequenceFinished()` 후 `Idle` 복귀
- 몬스터 스킬:
  - `BTTask_UseSkill`가 `AnimState` 세팅
  - `loopDurationSec` 도달 시 `AnimRequestEnd = true`
  - `Is_CurrentStateSequenceFinished()`면 `Succeeded`
- 본/소켓 스폰:
  - `sourceBoneName = "R_Hand_Weapon_cnt_tr"` 같은 값으로 손 위치 스폰 확인
  - 본 이름이 비어 있으면 기존 owner Transform fallback 확인
- 특수 스킬 Hook:
  - Hook 없는 스킬은 기존 흐름 그대로 동작
  - Hook 있는 스킬만 추가 이동/회전/추적 로직이 들어가는지 확인

### Assumptions / Defaults
- 기본 정책은 `AnimNotify 중심`이다. 다만 종료 제어와 특수 기믹은 범용 C++ 보조를 허용한다.
- `PlayerState_Skill`는 유지한다. 다만 “스킬마다 별도 상태 클래스”는 기본 전략이 아니다.
- `BTTask_UseSkill`는 `Client`에 두고 `GAME->Register_BTNode()`로 등록한다.
- 모든 스킬을 100% 데이터로 밀어붙이지 않는다. 복잡한 스킬은 `RuntimeHook`를 쓰는 것을 정식 허용한다.
- 문서에서는 `ANS_SpawnSkill`가 아니라 실제 구현명인 `AN_SpawnSkill`로 통일한다.
