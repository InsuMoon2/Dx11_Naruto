#include "pch.h"
#include "PlayerStateMachine_Inspector.h"
#include "PlayerStateMachine.h"

const char* PlayerStateMachine_Inspector::Get_StateLabel(EPlayerState state)
{
    const auto name = magic_enum::enum_name(state);
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

    const char* currentLabel = Get_StateLabel(_forceState);

    if (ImGui::BeginCombo("Force State", currentLabel))
    {
        for (EPlayerState state : magic_enum::enum_values<EPlayerState>())
        {
            if (state == EPlayerState::END)
                continue;

            const bool selected = (_forceState == state);

            if (ImGui::Selectable(Get_StateLabel(state), selected))
                _forceState = state;

            if (selected)
                ImGui::SetItemDefaultFocus();
        }

        ImGui::EndCombo();
    }

    if (ImGui::Button("Apply State", ImVec2(120.f, 30.f)))
    {
        stateMachine->Force_Enter_State(_forceState);
    }
}
