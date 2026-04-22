#pragma once

#include "Component_Inspector.h"
#include "Client_Enum.h"
#include "Client_Struct.h"

NS_BEGIN(Client)
class  AnimationStateComponent;
class  PlayerStateMachine;
struct FStateAnimationDesc;
struct FDirectionClipDesc;
NS_END

NS_BEGIN(Editor)

class AnimationState_Inspector : public Component_Inspector
{
public:
    // 인스펙터에 보여줄 애니메이션 항목을 미리 가공해 보관하는 캐시 엔트리다.
    struct FCachedAnimationEntry final
    {
        // 모델 원본 애니메이션 이름이다.
        string rawName = "";

        // 인스펙터 리스트에 보여줄 표시용 이름이다.
        string displayName = "";

        // 원본 이름 검색 비교를 빠르게 하기 위해 미리 소문자로 바꿔둔 값이다.
        string rawLower = "";

        // 표시 이름 검색 비교를 빠르게 하기 위해 미리 소문자로 바꿔둔 값이다.
        string displayLower = "";
    };

public:
    AnimationState_Inspector() = default;
    ~AnimationState_Inspector() override = default;

public:
    void    Draw_Inspector(shared_ptr<Component> component) override;
    uint32  Get_ComponentType() const override { return Protocol::COMPONENT_TYPE_ANIMATION_STATE; }

private:
    static bool        Pass_Filter(const string& candidate, const string& filterText);
    static const char* Get_ModeLabel(EStateAnimationMode mode);
    static const char* Get_PlayerStateLabel(EPlayerState state);
    static bool        Is_PlayerAnimationState(Shared<AnimationStateComponent> animState);
    // PlayerState 콤보에서 enum 전체를 매 프레임 다시 정렬하지 않도록 캐시된 목록을 가져올 때 호출한다.
    static const vector<EPlayerState>& Get_SortedPlayerStates();

    void    Draw_StateList(Shared<AnimationStateComponent> animState);
    // [추가] 현재 AnimationStateComponent에 등록된 상태 목록을 눈으로 확인하고 선택할 수 있게 그릴 때 호출한다.
    void    Draw_ExistingStateList(Shared<AnimationStateComponent> animState);
    void    Draw_ModeCombo(FStateAnimationDesc& desc);

    void    Draw_AnimationList(Shared<AnimationStateComponent> animState, string* targetClipName);

    void    Draw_SingleSection(Shared<AnimationStateComponent> animState, FStateAnimationDesc& desc);
    void    Draw_SequenceSection(Shared<AnimationStateComponent> animState, FStateAnimationDesc& desc);
    void    Draw_DirectionalSection(Shared<AnimationStateComponent> animState, FStateAnimationDesc& desc);

    void    Draw_PreviewControls(Shared<AnimationStateComponent> animState, FStateAnimationDesc& desc);

    void    Draw_EditNotifyButton(Shared<AnimationStateComponent> animState, const string& clipName);
    // 편집 중인 AnimationStateComponent가 바뀌거나 상태 추가/삭제가 일어난 뒤 상태 이름 캐시를 갱신할 때 호출한다.
    void    Refresh_StateNameCache(Shared<AnimationStateComponent> animState);
    // 편집 중인 모델이 바뀌었거나 애니메이션 수가 달라졌을 때 표시용 애니메이션 캐시를 다시 만들 때 호출한다.
    void    Refresh_AnimationCache(Shared<AnimationStateComponent> animState);
    // 상태 추가/삭제 직후 다음 Draw에서 상태 이름 캐시를 다시 만들도록 표시할 때 호출한다.
    void    Mark_StateNameCacheDirty();

    static FAnimationClipSetting* Get_SelectedSequenceSlot(FStateAnimationDesc& desc, int32 slotIndex);
    static FAnimationClipSetting* Get_SelectedDirectionalSlot(FDirectionClipDesc& desc, int32 slotIndex);
    static EMoveInputDirection    To_DirectionSlot(int32 slotIndex);

private:
    char    _newStateNameBuffer[128] = "";
    char    _searchBuffer[256] = "";

    string  _selectedStateName = "";
    EPlayerState _selectedPlayerState = EPlayerState::Idle;
    int32   _selectedSequenceSlot = 0;
    int32   _selectedDirectionalSlot = 0;

    // 현재 캐시가 어느 AnimationStateComponent를 기준으로 만들어졌는지 추적한다.
    AnimationStateComponent* _cachedAnimState = nullptr;

    // 등록된 상태 이름 목록을 매 프레임 다시 만들지 않도록 보관하는 캐시다.
    vector<string> _cachedStateNames;

    // 상태 추가/삭제 후 상태 이름 캐시를 다시 만들지 여부를 나타낸다.
    bool _isStateNameCacheDirty = true;

    // 현재 애니메이션 캐시가 어느 모델 GUID 기준인지 추적한다.
    string _cachedModelGuid = "";

    // 현재 애니메이션 캐시가 몇 개의 애니메이션을 기준으로 만들어졌는지 추적한다.
    uint32 _cachedAnimationCount = 0;

    // 애니메이션 리스트 표시용 문자열과 검색용 소문자 문자열을 묶어서 보관하는 캐시다.
    vector<FCachedAnimationEntry> _cachedAnimationEntries;


};

NS_END
