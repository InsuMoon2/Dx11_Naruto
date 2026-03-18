#pragma once

#include "Component_Inspector.h"
#include "Client_Enum.h"

NS_BEGIN(Engine)
NS_END

NS_BEGIN(Client)
class PlayerStateMachine;
struct FStateAnimationDesc;
NS_END

NS_BEGIN(Editor)

class PlayerStateMachine_Inspector : public Component_Inspector
{
public:
    explicit PlayerStateMachine_Inspector() = default;
    virtual ~PlayerStateMachine_Inspector() = default;

public:
    void    Draw_Inspector(shared_ptr<Component> component) override;
    uint32  Get_ComponentType() const override { return Protocol::COMPONENT_TYPE_PLAYER_STATE; }

private:
    static bool        Pass_Filter(const string& candidate, const string& filterText);
    static const char* Get_StateLabel(EPlayerState state);
    static const char* Get_ModeLabel(EStateAnimationMode mode);

    void    Draw_StateCombo();
    void    Draw_ModeCombo(FStateAnimationDesc& desc);

    void    Draw_AnimationList(Shared<PlayerStateMachine> stateMachine,
                                FStateAnimationDesc& desc, string* targetClipName);

    void    Draw_SingleAnimationSection(Shared<PlayerStateMachine> stateMachine,
        FStateAnimationDesc& desc);

    void    Draw_SequenceAnimationSection(Shared<PlayerStateMachine> stateMachine,
        FStateAnimationDesc& desc);

    void    Draw_DirectionalAnimationSection(Shared<PlayerStateMachine> stateMachine,
        FStateAnimationDesc& desc);

    void    Draw_PreviewControls(Shared<PlayerStateMachine> stateMachine,
        FStateAnimationDesc& desc);

    static FAnimationClipSetting* Get_SelectedSequenceSlot(FStateAnimationDesc& desc, int32 slotIndex);
    static FAnimationClipSetting* Get_SelectedDirectionalSlot(FDirectionClipDesc& desc, int32 slotIndex);

private:
    EPlayerState    _selectedState = EPlayerState::Idle;
    char            _searchBuffer[256] = "";

    int32           _selectedSequenceSlot = 0;
    int32           _selectedDirectionalSlot = 0;

};

NS_END
