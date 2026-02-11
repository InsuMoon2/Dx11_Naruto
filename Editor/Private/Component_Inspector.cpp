#include "pch.h"
#include "Component_Inspector.h"

bool Component_Inspector::Draw_Header(const string& name)
{
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.2f, 0.8f, 0.9f, 1.f));
    bool isOpen = ImGui::CollapsingHeader(name.c_str(), ImGuiTreeNodeFlags_DefaultOpen);
    ImGui::PopStyleColor();
    return isOpen;
}

bool Component_Inspector::Draw_Float(const string& label, float& value)
{
    ImGui::Text(label.c_str());
    ImGui::SameLine(100.f);

    string hiddenLabel = "##" + label;

    ImGui::PushItemWidth(-1);
    bool changed = ImGui::DragFloat(hiddenLabel.c_str(), &value, 0.1f);
    ImGui::PopItemWidth();

    return changed;
}

void Component_Inspector::Draw_ProgressBar(float current, float max, const string& label)
{
    float ratio = max > 0.f ? current / max : 0.f;
    ImGui::ProgressBar(ratio, ImVec2(0.f, 0.f), label.c_str());
}

void Component_Inspector::Draw_ReadOnly(const string& label, float value)
{
    // 읽기 전용
    ImGui::BeginDisabled();
    ImGui::Text(label.c_str());
    ImGui::SameLine(100.f);

    string hiddenLabel = "##" + label;

    ImGui::PushItemWidth(-1);
    ImGui::DragFloat(hiddenLabel.c_str(), const_cast<float*>(&value));
    ImGui::PopItemWidth();

    ImGui::EndDisabled();
}

void Component_Inspector::BeginEdit()
{
    _isDirty = false;
}

void Component_Inspector::EndEdit(shared_ptr<Component> component)
{
    if (_isDirty)
    {
        // TODO : 자동 저장? 추가할지??
    }
}
