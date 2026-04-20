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


};

NS_END
