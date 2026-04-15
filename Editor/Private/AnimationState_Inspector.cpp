#include "pch.h"
#include "AnimationState_Inspector.h"
#include "AnimationStateComponent.h"
#include "PlayerStateMachine.h"
#include "Model.h"
#include "GameObject.h"
#include "Animation_View.h"
#include "Editor_Helper.h"

static void Draw_ReadOnlyClipField(const char* label, const char* widgetId, const string& clipName)
{
    ImGui::Text("%s", label);
    ImGui::SameLine(120.f);

    char buffer[512] = {};
    strcpy_s(buffer, clipName.empty() ? "<None>" : clipName.c_str());

    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
    ImGui::InputText(widgetId, buffer, IM_ARRAYSIZE(buffer), ImGuiInputTextFlags_ReadOnly);
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

    if (_selectedStateName.empty())
    {
        ImGui::TextDisabled("추가된 상태가 없음");
        return;
    }

    auto& desc = animState->Edit_State(_selectedStateName);

    //ImGui::Text("Selected State");
    //ImGui::SameLine(120.f);
    //ImGui::Text("%s", _selectedStateName.c_str());

    Draw_ModeCombo(desc);

    ImGui::Spacing();

    const float labelOffset = 120.f;
    ImGui::Text("Search");
    ImGui::SameLine(labelOffset);

    const float searchWidth = Utils::Max(320.f, ImGui::GetContentRegionAvail().x - 4.f);
    ImGui::SetNextItemWidth(searchWidth);
    ImGui::InputText("##SearchAnimState", _searchBuffer, IM_ARRAYSIZE(_searchBuffer));

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

bool AnimationState_Inspector::Pass_Filter(const string& candidate, const string& filterText)
{
    if (filterText.empty())
        return true;

    const string candiateLower = Utils::ToLowerCopy(candidate);
    const string filterLower = Utils::ToLowerCopy(filterText);

    return candiateLower.find(filterLower) != string::npos;
}

const char* AnimationState_Inspector::Get_ModeLabel(EStateAnimationMode mode)
{
    const auto name = magic_enum::enum_name(mode);
    if (name.empty())
        return "Unknown";

    return name.data();
}

const char* AnimationState_Inspector::Get_PlayerStateLabel(EPlayerState state)
{
    const auto name = magic_enum::enum_name(state);
    if (name.empty())
        return "Unknown";

    return name.data();
}

bool AnimationState_Inspector::Is_PlayerAnimationState(Shared<AnimationStateComponent> animState)
{
    if (!animState)
        return false;

    auto owner = animState->Get_Owner();
    if (!owner)
        return false;

    return owner->Get_Component<PlayerStateMachine>() != nullptr;
}

void AnimationState_Inspector::Draw_StateList(Shared<AnimationStateComponent> animState)
{
    const float labelOffset = 120.f;
    const float buttonWidth = 72.f;
    const float spacing = ImGui::GetStyle().ItemSpacing.x;

    if (Is_PlayerAnimationState(animState))
    {
        ImGui::Text("State");
        ImGui::SameLine(labelOffset);

        const float startX = ImGui::GetCursorPosX();
        const float availWidth = ImGui::GetContentRegionAvail().x;
        const float reservedWidth = buttonWidth * 2.f + spacing * 2.f;
        const float comboWidth = Utils::Max(120.f, availWidth - reservedWidth);

        const char* currentLabel = Get_PlayerStateLabel(_selectedPlayerState);

        ImGui::SetNextItemWidth(comboWidth);
        if (ImGui::BeginCombo("##PlayerState", currentLabel))
        {
            vector<EPlayerState> sortedStates;
            sortedStates.reserve(magic_enum::enum_count<EPlayerState>());

            for (EPlayerState state : magic_enum::enum_values<EPlayerState>())
            {
                if (state == EPlayerState::END)
                    continue;

                sortedStates.push_back(state);
            }

            sort(sortedStates.begin(), sortedStates.end(),
                [](EPlayerState lhs, EPlayerState rhs)
                {
                    return std::lexicographical_compare(
                        magic_enum::enum_name(lhs).begin(), magic_enum::enum_name(lhs).end(),
                        magic_enum::enum_name(rhs).begin(), magic_enum::enum_name(rhs).end(),
                        [](char l, char r)
                        {
                            return std::tolower(static_cast<unsigned char>(l)) <
                                   std::tolower(static_cast<unsigned char>(r));
                        });
                });

            for (EPlayerState state : sortedStates)
            {
                const bool selected = (_selectedPlayerState == state);

                if (ImGui::Selectable(Get_PlayerStateLabel(state), selected))
                    _selectedPlayerState = state;

                if (selected)
                    ImGui::SetItemDefaultFocus();
            }

            ImGui::EndCombo();
        }

        const auto enumName = magic_enum::enum_name(_selectedPlayerState);
        const string stateName = enumName.empty() ? "" : string(enumName);
        const bool alreadyExists = !stateName.empty() && animState->Find_State(stateName) != nullptr;

        const float buttonStartX = startX + comboWidth + spacing;

        ImGui::SameLine(buttonStartX);

        if (alreadyExists)
            ImGui::BeginDisabled();

        if (ImGui::Button("추가##AnimState", ImVec2(buttonWidth, 0.f)))
        {
            if (!stateName.empty())
            {
                animState->Edit_State(stateName);
                _selectedStateName = stateName;
            }
        }

        if (alreadyExists)
            ImGui::EndDisabled();

        ImGui::SameLine();

        if (!alreadyExists)
            ImGui::BeginDisabled();

        if (ImGui::Button("제거##AnimState", ImVec2(buttonWidth, 0.f)))
        {
            if (animState->Remove_State(stateName) && _selectedStateName == stateName)
                _selectedStateName.clear();
        }

        if (!alreadyExists)
            ImGui::EndDisabled();

        ImGui::Spacing();

        if (alreadyExists)
            _selectedStateName = stateName;
        else if (_selectedStateName == stateName)
            _selectedStateName.clear();

        return;
    }

    ImGui::Text("State");
    ImGui::SameLine(labelOffset);

    const float startX = ImGui::GetCursorPosX();
    const float availWidth = ImGui::GetContentRegionAvail().x;
    const float reservedWidth = buttonWidth * 2.f + spacing * 2.f;
    const float inputWidth = Utils::Max(120.f, availWidth - reservedWidth);

    ImGui::SetNextItemWidth(inputWidth);
    ImGui::InputText("##NewStateName", _newStateNameBuffer, IM_ARRAYSIZE(_newStateNameBuffer));

    const string stateName = _newStateNameBuffer;
    const bool alreadyExists = !stateName.empty() && animState->Find_State(stateName) != nullptr;

    const float buttonStartX = startX + inputWidth + spacing;
    ImGui::SameLine(buttonStartX);

    if (stateName.empty() || alreadyExists)
        ImGui::BeginDisabled();

    if (ImGui::Button("Add##AnimState", ImVec2(buttonWidth, 0.f)))
    {
        animState->Edit_State(stateName);
        _selectedStateName = stateName;
        ZeroMemory(_newStateNameBuffer, sizeof(_newStateNameBuffer));
    }

    if (stateName.empty() || alreadyExists)
        ImGui::EndDisabled();

    ImGui::SameLine();

    if (stateName.empty() || !alreadyExists)
        ImGui::BeginDisabled();

    if (ImGui::Button("Delete##AnimState", ImVec2(buttonWidth, 0.f)))
    {
        if (animState->Remove_State(stateName))
        {
            if (_selectedStateName == stateName)
                _selectedStateName.clear();

            ZeroMemory(_newStateNameBuffer, sizeof(_newStateNameBuffer));
        }
    }

    if (stateName.empty() || !alreadyExists)
        ImGui::EndDisabled();

    ImGui::Spacing();

    if (alreadyExists)
        _selectedStateName = stateName;
    else if (_selectedStateName == stateName)
        _selectedStateName.clear();
}


void AnimationState_Inspector::Draw_ModeCombo(FStateAnimationDesc& desc)
{
    ImGui::Text("Animation Mode");
    ImGui::SameLine(120.f);

    const char* currentLabel = Get_ModeLabel(desc.mode);

    ImGui::SetNextItemWidth(220.f);

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

void AnimationState_Inspector::Draw_AnimationList(Shared<AnimationStateComponent> animState, string* targetClipName)
{
    const string filterText = _searchBuffer;
    const vector<string> animationNames = animState->Get_ModelAnimationNames();

    if (animationNames.empty())
    {
        ImGui::TextDisabled("Animation clip not found.");
        return;
    }

    ImGui::BeginChild("AnimationList", ImVec2(0.f, 220.f), true);

    bool anyVisible = false;

    for (size_t i = 0; i < animationNames.size(); ++i)
    {
        const string& animName = animationNames[i];
        const string displayName = Editor_Helper::Build_AnimatoinDisplayName(animName);

        if (!Editor_Helper::Passes_AnimationDisplayFilter(animName, filterText))
            continue;

        anyVisible = true;

        const bool selected = (targetClipName && *targetClipName == animName);

        const string itemLabel = displayName + "##Anim_" + to_string(i);

        if (ImGui::Selectable(itemLabel.c_str(), selected))
        {
            if (targetClipName)
                *targetClipName = animName;
        }

        if (selected)
            ImGui::SetItemDefaultFocus();

        if (ImGui::IsItemHovered() && displayName != animName)
        {
            ImGui::BeginTooltip();
            ImGui::TextUnformatted(animName.c_str());
            ImGui::EndTooltip();
        }
    }

    if (!anyVisible)
        ImGui::TextDisabled("(No animation matches the filter)");

    ImGui::EndChild();
}

void AnimationState_Inspector::Draw_SingleSection(Shared<AnimationStateComponent> animState, FStateAnimationDesc& desc)
{
    Draw_ReadOnlyClipField("Selected Clip", "##SelectedSingleClip", desc.single.animationName);

    ImGui::Spacing();
    Draw_AnimationList(animState, &desc.single.animationName);

    ImGui::Spacing();
    ImGui::Checkbox("Loop##SingleStateLoop", &desc.single.loop);
    ImGui::SameLine();
    ImGui::SetNextItemWidth(120.f);
    ImGui::DragFloat("Play Rate##SingleStatePlayRate", &desc.single.playRate, 0.01f, 0.05f, 3.f, "%.2f");
    desc.single.playRate = Utils::Max(desc.single.playRate, 0.01f);
}

void AnimationState_Inspector::Draw_SequenceSection(Shared<AnimationStateComponent> animState,
    FStateAnimationDesc& desc)
{
    Draw_ReadOnlyClipField("Start Clip", "##StartClipReadOnly", desc.start.animationName);
    Draw_ReadOnlyClipField("Loop Clip", "##LoopClipReadOnly", desc.loop.animationName);
    Draw_ReadOnlyClipField("End Clip", "##EndClipReadOnly", desc.end.animationName);

    ImGui::Spacing();

    const char* slotLabels[] = { "Start", "Loop", "End" };
    ImGui::SetNextItemWidth(120.f);
    ImGui::Combo("Clip Slot##SequenceState", &_selectedSequenceSlot, slotLabels, IM_ARRAYSIZE(slotLabels));

    auto* selectedSlot = Get_SelectedSequenceSlot(desc, _selectedSequenceSlot);
    if (!selectedSlot)
        return;

    Draw_AnimationList(animState, &selectedSlot->animationName);

    ImGui::Spacing();
    ImGui::Checkbox("Slot Loop##SequenceSlotLoop", &selectedSlot->loop);
    ImGui::SameLine();
    ImGui::SetNextItemWidth(120.f);
    ImGui::DragFloat("Slot Play Rate##SequenceSlotPlayRate", &selectedSlot->playRate, 0.01f, 0.05f, 3.f, "%.2f");
    selectedSlot->playRate = Utils::Max(selectedSlot->playRate, 0.01f);
}

void AnimationState_Inspector::Draw_DirectionalSection(Shared<AnimationStateComponent> animState,
    FStateAnimationDesc& desc)
{
    Draw_ReadOnlyClipField("Forward Clip", "##ForwardClipReadOnly", desc.directional.forward.animationName);
    Draw_ReadOnlyClipField("Backward Clip", "##BackwardClipReadOnly", desc.directional.backward.animationName);
    Draw_ReadOnlyClipField("Left Clip", "##LeftClipReadOnly", desc.directional.left.animationName);
    Draw_ReadOnlyClipField("Right Clip", "##RightClipReadOnly", desc.directional.right.animationName);

    ImGui::Spacing();

    const char* slotLabels[] = { "Forward", "Backward", "Left", "Right" };
    ImGui::SetNextItemWidth(120.f);
    ImGui::Combo("Direction Slot##DirectionalState", &_selectedDirectionalSlot, slotLabels, IM_ARRAYSIZE(slotLabels));

    auto* selectedSlot = Get_SelectedDirectionalSlot(desc.directional, _selectedDirectionalSlot);
    if (!selectedSlot)
        return;

    Draw_AnimationList(animState, &selectedSlot->animationName);

    ImGui::Spacing();
    ImGui::Checkbox("Slot Loop##DirectionalSlotLoop", &selectedSlot->loop);
    ImGui::SameLine();
    ImGui::SetNextItemWidth(120.f);
    ImGui::DragFloat("Slot Play Rate##DirectionalSlotPlayRate", &selectedSlot->playRate, 0.01f, 0.05f, 3.f, "%.2f");
    selectedSlot->playRate = Utils::Max(selectedSlot->playRate, 0.01f);
}

void AnimationState_Inspector::Draw_PreviewControls(Shared<AnimationStateComponent> animState,
    FStateAnimationDesc& desc)
{
    if (ImGui::Button("프리뷰 재생##AnimState", ImVec2(140.f, 30.f)))
    {
        if (desc.mode == EStateAnimationMode::DirectionalSingle)
        {
            animState->Preview_State(
                _selectedStateName,
                _selectedDirectionalSlot,
                To_DirectionSlot(_selectedDirectionalSlot));
        }
        else
        {
            animState->Preview_State(
                _selectedStateName,
                _selectedSequenceSlot,
                EMoveInputDirection::Forward);
        }
    }

    ImGui::SameLine();

    if (ImGui::Button("제거##AnimNotifyState", ImVec2(100.f, 30.f)))
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

void AnimationState_Inspector::Draw_EditNotifyButton(Shared<AnimationStateComponent> animState, const string& clipName)
{
    if (clipName.empty())
    {
        ImGui::BeginDisabled();
        ImGui::Button("노티파이 수정##AnimStateDisabled", ImVec2(140.f, 30.f));
        ImGui::EndDisabled();
        return;
    }

    if (!ImGui::Button("노티파이 수정##AnimState", ImVec2(140.f, 30.f)))
        return;

    auto owner = animState->Get_Owner();
    if (!owner)
        return;

    auto model = owner->Get_Component<Model>();
    if (!model)
        return;

    auto animView = dynamic_pointer_cast<Animation_View>(EDITOR->Get_Window(TEXT("Animation View")));
    if (!animView)
        return;

    animView->Open_Model(model);
    animView->Focus_Clip(clipName);
}

FAnimationClipSetting* AnimationState_Inspector::Get_SelectedSequenceSlot(FStateAnimationDesc& desc, int32 slotIndex)
{
    if (slotIndex == 0)
        return &desc.start;
    if (slotIndex == 1)
        return &desc.loop;

    return &desc.end;
}

FAnimationClipSetting* AnimationState_Inspector::Get_SelectedDirectionalSlot(FDirectionClipDesc& desc, int32 slotIndex)
{
    if (slotIndex == 0)
        return &desc.forward;
    if (slotIndex == 1)
        return &desc.backward;
    if (slotIndex == 2)
        return &desc.left;

    return &desc.right;
}

EMoveInputDirection AnimationState_Inspector::To_DirectionSlot(int32 slotIndex)
{
    if (slotIndex == 0)
        return EMoveInputDirection::Forward;
    if (slotIndex == 1)
        return EMoveInputDirection::Backward;
    if (slotIndex == 2)
        return EMoveInputDirection::Left;

    return EMoveInputDirection::Right;
}
