# Start -> Loop -> End 애니메이션 시퀀스 구현 가이드라인

단순히 애니메이션 3개를 예약해서 연이어 트랙을 전환하는 것이 아닙니다. **"사용자 입력(또는 AI 상태)에 따라 특정 애니메이션(Start, End)이 능동적으로 개입하고, 평상시에는 Loop 클립이 재생되는"** 흐름을 만드는 가이드입니다. 

코드는 직접 수정하지 않고, **어느 파일의 어느 부분을 어떻게 수정해야 하는지**에 대한 상세 가이드를 제공합니다.

---

## 1. 개요 및 핵심 원리

이 흐름은 3가지 상태(Phase)를 가집니다.

- **Start (시작)**: 달리기 입력이 들어온 최초 1회 재생 (예: 튀어나가는 모션, `isLoop = false`)
- **Loop (반복)**: Start 재생이 완전히 끝난 뒤, 입력이 유지되고 있다면 자동으로 넘어가는 런(Run) 모션 (`isLoop = true`)
- **End (종료)**: 달리는 도중(혹은 Start 중간)에 키보드나 패드 입력을 뗄 때 재생. 브레이크를 걸거나 서서히 멈추는 모션 (`isLoop = false`). 이 클립 재생이 끝나야 비로소 진정한 **Idle** 상태로 돌아갑니다.

### 구조적 위치 (어디에 구현해야 하는가?)
이 논리는 FSM이나 특정 플레이어 상태에 종속되는 것이 아니라, 캐릭터/몬스터 모델 자체의 재생 방식입니다. 따라서 **`Engine`의 `Model` 클래스**에서 이 Phase를 관리하는 것이 가장 객체지향적이고 깔끔합니다. 

---

## 2. 엔진(Engine) 측 수정 가이드

`Engine/Public/Model.h`와 `Engine/Private/Model.cpp` 파일에 **시퀀스 애니메이션 재생** 기능을 추가합니다.
(기존 단일 애니메이션 재생 함수 `Set_Animation`, `Play_Animation`과 완벽히 호환되게 설계합니다)

### 2.1. Model.h 추가 사항

```cpp
// 1. 애니메이션 페이즈 열거형 선언
enum class EAnimPhase { Start, Loop, End };

public:
    // 2. 시퀀스 설정 함수
    void Set_AnimationSequence(
        const string& startAnim,   // 비어있으면 Start 생략
        const string& loopAnim,    // 메인 루프 (필수)
        const string& endAnim      // 비어있으면 End 생략
    );

    // 3. (옵션) 애니메이션 종료 요청 함수 - Loop 도중 입력을 뗐을 때 호출
    void Request_AnimEnd();

    // 4. (옵션) 현재 페이즈 조회 
    EAnimPhase Get_AnimPhase() const { return _animPhase; }

private:
    EAnimPhase  _animPhase = EAnimPhase::Start;
    string      _startAnimName = "";
    string      _loopAnimName = "";
    string      _endAnimName = "";
```

### 2.2. Model.cpp 업데이트 가이드

#### A. Set_AnimationSequence(...) 구현
```cpp
void Model::Set_AnimationSequence(const string& startAnim, const string& loopAnim, const string& endAnim)
{
    _startAnimName = startAnim;
    _loopAnimName = loopAnim;
    _endAnimName = endAnim;

    // Start 애니메이션이 존재한다면 Start부터 시작
    if (!_startAnimName.empty())
    {
        _animPhase = EAnimPhase::Start;
        Set_Animation(_startAnimName, false); // isLoop = false
    }
    else
    {
        // Start가 없으면 바로 Loop로 진입
        _animPhase = EAnimPhase::Loop;
        Set_Animation(_loopAnimName, true);   // isLoop = true
    }
}
```

#### B. 기존 Set_Animation(...) 연동 (하위 호환성 유지)
기존 `Set_Animation(name, isLoop)` 함수를 호출할 경우, 과거의 코드들이 깨지지 않아야 합니다. 기존 함수에 다음 로직을 추가하여 단일 재생으로 취급하게 만듭니다.
```cpp
// 기존 함수 내부에 이렇게 추가
_startAnimName = "";
_loopAnimName = name;
_endAnimName = "";
_animPhase = EAnimPhase::Loop;
```

#### C. Request_AnimEnd() 구현 (입력 해제 시 호출 지점)
이 함수는 클라이언트에서 "더 이상 달리지 않음"을 감지했을 때 호출합니다.
```cpp
void Model::Request_AnimEnd()
{
    // 이미 End 페이즈거나 Start/Loop 클립이 설정되지 않은 경우 무시
    if (_animPhase == EAnimPhase::End) return;

    if (!_endAnimName.empty())
    {
        _animPhase = EAnimPhase::End;
        Set_Animation(_endAnimName, false); // isLoop = false
    }
    else
    {
        // End 애니메이션이 등록 안되어 있으면 즉시 종료 처리
        // (이후 FSM 코드가 이를 감지하고 Idle로 넘길 수 있도록)
        _currentAnimationIndex = -1; // 또는 종료 플래그 활성화
    }
}
```

#### D. Play_Animation(...) 흐름 제어 추가
매 프레임 호출되는 이 함수에서 `finished` 여부를 확인하여, **Phase를 자동으로 넘겨줍니다.**
```cpp
bool Model::Play_Animation(float timeDelta)
{
    // ... [기존 코드 유지: Update_TransformationMatrices 호출 등] ...
    // 만약 Animation(재생기) 쪽에서 재생이 다 되었다고 true(finished)를 리턴한다면:
    
    if (finished) 
    {
        if (_animPhase == EAnimPhase::Start)
        {
            // Start 재생 완료 -> Loop 클립으로 자연스럽게 전환
            _animPhase = EAnimPhase::Loop;
            Set_Animation(_loopAnimName, true); // isLoop = true
            return false; // 아직 전체 시퀀스가 끝난 것이 아님
        }
        else if (_animPhase == EAnimPhase::Loop)
        {
            // Loop 상태에서는 isLoop=true면 여기 안 걸림
            // 만약 isLoop=false짜리 특수 Loop(공격 등)라면 끝나면 끝나는 것
            return true;
        }
        else if (_animPhase == EAnimPhase::End)
        {
            // End 애니메이션까지 완전히 끝남! (이제 다음 State로 갈 수 있음)
            return true;
        }
    }

    return finished;
}
```

---

## 3. 클라이언트(Client) 측 수정 가이드

이제 저 엔진의 구조를 활용해서 실제 FSM의 **Run State**에 달아줍니다.

### `PlayerState_Run.cpp` 업데이트 가이드

- **Enter()**
  - 기존에 `Apply_StateAnimation`을 부르던 곳을 변경하여,
  - `state->Get_Model()->Set_AnimationSequence("Run_Start", "Run_Loop", "Run_End");` 와 같이 호출합니다. (또는 `FStateAnimationDesc` 구조체에 `startAnimName`, `loopAnimName`, `endAnimName` 세 필드를 뚫어두는 방식을 추천합니다!)

- **Update()**
  - 입력 여부를 매 프레임 확인합니다 (`state->Get_Input()->Get_Frame()`).
  - 만약 이동키에서 손을 뗐다면 (`moveX`, `moveY` 벡터 길이가 0에 가까워졌다면):
    - **[수정 전]** 곧바로 `state->Change_State(EPlayerState::Idle);`
    - **[수정 후]** 곧바로 Idle로 보내지 않고 `state->Get_Model()->Request_AnimEnd()`를 1회만 호출합니다.
  - 그런 다음, `state->Get_Model()->Play_Animation(timeDelta)` 에서 최종적으로 `true`가 반환되는지 확인합니다. 즉, 현재 Phase가 무엇이든 간에, 모델에서 `Play_Animation() == true`가 뜰 때 비로소 `Change_State(EPlayerState::Idle)`을 부르게 만듭니다.

#### 예시 코드 (PlayerState_Run::Update)
```cpp
// 1. 이동 명령 입력 & 감속/가속
auto input = state->Get_Input();
auto movement = state->Get_Movement();
auto model = state->Get_Model();
const auto& frame = input->Get_Frame();

bool bHasMoveInput = (Vec2(frame.moveX, frame.moveY).LengthSquared() > FLT_EPSILON);

if (!bHasMoveInput)
{
    // 입력 해제 시 End 모션 시작
    // Request_AnimEnd는 내부에서 _animPhase가 이미 End이면 무시하므로 매 프레임 불려도 안전(혹은 1회 플래그 처리)
    model->Request_AnimEnd(); 
}
else
{
    // 계속 달리고 있을 때의 이동 물리 적용
    auto cmd = state->Init_MoveCommand();
    movement->Apply_Command(cmd);
}
movement->Update(timeDelta);

// 2. 애니메이션 진행 상태 확인
bool bSequenceFinished = model->Play_Animation(timeDelta); // 여기서 내부적으로 페이즈 전환됨

// 3. 만약 시퀀스가 끝났다면(End 모션까지 완료했거나, 등록된 모션이 없어 즉시 종료된 경우)
if (bSequenceFinished)
{
    if (!bHasMoveInput) {
        state->Change_State(EPlayerState::Idle);
        return;
    }
}
```

---

## 4. 엣지 케이스 (Edge Cases)

**Q1. Start 클립이 재생 중일 때 입력을 뗀다면?**
A: `Update()` 에서 입력을 뗐음을 감지하고 `Request_AnimEnd()`를 바로 부를 것입니다. 그러면 아직 Start가 끝나지 않았지만 강제로 End 클립(`_endAnimName`)으로 덮어씌워지고 전환됩니다. 원하시는 즉각적인 브레이크 반응에 알맞습니다.

**Q2. End 클립(멈추는 중)이 재생 중인데 플레이어가 다시 달리기 키를 누른다면?**
A: 다시 `Update()`에서 이동 키 입력을 감지하고, 이 경우 `PlayerState_Idle::Update`에 있거나 (End 재생 중에 Run에 남아있다면) Run State 내에서 다시 `Set_AnimationSequence`를 호출해 버리면 그만입니다. 자연스럽게 다시 Start -> Loop 흐름으로 가속하게 됩니다.
