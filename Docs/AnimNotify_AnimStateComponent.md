# AnimNotify Runtime + Editor Integration Guide
이 문서는 현재 `Dx11_Naruto` 최신 코드 기준으로, 아래 범위를 내일 바로 이어칠 수 있게 정리한 저장용 가이드다.

대상 범위:
- `Engine/Model`에 AnimNotify lazy load + runtime evaluation 추가
- `Editor/Prefab_View`에 Animation View 진입점 추가
- `Editor/AnimationState_Inspector` 구현
- `Editor` 초기화/등록 연결

중요 원칙:
- 없는 멤버를 가정하지 않는다.
- 이미 존재하는 코드는 “변경”이라고 쓰지 않는다.
- 코드에는 수정 의도를 설명하는 주석을 넣는다.
- 이번 문서는 `현재 레포 최신 코드` 기준이다.

---

## 1. Model에 AnimNotify 런타임 연결

### 1-1. 파일
- [Model.h](/c:/Users/moon/Desktop/Jusin/GitDesktop/Dx11_Naruto/Engine/Public/Model.h)
- [Model.cpp](/c:/Users/moon/Desktop/Jusin/GitDesktop/Dx11_Naruto/Engine/Private/Model.cpp)

---

### 1-2. `Model.h` 수정

기존 include:
```cpp
#include "Component.h"
```

바로 아래에 추가:
```cpp
#include "AnimNotify_Types.h"   // [추가] AnimNotify 런타임 데이터/컨텍스트/활성 state 타입
```

기존 선언:
```cpp
bool            Play_Animation(float timeDelta);
```

아래처럼 바꾼다:
```cpp
    // [변경] 기존 시그니처는 유지하고, Notify 실행 제어용 오버로드를 추가한다.
    bool            Play_Animation(float timeDelta);
    bool            Play_Animation(float timeDelta, bool executeNotifies);

    // [추가] 현재 재생 중인 애니메이션 이름 반환
    const string&   Get_CurrentAnimationName() const;

    // [추가] Editor / Notify asset lookup 용
    const string&   Get_ModelGuid() const { return _modelGuid; }
```

`private` 함수 선언부에 추가:
```cpp
private:
    // [추가] Notify asset을 필요할 때만 로드하는 lazy-load helper
    bool    Ensure_AnimNotifyAssetLoaded();
    void    Invalidate_AnimNotifyAsset();
    const FAnimNotifyClipData* Find_CurrentNotifyClip() const;

    // [추가] 활성화된 NotifyState 정리
    void    Stop_AllNotifyStates(bool executeEndCallback);

    // [추가] 단일 시점 Notify 평가
    void    Evaluate_AnimNotifyEvents(
        const FAnimNotifyClipData& clipData,
        const FAnimNotifyContext& context,
        bool executeNotifies) const;

    // [추가] 구간형 NotifyState 평가
    void    Evaluate_AnimNotifyStates(
        const FAnimNotifyClipData& clipData,
        const FAnimNotifyContext& context,
        bool executeNotifies);
```

멤버 변수에 추가:
```cpp
private:
    string                          _modelGuid = "";

    // [추가] 모델별 Notify asset 캐시
    bool                            _isAnimNotifyAssetLoaded = false;
    FAnimNotifyAsset                _animNotifyAsset;
    vector<FActiveAnimNotifyState>  _activeNotifyStates;
```

---

### 1-3. `Model.cpp` include 추가

상단 include에 추가:
```cpp
#include "AnimNotify_Serializer.h"   // [추가] Notify asset 파일 로드/clip 조회
#include "AnimNotify.h"              // [추가] 단일 Notify 실행
#include "AnimNotifyState.h"         // [추가] State Begin/Tick/End 실행
```

---

### 1-4. `Model.cpp` helper 추가

include 아래, 생성자 위에 추가:
```cpp
// [추가] 일반 구간 / loop wrap 구간 모두 지원하는 시간 통과 판정
static bool Is_NotifyTimeInRange(float targetTime, float previousTime, float currentTime, bool wrapped)
{
    if (!wrapped)
        return targetTime >= previousTime && targetTime <= currentTime;

    return targetTime >= previousTime || targetTime <= currentTime;
}

// [추가] 특정 시점이 NotifyState 구간 안인지 판정
static bool Is_NotifyStateActiveAtTime(float currentTime, float startTime, float duration)
{
    const float clampedDuration = Utils::Max(duration, 0.f);
    const float endTime = startTime + clampedDuration;

    return currentTime >= startTime && currentTime < endTime;
}

// [추가] active state 테이블에서 동일 state index 찾기
static int32 Find_ActiveNotifyStateIndex(const vector<Engine::FActiveAnimNotifyState>& activeStates, int32 stateIndex)
{
    for (int32 i = 0; i < static_cast<int32>(activeStates.size()); ++i)
    {
        if (activeStates[i].stateIndex == stateIndex)
            return i;
    }

    return -1;
}
```

---

### 1-5. 복사 생성자 initializer 보강

현재 복사 생성자 initializer list에 아래 3줄 추가:
```cpp
    , _isAnimNotifyAssetLoaded(false)   // [추가] clone 시 Notify lazy-load 상태는 새로 시작
    , _animNotifyAsset{}
    , _activeNotifyStates{}
```

예시:
```cpp
Model::Model(const Model& rhs)
    : Component(rhs)
    , _modelType(rhs._modelType)
    , _preLocalTransformMatrix(rhs._preLocalTransformMatrix)
    , _numMeshes(rhs._numMeshes)
    , _meshes(rhs._meshes)
    , _numMaterials(rhs._numMaterials)
    , _materials(rhs._materials)
    , _currentAnimationIndex(-1)
    , _isAnimationLoop(false)
    , _modelGuid(rhs._modelGuid)
    , _isAnimNotifyAssetLoaded(false)   // [추가]
    , _animNotifyAsset{}                // [추가]
    , _activeNotifyStates{}             // [추가]
    , _animationPlayRate(1.f)
    , _animationBlendDuration(rhs._animationBlendDuration)
```

---

### 1-6. `Model.cpp` 새 함수 추가

`Find_AnimationIndex_ByName()` 아래에 추가:

```cpp
const string& Model::Get_CurrentAnimationName() const
{
    // [추가] 현재 clip이 유효하지 않으면 빈 문자열 반환
    static const string empty = "";

    if (!_currentClip.Is_Valid())
        return empty;

    if (_currentClip.animIndex < 0 || _currentClip.animIndex >= static_cast<int32>(_animations.size()))
        return empty;

    return Get_AnimationName(static_cast<uint32>(_currentClip.animIndex));
}

void Model::Invalidate_AnimNotifyAsset()
{
    // [추가] 모델 GUID 변경 또는 재로딩 필요 시 Notify cache를 비운다.
    _isAnimNotifyAssetLoaded = false;
    _animNotifyAsset = {};
    _activeNotifyStates.clear();
}

bool Model::Ensure_AnimNotifyAssetLoaded()
{
    // [추가] 한 번 lazy-load 판단 후 프레임마다 파일 IO 하지 않도록 막는다.
    if (_isAnimNotifyAssetLoaded)
        return true;

    _animNotifyAsset = {};
    _animNotifyAsset.modelGuid = _modelGuid;
    _isAnimNotifyAssetLoaded = true;

    if (_modelGuid.empty())
        return false;

    const fs::path filePath = AnimNotify_Serializer::Get_ModelNotifyFilePath(_modelGuid);
    if (!fs::exists(filePath))
        return false;

    return AnimNotify_Serializer::Load_FromFile(filePath.wstring(), _animNotifyAsset);
}

const FAnimNotifyClipData* Model::Find_CurrentNotifyClip() const
{
    // [추가] 현재 재생 중인 애니메이션 이름으로 clip notify track을 찾는다.
    if (_modelGuid.empty())
        return nullptr;

    return AnimNotify_Serializer::Find_Clip(_animNotifyAsset, Get_CurrentAnimationName());
}

void Model::Stop_AllNotifyStates(bool executeEndCallback)
{
    // [추가] clip 종료 / clip 전환 / sequence 종료 시 활성 state 정리
    if (_activeNotifyStates.empty())
        return;

    if (executeEndCallback)
    {
        FAnimNotifyContext context;
        context.owner = Get_Owner().get();
        context.model = this;
        context.modelGuid = _modelGuid;
        context.clipName = Get_CurrentAnimationName();
        context.currentTimeSec = _currentClip.trackPosition;

        for (auto& active : _activeNotifyStates)
        {
            if (active.notifyState)
                active.notifyState->On_End(context);
        }
    }

    _activeNotifyStates.clear();
}

void Model::Evaluate_AnimNotifyEvents(
    const FAnimNotifyClipData& clipData,
    const FAnimNotifyContext& context,
    bool executeNotifies) const
{
    // [추가] preview 모드에서는 gameplay side effect를 막기 위해 Execute를 생략한다.
    if (!executeNotifies)
        return;

    for (const auto& entry : clipData.notifies)
    {
        if (!entry.notify)
            continue;

        if (!Is_NotifyTimeInRange(entry.timeSec, context.previousTimeSec, context.currentTimeSec, context.wrapped))
            continue;

        entry.notify->Execute(context);
    }
}

void Model::Evaluate_AnimNotifyStates(
    const FAnimNotifyClipData& clipData,
    const FAnimNotifyContext& context,
    bool executeNotifies)
{
    for (int32 stateIndex = 0; stateIndex < static_cast<int32>(clipData.notifyStates.size()); ++stateIndex)
    {
        const auto& entry = clipData.notifyStates[stateIndex];
        if (!entry.notifyState)
            continue;

        const float durationSec = Utils::Max(entry.durationSec, 0.f);
        if (durationSec <= 0.f)
            continue;

        const float startSec = entry.startSec;
        const float endSec = startSec + durationSec;

        const bool crossedStart = Is_NotifyTimeInRange(
            startSec,
            context.previousTimeSec,
            context.currentTimeSec,
            context.wrapped);

        const bool isActiveNow = Is_NotifyStateActiveAtTime(
            context.currentTimeSec,
            startSec,
            durationSec);

        int32 activeIndex = Find_ActiveNotifyStateIndex(_activeNotifyStates, stateIndex);

        // [추가] 시작 지점을 통과했고 아직 active가 아니면 Begin
        if (activeIndex < 0 && crossedStart)
        {
            FActiveAnimNotifyState activeState;
            activeState.stateIndex = stateIndex;
            activeState.startSec = startSec;
            activeState.endSec = endSec;
            activeState.notifyState = entry.notifyState;

            _activeNotifyStates.push_back(activeState);
            activeIndex = static_cast<int32>(_activeNotifyStates.size()) - 1;

            if (executeNotifies)
                entry.notifyState->On_Begin(context);
        }

        if (activeIndex < 0)
            continue;

        // [추가] 현재 프레임도 state 내부면 Tick
        if (isActiveNow)
        {
            if (executeNotifies && _activeNotifyStates[activeIndex].notifyState)
                _activeNotifyStates[activeIndex].notifyState->On_Tick(context);

            continue;
        }

        // [추가] state 범위를 벗어나면 End 후 active 제거
        if (executeNotifies && _activeNotifyStates[activeIndex].notifyState)
            _activeNotifyStates[activeIndex].notifyState->On_End(context);

        _activeNotifyStates.erase(_activeNotifyStates.begin() + activeIndex);
    }
}
```

---

### 1-7. `Play_Animation` 오버로드 추가

기존 `bool Model::Play_Animation(float timeDelta)` 전체를 아래로 교체:

```cpp
bool Model::Play_Animation(float timeDelta)
{
    // [변경] 기존 호출부 호환 유지
    return Play_Animation(timeDelta, true);
}

bool Model::Play_Animation(float timeDelta, bool executeNotifies)
{
    if (!_currentClip.Is_Valid())
    {
        // [추가] 현재 clip이 없으면 active state도 같이 정리
        Stop_AllNotifyStates(executeEndCallback);

        if (!_lastAppliedPose.empty())
        {
            Apply_LocalPoses_ToBones(_lastAppliedPose);
            Update_BoneMatrices_FromBones();
        }

        return false;
    }

    for (auto& bone : _bones)
    {
        if (bone)
            bone->Reset_ToNodeTransform();
    }

    bool currentFinished = false;

    // [추가] 이번 프레임 Notify 평가용 이전 위치 저장
    const float previousTrackPosition = _currentClip.trackPosition;

    if (_currentClip.animIndex >= 0 &&
        _currentClip.animIndex < static_cast<int32>(_animations.size()) &&
        _animations[_currentClip.animIndex] != nullptr)
    {
        currentFinished = _animations[_currentClip.animIndex]->Advance_TrackPosition(
            timeDelta * _currentClip.playRate,
            _currentClip.loop,
            _currentClip.trackPosition);

        Sample_ClipPose(_currentClip, _currentSamplePose);
    }

    // [추가] loop clip wrap 여부 계산
    const bool wrapped = (_currentClip.loop && _currentClip.trackPosition < previousTrackPosition);

    if (_blendState.active)
    {
        _isCurrentAnimationFinished = false;

        if (_blendState.next.animIndex >= 0 &&
            _blendState.next.animIndex < static_cast<int32>(_animations.size()) &&
            _animations[_blendState.next.animIndex] != nullptr)
        {
            _animations[_blendState.next.animIndex]->Advance_TrackPosition(
                timeDelta * _blendState.next.playRate,
                _blendState.next.loop,
                _blendState.next.trackPosition);

            Sample_ClipPose(_blendState.next, _nextSamplePose);
        }

        _blendState.elapsed += timeDelta;

        const float blendRatio = (_blendState.duration <= FLT_EPSILON)
            ? 1.f
            : Utils::Min(_blendState.elapsed / _blendState.duration, 1.f);

        Blend_LocalPoses(_blendState.fromPose, _nextSamplePose, blendRatio, _blendedPose);
        Apply_LocalPoses_ToBones(_blendedPose);
        Update_BoneMatrices_FromBones();

        _lastAppliedPose = _blendedPose;

        if (blendRatio >= 1.f)
        {
            // [추가] 실제 clip 전환 시점에 이전 active state 정리
            Stop_AllNotifyStates(executeNotifies);

            _currentClip = _blendState.next;
            Clear_BlendState();
            Sync_LegacyAnimationState();
        }

        // [가정] v1은 cross-fade 중 notify 평가 생략
        return false;
    }

    Apply_LocalPoses_ToBones(_currentSamplePose);
    Update_BoneMatrices_FromBones();

    _lastAppliedPose = _currentSamplePose;
    Sync_LegacyAnimationState();

    // [추가] pose 적용 후 Notify 평가
    if (Ensure_AnimNotifyAssetLoaded())
    {
        if (const auto* clipData = Find_CurrentNotifyClip())
        {
            FAnimNotifyContext context;
            context.owner = Get_Owner().get();
            context.model = this;
            context.modelGuid = _modelGuid;
            context.clipName = Get_CurrentAnimationName();
            context.previousTimeSec = previousTrackPosition;
            context.currentTimeSec = _currentClip.trackPosition;
            context.deltaTime = timeDelta;
            context.isLooping = _currentClip.loop;
            context.wrapped = wrapped;
            context.isPreview = !executeNotifies;

            Evaluate_AnimNotifyEvents(*clipData, context, executeNotifies);
            Evaluate_AnimNotifyStates(*clipData, context, executeNotifies);
        }
    }

    if (!_hasAnimSequence)
    {
        _isCurrentAnimationFinished = currentFinished;
        return currentFinished;
    }

    if (!currentFinished)
        return false;

    if (_animPhase == EAnimPhase::Start)
    {
        if (!Try_AdvanceSequenceAfterCurrentFinished())
            return _isAnimSequenceFinished;

        return false;
    }

    if (_animPhase == EAnimPhase::End)
    {
        _isCurrentAnimationFinished = true;

        // [추가] End clip 종료 시 active state 정리
        Stop_AllNotifyStates(executeNotifies);

        Complete_AnimationSequence();
        return true;
    }

    return false;
}
```

주의: 첫 부분에 오타 없이 이렇게 써야 한다.
```cpp
Stop_AllNotifyStates(executeNotifies);
```

`executeEndCallback`라고 쓰면 안 된다.

---

### 1-8. sequence 정리 함수 보강

`Reset_AnimationSequenceState()` 교체:
```cpp
void Model::Reset_AnimationSequenceState()
{
    // [추가] 시퀀스를 끊고 단일 clip 재생으로 돌아갈 때 active state도 정리
    Stop_AllNotifyStates(false);

    _hasAnimSequence = false;
    _isAnimSequenceFinished = false;
    _animPhase = EAnimPhase::Start;
    _startClip = {};
    _loopClip = {};
    _endClip = {};
}
```

`Complete_AnimationSequence()` 교체:
```cpp
void Model::Complete_AnimationSequence()
{
    // [추가] 시퀀스 종료 시 active state 정리
    Stop_AllNotifyStates(false);

    _hasAnimSequence = false;
    _isAnimSequenceFinished = true;

    _startClip = {};
    _loopClip = {};
    _endClip = {};
}
```

`From_Json()` 안 `_modelGuid = newGuid; _modelType = newType;` 아래에 추가:
```cpp
    // [추가] 다른 모델로 바뀌면 이전 Notify cache는 무효화해야 한다.
    Invalidate_AnimNotifyAsset();
```

---

## 2. Prefab_View는 Animation View 진입점만 담당

### 2-1. 파일
- [Prefab_View.h](/c:/Users/moon/Desktop/Jusin/GitDesktop/Dx11_Naruto/Editor/Public/Prefab_View.h)
- [Prefab_View.cpp](/c:/Users/moon/Desktop/Jusin/GitDesktop/Dx11_Naruto/Editor/Private/Prefab_View.cpp)

---

### 2-2. `Prefab_View.h` 수정

private 함수 선언에 추가:
```cpp
private:
    // [추가] Preview model 애니메이션 UI / Animation View 진입
    void            Draw_AnimationControls();
    void            Open_AnimationView();
```

멤버 변수 추가:
```cpp
    // [추가] preview용 선택 애니메이션 상태
    int32   _previewSelectedAnimIndex = 0;
    bool    _previewAnimLoop = true;
```

---

### 2-3. `Prefab_View.cpp` include 추가

상단 include에 추가:
```cpp
#include "Animation_View.h"   // [추가] Open Animation View 연결
```

---

### 2-4. `Draw_ComponentInspector()` 안 호출 추가

`Draw_PreviewCameraInspector();` 바로 아래에 추가:
```cpp
    // [추가] Model preview가 있으면 Animation View 진입 UI 노출
    Draw_AnimationControls();
```

---

### 2-5. 새 함수 추가

```cpp
void Prefab_View::Draw_AnimationControls()
{
    auto model = Find_PreviewModel();
    if (!model || !model->Has_Animations())
        return;

    ImGui::TextColored(ImVec4(0.7f, 0.9f, 1.f, 1.f), "[ Animation ]");
    ImGui::Separator();

    vector<string> names;
    vector<const char*> items;

    const uint32 count = model->Get_AnimationCount();
    names.reserve(count);
    items.reserve(count);

    for (uint32 i = 0; i < count; ++i)
        names.push_back(model->Get_AnimationName(i));

    for (auto& name : names)
        items.push_back(name.c_str());

    // [추가] 현재 선택 인덱스를 안전하게 보정
    if (!items.empty())
    {
        _previewSelectedAnimIndex = std::clamp(
            _previewSelectedAnimIndex,
            0,
            static_cast<int32>(items.size()) - 1);

        ImGui::Combo("Preview Clip", &_previewSelectedAnimIndex, items.data(), static_cast<int32>(items.size()));
    }

    ImGui::Checkbox("Loop", &_previewAnimLoop);

    if (ImGui::Button("Apply Preview Animation"))
    {
        model->Set_Animation(static_cast<uint32>(_previewSelectedAnimIndex), _previewAnimLoop);
    }

    if (ImGui::Button("Open Animation View"))
    {
        Open_AnimationView();
    }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();
}

void Prefab_View::Open_AnimationView()
{
    auto model = Find_PreviewModel();
    if (!model)
        return;

    auto animView = dynamic_pointer_cast<Animation_View>(EDITOR->Get_Window(TEXT("Animation")));
    if (!animView)
        return;

    // [추가] 현재 preview 중인 model을 Animation View에 넘긴다.
    animView->Open_Model(model);

    // [추가] combo에서 선택한 clip이 있으면 그 clip으로 바로 포커싱
    if (model->Get_AnimationCount() > 0 &&
        _previewSelectedAnimIndex >= 0 &&
        _previewSelectedAnimIndex < static_cast<int32>(model->Get_AnimationCount()))
    {
        animView->Focus_Clip(model->Get_AnimationName(static_cast<uint32>(_previewSelectedAnimIndex)));
    }
}
```

---

### 2-6. preview animation tick 수정

기존:
```cpp
    model->Play_Animation(timeDelta);
```

교체:
```cpp
    // [변경] Prefab preview는 gameplay notify side effect를 막는다.
    model->Play_Animation(timeDelta, false);
```

---

## 3. AnimationState_Inspector 구현

### 3-1. 파일
- [AnimationState_Inspector.h](/c:/Users/moon/Desktop/Jusin/GitDesktop/Dx11_Naruto/Editor/Public/AnimationState_Inspector.h)
- [AnimationState_Inspector.cpp](/c:/Users/moon/Desktop/Jusin/GitDesktop/Dx11_Naruto/Editor/Private/AnimationState_Inspector.cpp)

---

### 3-2. `AnimationState_Inspector.h` 전체 교체본

```cpp
#pragma once

#include "Component_Inspector.h"
#include "Client_Enum.h"   // [추가] EStateAnimationMode / EMoveInputDirection

NS_BEGIN(Engine)
struct FAnimationClipSetting;   // [추가] slot 선택 포인터 반환용
NS_END

NS_BEGIN(Client)
class AnimationStateComponent;
struct FStateAnimationDesc;
struct FDirectionClipDesc;
NS_END

NS_BEGIN(Editor)

class AnimationState_Inspector : public Component_Inspector
{
public:
    AnimationState_Inspector() = default;
    ~AnimationState_Inspector() override = default;

public:
    void Draw_Inspector(shared_ptr<Component> component) override;
    uint32 Get_ComponentType() const override { return Protocol::COMPONENT_TYPE_ANIMATION_STATE; }

private:
    static bool        Pass_Filter(const string& candidate, const string& filterText);
    static const char* Get_ModeLabel(EStateAnimationMode mode);

    void    Draw_StateList(Shared<Client::AnimationStateComponent> animState);
    void    Draw_ModeCombo(Client::FStateAnimationDesc& desc);

    void    Draw_AnimationList(Shared<Client::AnimationStateComponent> animState, string* targetClipName);

    void    Draw_SingleSection(Shared<Client::AnimationStateComponent> animState, Client::FStateAnimationDesc& desc);
    void    Draw_SequenceSection(Shared<Client::AnimationStateComponent> animState, Client::FStateAnimationDesc& desc);
    void    Draw_DirectionalSection(Shared<Client::AnimationStateComponent> animState, Client::FStateAnimationDesc& desc);

    void    Draw_PreviewControls(Shared<Client::AnimationStateComponent> animState, Client::FStateAnimationDesc& desc);

    // [추가] 현재 선택된 clip을 Animation View에서 바로 열기 위한 버튼
    void    Draw_EditNotifyButton(Shared<Client::AnimationStateComponent> animState, const string& clipName);

    static FAnimationClipSetting* Get_SelectedSequenceSlot(Client::FStateAnimationDesc& desc, int32 slotIndex);
    static FAnimationClipSetting* Get_SelectedDirectionalSlot(Client::FDirectionClipDesc& desc, int32 slotIndex);

private:
    char    _newStateNameBuffer[128] = "";
    char    _searchBuffer[256] = "";

    string  _selectedStateName = "";
    int32   _selectedSequenceSlot = 0;
    int32   _selectedDirectionalSlot = 0;
};

NS_END
```

---

### 3-3. `AnimationState_Inspector.cpp` 전체 교체본

```cpp
#include "pch.h"
#include "AnimationState_Inspector.h"

#include "AnimationStateComponent.h"
#include "Animation_View.h"
#include "Client_Struct.h"
#include "Model.h"

static Client::EMoveInputDirection To_DirectionSlot(int32 slotIndex)
{
    // [추가] directional slot index -> 실제 방향 enum 변환
    if (slotIndex == 0)
        return Client::EMoveInputDirection::Forward;
    if (slotIndex == 1)
        return Client::EMoveInputDirection::Backward;
    if (slotIndex == 2)
        return Client::EMoveInputDirection::Left;

    return Client::EMoveInputDirection::Right;
}

bool AnimationState_Inspector::Pass_Filter(const string& candidate, const string& filterText)
{
    if (filterText.empty())
        return true;

    const string candidateLower = Utils::ToLowerCopy(candidate);
    const string filterLower = Utils::ToLowerCopy(filterText);

    return candidateLower.find(filterLower) != string::npos;
}

const char* AnimationState_Inspector::Get_ModeLabel(EStateAnimationMode mode)
{
    const auto name = magic_enum::enum_name(mode);
    if (name.empty())
        return "Unknown";

    return name.data();
}

void AnimationState_Inspector::Draw_Inspector(shared_ptr<Component> component)
{
    auto animState = static_pointer_cast<Client::AnimationStateComponent>(component);
    if (!animState)
        return;

    if (!Draw_Header("AnimationState"))
        return;

    ImGui::Spacing();

    ImGui::Text("Current State");
    ImGui::SameLine(120.f);
    ImGui::TextDisabled("%s", animState->Get_CurrentStateName().empty() ? "<None>" : animState->Get_CurrentStateName().c_str());

    ImGui::Text("Prev State");
    ImGui::SameLine(120.f);
    ImGui::TextDisabled("%s", animState->Get_PrevStateName().empty() ? "<None>" : animState->Get_PrevStateName().c_str());

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    Draw_StateList(animState);

    const vector<string> stateNames = animState->Get_StateNames();
    if (stateNames.empty())
    {
        ImGui::TextDisabled("No animation states.");
        return;
    }

    // [추가] 선택 state가 비었거나 삭제된 경우 첫 번째 state로 보정
    if (_selectedStateName.empty() ||
        std::find(stateNames.begin(), stateNames.end(), _selectedStateName) == stateNames.end())
    {
        _selectedStateName = stateNames.front();
    }

    auto& desc = animState->Edit_State(_selectedStateName);

    ImGui::Text("Selected State");
    ImGui::SameLine(120.f);
    ImGui::Text("%s", _selectedStateName.c_str());

    Draw_ModeCombo(desc);

    ImGui::Spacing();
    ImGui::InputText("Search", _searchBuffer, IM_ARRAYSIZE(_searchBuffer));
    ImGui::Spacing();

    if (desc.mode == EStateAnimationMode::Single)
    {
        Draw_SingleSection(animState, desc);
    }
    else if (desc.mode == EStateAnimationMode::Sequence)
    {
        Draw_SequenceSection(animState, desc);
    }
    else if (desc.mode == EStateAnimationMode::DirectionalSingle)
    {
        Draw_DirectionalSection(animState, desc);
    }

    ImGui::Spacing();
    Draw_PreviewControls(animState, desc);

    // [추가] 현재 inspector에서 바라보는 slot의 clip을 Animation View로 연다.
    string targetClipName;

    if (desc.mode == EStateAnimationMode::Single)
    {
        targetClipName = desc.single.animationName;
    }
    else if (desc.mode == EStateAnimationMode::Sequence)
    {
        if (auto* slot = Get_SelectedSequenceSlot(desc, _selectedSequenceSlot))
            targetClipName = slot->animationName;
    }
    else if (desc.mode == EStateAnimationMode::DirectionalSingle)
    {
        if (auto* slot = Get_SelectedDirectionalSlot(desc.directional, _selectedDirectionalSlot))
            targetClipName = slot->animationName;
    }

    ImGui::Spacing();
    Draw_EditNotifyButton(animState, targetClipName);
}

void AnimationState_Inspector::Draw_StateList(Shared<Client::AnimationStateComponent> animState)
{
    ImGui::Text("State List");
    ImGui::Separator();

    // [가정] 현재 AnimationStateComponent에는 Remove_State API가 없어서 add/select 위주로 구성
    ImGui::InputText("New State", _newStateNameBuffer, IM_ARRAYSIZE(_newStateNameBuffer));

    ImGui::SameLine();

    if (ImGui::Button("Add State"))
    {
        string newStateName = _newStateNameBuffer;
        if (!newStateName.empty())
        {
            animState->Edit_State(newStateName);
            _selectedStateName = newStateName;
            ::memset(_newStateNameBuffer, 0, sizeof(_newStateNameBuffer));
        }
    }

    const vector<string> stateNames = animState->Get_StateNames();

    ImGui::BeginChild("AnimationStateList", ImVec2(0.f, 180.f), true);
    {
        for (size_t i = 0; i < stateNames.size(); ++i)
        {
            const bool selected = (_selectedStateName == stateNames[i]);
            string label = stateNames[i] + "##AnimState_" + to_string(i);

            if (ImGui::Selectable(label.c_str(), selected))
                _selectedStateName = stateNames[i];

            if (selected)
                ImGui::SetItemDefaultFocus();
        }

        if (stateNames.empty())
            ImGui::TextDisabled("(No states)");
    }
    ImGui::EndChild();

    ImGui::Spacing();
}

void AnimationState_Inspector::Draw_ModeCombo(Client::FStateAnimationDesc& desc)
{
    ImGui::Text("Animation Mode");
    ImGui::SameLine(120.f);

    const char* currentLabel = Get_ModeLabel(desc.mode);

    if (ImGui::BeginCombo("##AnimationMode", currentLabel))
    {
        const bool singleSelected = (desc.mode == EStateAnimationMode::Single);
        if (ImGui::Selectable("Single", singleSelected))
            desc.mode = EStateAnimationMode::Single;

        if (singleSelected)
            ImGui::SetItemDefaultFocus();

        const bool sequenceSelected = (desc.mode == EStateAnimationMode::Sequence);
        if (ImGui::Selectable("Sequence", sequenceSelected))
            desc.mode = EStateAnimationMode::Sequence;

        if (sequenceSelected)
            ImGui::SetItemDefaultFocus();

        const bool directionalSelected = (desc.mode == EStateAnimationMode::DirectionalSingle);
        if (ImGui::Selectable("DirectionalSingle", directionalSelected))
            desc.mode = EStateAnimationMode::DirectionalSingle;

        if (directionalSelected)
            ImGui::SetItemDefaultFocus();

        ImGui::EndCombo();
    }
}

void AnimationState_Inspector::Draw_AnimationList(Shared<Client::AnimationStateComponent> animState, string* targetClipName)
{
    const vector<string> animationNames = animState->Get_ModelAnimationNames();
    const string filterText = _searchBuffer;

    if (animationNames.empty())
    {
        ImGui::TextDisabled("Animation clip not found.");
        return;
    }

    ImGui::BeginChild("AnimationClipList", ImVec2(0.f, 220.f), true);
    {
        bool anyVisible = false;

        for (size_t i = 0; i < animationNames.size(); ++i)
        {
            const string& animName = animationNames[i];

            if (!Pass_Filter(animName, filterText))
                continue;

            anyVisible = true;

            const bool selected = (targetClipName && *targetClipName == animName);
            string label = animName + "##AnimClip_" + to_string(i);

            if (ImGui::Selectable(label.c_str(), selected))
            {
                if (targetClipName)
                    *targetClipName = animName;
            }

            if (selected)
                ImGui::SetItemDefaultFocus();
        }

        if (!anyVisible)
            ImGui::TextDisabled("(No animation matches the filter)");
    }
    ImGui::EndChild();
}

void AnimationState_Inspector::Draw_SingleSection(Shared<Client::AnimationStateComponent> animState, Client::FStateAnimationDesc& desc)
{
    ImGui::Text("Selected Clip");
    ImGui::SameLine(120.f);
    ImGui::TextDisabled("%s", desc.single.animationName.empty() ? "<None>" : desc.single.animationName.c_str());

    ImGui::Spacing();
    Draw_AnimationList(animState, &desc.single.animationName);

    ImGui::Spacing();
    ImGui::Checkbox("Loop", &desc.single.loop);

    ImGui::SetNextItemWidth(120.f);
    ImGui::DragFloat("Play Rate", &desc.single.playRate, 0.01f, 0.05f, 3.f, "%.2f");
    desc.single.playRate = Utils::Max(desc.single.playRate, 0.01f);
}

void AnimationState_Inspector::Draw_SequenceSection(Shared<Client::AnimationStateComponent> animState, Client::FStateAnimationDesc& desc)
{
    ImGui::Text("Start Clip");
    ImGui::SameLine(120.f);
    ImGui::TextDisabled("%s", desc.start.animationName.empty() ? "<None>" : desc.start.animationName.c_str());

    ImGui::Text("Loop Clip");
    ImGui::SameLine(120.f);
    ImGui::TextDisabled("%s", desc.loop.animationName.empty() ? "<None>" : desc.loop.animationName.c_str());

    ImGui::Text("End Clip");
    ImGui::SameLine(120.f);
    ImGui::TextDisabled("%s", desc.end.animationName.empty() ? "<None>" : desc.end.animationName.c_str());

    ImGui::Spacing();

    const char* slotLabels[] = { "Start", "Loop", "End" };
    ImGui::SetNextItemWidth(120.f);
    ImGui::Combo("Clip Slot", &_selectedSequenceSlot, slotLabels, IM_ARRAYSIZE(slotLabels));

    auto* selectedSlot = Get_SelectedSequenceSlot(desc, _selectedSequenceSlot);
    if (!selectedSlot)
        return;

    Draw_AnimationList(animState, &selectedSlot->animationName);

    ImGui::Spacing();
    ImGui::Checkbox("Slot Loop", &selectedSlot->loop);

    ImGui::SetNextItemWidth(120.f);
    ImGui::DragFloat("Slot Play Rate", &selectedSlot->playRate, 0.01f, 0.05f, 3.f, "%.2f");
    selectedSlot->playRate = Utils::Max(selectedSlot->playRate, 0.01f);
}

void AnimationState_Inspector::Draw_DirectionalSection(Shared<Client::AnimationStateComponent> animState, Client::FStateAnimationDesc& desc)
{
    ImGui::Text("Forward Clip");
    ImGui::SameLine(120.f);
    ImGui::TextDisabled("%s", desc.directional.forward.animationName.empty() ? "<None>" : desc.directional.forward.animationName.c_str());

    ImGui::Text("Backward Clip");
    ImGui::SameLine(120.f);
    ImGui::TextDisabled("%s", desc.directional.backward.animationName.empty() ? "<None>" : desc.directional.backward.animationName.c_str());

    ImGui::Text("Left Clip");
    ImGui::SameLine(120.f);
    ImGui::TextDisabled("%s", desc.directional.left.animationName.empty() ? "<None>" : desc.directional.left.animationName.c_str());

    ImGui::Text("Right Clip");
    ImGui::SameLine(120.f);
    ImGui::TextDisabled("%s", desc.directional.right.animationName.empty() ? "<None>" : desc.directional.right.animationName.c_str());

    ImGui::Spacing();

    const char* slotLabels[] = { "Forward", "Backward", "Left", "Right" };
    ImGui::SetNextItemWidth(120.f);
    ImGui::Combo("Direction Slot", &_selectedDirectionalSlot, slotLabels, IM_ARRAYSIZE(slotLabels));

    auto* selectedSlot = Get_SelectedDirectionalSlot(desc.directional, _selectedDirectionalSlot);
    if (!selectedSlot)
        return;

    Draw_AnimationList(animState, &selectedSlot->animationName);

    ImGui::Spacing();
    ImGui::Checkbox("Slot Loop", &selectedSlot->loop);

    ImGui::SetNextItemWidth(120.f);
    ImGui::DragFloat("Slot Play Rate", &selectedSlot->playRate, 0.01f, 0.05f, 3.f, "%.2f");
    selectedSlot->playRate = Utils::Max(selectedSlot->playRate, 0.01f);
}

void AnimationState_Inspector::Draw_PreviewControls(Shared<Client::AnimationStateComponent> animState, Client::FStateAnimationDesc& desc)
{
    if (ImGui::Button("Preview Play", ImVec2(140.f, 30.f)))
    {
        if (desc.mode == EStateAnimationMode::DirectionalSingle)
        {
            animState->Preview_State(
                _selectedStateName,
                _selectedDirectionalSlot,
                To_DirectionSlot(_selectedDirectionalSlot));
        }
        else if (desc.mode == EStateAnimationMode::Sequence)
        {
            animState->Preview_State(
                _selectedStateName,
                _selectedSequenceSlot,
                Client::EMoveInputDirection::Forward);
        }
        else
        {
            animState->Preview_State(
                _selectedStateName,
                0,
                Client::EMoveInputDirection::Forward);
        }
    }

    ImGui::SameLine();

    if (ImGui::Button("Apply State", ImVec2(120.f, 30.f)))
    {
        if (desc.mode == EStateAnimationMode::DirectionalSingle)
        {
            animState->Play_DirectionalState(_selectedStateName, To_DirectionSlot(_selectedDirectionalSlot));
        }
        else
        {
            animState->Play_State(_selectedStateName);
        }
    }

    ImGui::SameLine();

    if (ImGui::Button("Request End", ImVec2(120.f, 30.f)))
    {
        animState->Request_StateEnd();
    }

    ImGui::SameLine();

    if (ImGui::Button("Clear", ImVec2(100.f, 30.f)))
    {
        if (desc.mode == EStateAnimationMode::Single)
        {
            desc.single = {};
        }
        else if (desc.mode == EStateAnimationMode::Sequence)
        {
            desc.start = {};
            desc.loop = {};
            desc.end = {};
        }
        else if (desc.mode == EStateAnimationMode::DirectionalSingle)
        {
            desc.directional.forward = {};
            desc.directional.backward = {};
            desc.directional.left = {};
            desc.directional.right = {};
        }
    }
}

void AnimationState_Inspector::Draw_EditNotifyButton(Shared<Client::AnimationStateComponent> animState, const string& clipName)
{
    // [추가] 현재 slot에 연결된 clip이 없으면 버튼 비활성화
    if (clipName.empty())
    {
        ImGui::BeginDisabled();
        ImGui::Button("Edit Notify", ImVec2(140.f, 30.f));
        ImGui::EndDisabled();
        return;
    }

    if (!ImGui::Button("Edit Notify", ImVec2(140.f, 30.f)))
        return;

    auto owner = animState->Get_Owner();
    if (!owner)
        return;

    auto model = owner->Get_Component<Model>();
    if (!model)
        return;

    auto animView = dynamic_pointer_cast<Animation_View>(EDITOR->Get_Window(TEXT("Animation")));
    if (!animView)
        return;

    animView->Open_Model(model);
    animView->Focus_Clip(clipName);
}

FAnimationClipSetting* AnimationState_Inspector::Get_SelectedSequenceSlot(Client::FStateAnimationDesc& desc, int32 slotIndex)
{
    if (slotIndex == 0)
        return &desc.start;
    if (slotIndex == 1)
        return &desc.loop;

    return &desc.end;
}

FAnimationClipSetting* AnimationState_Inspector::Get_SelectedDirectionalSlot(Client::FDirectionClipDesc& desc, int32 slotIndex)
{
    if (slotIndex == 0)
        return &desc.forward;
    if (slotIndex == 1)
        return &desc.backward;
    if (slotIndex == 2)
        return &desc.left;

    return &desc.right;
}
```

---

## 4. Factory / Editor 초기화 연결

### 4-1. `Inspector_Factory.cpp`

파일: [Inspector_Factory.cpp](/c:/Users/moon/Desktop/Jusin/GitDesktop/Dx11_Naruto/Editor/Private/Inspector_Factory.cpp)

기존:
```cpp
#include "AnimationStateComponent.h"
```

교체:
```cpp
#include "AnimationState_Inspector.h"
```

기존:
```cpp
Register_Inspector(Protocol::COMPONENT_TYPE_ANIMATION_STATE, make_shared<AnimationStateComponent>());
```

교체:
```cpp
// [수정] AnimationStateComponent는 inspector가 아니라 component다.
// 반드시 AnimationState_Inspector를 등록해야 한다.
Register_Inspector(Protocol::COMPONENT_TYPE_ANIMATION_STATE, make_shared<AnimationState_Inspector>());
```

---

### 4-2. `Editor_Manager.cpp`

파일: [Editor_Manager.cpp](/c:/Users/moon/Desktop/Jusin/GitDesktop/Dx11_Naruto/Editor/Private/Editor_Manager.cpp)

기존:
```cpp
Add_Window(TEXT("Animation Veiw"), Animation_View::Create());
```

교체:
```cpp
// [수정] 창 키 오타 수정. EDITOR->Get_Window(TEXT("Animation"))와 반드시 맞아야 한다.
Add_Window(TEXT("Animation"), Animation_View::Create());
```

---

### 4-3. `EditorInstance.cpp`

파일: [EditorInstance.cpp](/c:/Users/moon/Desktop/Jusin/GitDesktop/Dx11_Naruto/Editor/Private/EditorInstance.cpp)

include 추가:
```cpp
#include "AnimNotify_Inspector_Factory.h"
```

`Initialize_Editor()` 안에 추가:
```cpp
    Inspector_Factory::GetInstance()->Initialize();

    // [추가] Component Inspector와 별개로 Notify Inspector Factory도 초기화
    AnimNotify_Inspector_Factory::GetInstance()->Initialize();

    _editorManager = Editor_Manager::Create();
```

---

### 4-4. `Editor_MainApp.cpp`

파일: [Editor_MainApp.cpp](/c:/Users/moon/Desktop/Jusin/GitDesktop/Dx11_Naruto/Editor/Private/Editor_MainApp.cpp)

include 추가:
```cpp
#include "AnimNotify_Inspector_Factory.h"
```

`Free()` 안에 추가:
```cpp
    Inspector_Factory::DestroyInstance();

    // [추가] Editor 전용 Notify Inspector Factory 정리
    AnimNotify_Inspector_Factory::DestroyInstance();
```

---

## 5. 내일 이어서 할 때 체크 순서

1. `Model.h` 선언 추가
2. `Model.cpp` helper + lazy load + evaluate 함수 추가
3. `Play_Animation(float, bool)` 오버로드 교체
4. `Prefab_View.h/.cpp` 진입 버튼 추가
5. `AnimationState_Inspector.h/.cpp` 완성
6. `Inspector_Factory.cpp` 등록 수정
7. `Editor_Manager.cpp` 창 키 수정
8. `EditorInstance.cpp` 초기화 추가
9. `Editor_MainApp.cpp` destroy 추가

---

## 6. 내일 바로 확인할 테스트

1. `Prefab_View`에서 skeletal model 프리팹 열기
2. `Preview Clip` 콤보와 `Open Animation View` 버튼 확인
3. `AnimationState_Inspector`에서 state 추가 / mode 변경 / slot 선택 확인
4. `Edit Notify` 버튼으로 `Animation_View`가 해당 clip에 포커싱되는지 확인
5. preview 재생에서 `Play_Animation(timeDelta, false)` 경로로 gameplay callback이 막히는지 확인
6. runtime 재생에서 `Execute / On_Begin / On_Tick / On_End`가 호출되는지 확인
7. clip 전환 / sequence end에서 active state 누수 없는지 확인

---

## 7. 메모

- 현재 [AnimNotify_Serializer.cpp](/c:/Users/moon/Desktop/Jusin/GitDesktop/Dx11_Naruto/Engine/Private/AnimNotify_Serializer.cpp) 의 `From_Json()`은 중괄호 위치가 이상해서 별도 정리가 필요하다.
- 다음 작업 시작 전에 이 파일도 한 번 바로잡는 것이 안전하다.
- v1은 Montage가 아니라 외부 `.animnotify.json` 기반 비파괴 Notify Track Asset이다.
- Engine은 ImGui를 몰라야 한다.
- Notify payload UI는 반드시 Editor의 `AnimNotify_Inspector_Factory` 쪽에서만 그린다.
