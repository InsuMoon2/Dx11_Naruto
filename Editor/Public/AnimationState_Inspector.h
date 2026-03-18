#pragma once

#include "Component_Inspector.h"

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
