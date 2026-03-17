#include "pch.h"
#include "PlayerStateMachine_Inspector.h"
#include "PlayerStateMachine.h"
#include "Model.h"


bool PlayerStateMachine_Inspector::Pass_Filter(const string& candidate, const string& filterText)
{
    if (filterText.empty())
        return true;

    const string candidateLower = Utils::ToLowerCopy(candidate);
    const string filterLower = Utils::ToLowerCopy(filterText);

    return candidateLower.find(filterLower) != string::npos;
}

const char* PlayerStateMachine_Inspector::Get_StateLabel(EPlayerState state)
{
    const auto name = magic_enum::enum_name(state);
    if (name.empty())
        return "Unknown";

    return name.data();
}

const char* PlayerStateMachine_Inspector::Get_ModeLabel(EStateAnimationMode mode)
{
    const auto name = magic_enum::enum_name(mode);
    if (name.empty())
        return "Unknown";

    return name.data();
}

void PlayerStateMachine_Inspector::Draw_Inspector(shared_ptr<Component> component)
{
    auto stateMachine = static_pointer_cast<PlayerStateMachine>(component);
    if (!stateMachine)
        return;

    if (!Draw_Header("PlayerStateMachine"))
        return;

    ImGui::Spacing();

    ImGui::Text("Current State");
    ImGui::SameLine(120.f);
    ImGui::TextDisabled("%s", Get_StateLabel(stateMachine->Get_CurrentStateID()));

    ImGui::Text("Prev State");
    ImGui::SameLine(120.f);
    ImGui::TextDisabled("%s", Get_StateLabel(stateMachine->Get_PrevStateID()));

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    Draw_StateCombo();

    auto& desc = stateMachine->Edit_StateAnimation(_selectedState);

    ImGui::Text("Selected State");
    ImGui::SameLine(120.f);
    ImGui::Text("%s", Get_StateLabel(_selectedState));

    Draw_ModeCombo(desc);

    ImGui::Spacing();
    ImGui::InputText("Search", _searchBuffer, IM_ARRAYSIZE(_searchBuffer));
    ImGui::Spacing();

    if (desc.mode == EStateAnimationMode::Single)
    {
        Draw_SingleAnimationSection(stateMachine, desc);
    }
    else
    {
        Draw_SequenceAnimationSection(stateMachine, desc);
    }

    ImGui::Spacing();

    ImGui::SetNextItemWidth(120.f);
    ImGui::DragFloat("Play Rate", &desc.playRate, 0.01f, 0.05f, 3.f, "%.2f");

    // 재생속도는 0 이하가 되지 않게 보정
    desc.playRate = Utils::Max(desc.playRate, 0.01f);

    ImGui::Spacing();
    Draw_PreviewControls(stateMachine, desc);
}


void PlayerStateMachine_Inspector::Draw_StateCombo()
{
    const char* currentLabel = Get_StateLabel(_selectedState);

    if (ImGui::BeginCombo("State", currentLabel))
    {
        for (EPlayerState state : magic_enum::enum_values<EPlayerState>())
        {
            if (state == EPlayerState::END)
                continue;

            const bool selected = (_selectedState == state);

            if (ImGui::Selectable(Get_StateLabel(state), selected))
                _selectedState = state;

            if (selected)
                ImGui::SetItemDefaultFocus();
        }

        ImGui::EndCombo();
    }
}

void PlayerStateMachine_Inspector::Draw_ModeCombo(FStateAnimationDesc& desc)
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

        ImGui::EndCombo();
    }
}

void PlayerStateMachine_Inspector::Draw_AnimationList(Shared<PlayerStateMachine> stateMachine,
                                                      FStateAnimationDesc& desc, string* targetClipName)
{
    const auto model = stateMachine->Get_Model();
    if (!model)
    {
        ImGui::TextDisabled("Model component not found.");
        return;
    }

    const string filterText = _searchBuffer;
    const vector<string> animationNames = stateMachine->Get_AvaiableAnimationNames();

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

        if (!Pass_Filter(animName, filterText))
            continue;

        anyVisible = true;

        const bool selected = (targetClipName && *targetClipName == animName);

        string itemLabel = animName + "##Anim_" + to_string(i);

        if (ImGui::Selectable(itemLabel.c_str(), selected))
        {
            if (targetClipName)
                *targetClipName = animName;
        }

        if (selected)
            ImGui::SetItemDefaultFocus();
    }

    if (!anyVisible)
        ImGui::TextDisabled("(No animation matches the filter)");

    ImGui::EndChild();
}

void PlayerStateMachine_Inspector::Draw_SingleAnimationSection(Shared<PlayerStateMachine> stateMachine,
    FStateAnimationDesc& desc)
{
    ImGui::Text("Selected Clip");
    ImGui::SameLine(120.f);
    ImGui::TextDisabled("%s", desc.single.animationName.empty() ? "<None>" : desc.single.animationName.c_str());

    ImGui::Spacing();
    Draw_AnimationList(stateMachine, desc, &desc.single.animationName);

    ImGui::Spacing();
    ImGui::Checkbox("Loop", &desc.single.loop);

    ImGui::SetNextItemWidth(120.f);
    ImGui::DragFloat("Play Rate", &desc.single.playRate, 0.01f, 0.05f, 3.f, "%.2f");
    desc.single.playRate = Utils::Max(desc.single.playRate, 0.01f);
}

void PlayerStateMachine_Inspector::Draw_SequenceAnimationSection(Shared<PlayerStateMachine> stateMachine,
    FStateAnimationDesc& desc)
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

    Draw_AnimationList(stateMachine, desc, &selectedSlot->animationName);

    ImGui::Spacing();
    ImGui::Checkbox("Slot Loop", &selectedSlot->loop);

    ImGui::SetNextItemWidth(120.f);
    ImGui::DragFloat("Slot Play Rate", &selectedSlot->playRate, 0.01f, 0.05f, 3.f, "%.2f");
    selectedSlot->playRate = Utils::Max(selectedSlot->playRate, 0.01f);

    ImGui::Spacing();
}

void PlayerStateMachine_Inspector::Draw_PreviewControls(Shared<PlayerStateMachine> stateMachine,
                                                        FStateAnimationDesc& desc)
{
    if (ImGui::Button("Preview Play", ImVec2(140.f, 30.f)))
    {
        stateMachine->Preview_StateAnimation(_selectedState, _selectedSequenceSlot);
    }

    ImGui::SameLine();

    if (ImGui::Button("Apply State", ImVec2(120.f, 30.f)))
    {
        stateMachine->Force_Enter_State(_selectedState);
    }

    ImGui::SameLine();

    if (ImGui::Button("Clear", ImVec2(100.f, 30.f)))
    {
        if (desc.mode == EStateAnimationMode::Single)
        {
            desc.single = {};
        }
        else
        {
            desc.start = {};
            desc.loop = {};
            desc.end = {};
        }
    }
}

FAnimationClipSetting* PlayerStateMachine_Inspector::Get_SelectedSequenceSlot(FStateAnimationDesc& desc,
    int32 slotIndex)
{
    if (slotIndex == 0)
        return &desc.start;
    if (slotIndex == 1)
        return &desc.loop;

    return &desc.end;
}
