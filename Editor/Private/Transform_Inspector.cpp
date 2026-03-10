#include "pch.h"
#include "Transform_Inspector.h"

#include "Action_Command.h"
#include "Component.h"
#include "Transform.h"

void Transform_Inspector::Draw_Inspector(shared_ptr<Component> component)
{
    auto transform = static_pointer_cast<Transform>(component);
    CHECK_NULL(transform);

    if (!Draw_Header("Transform"))
        return;

    ImGui::Spacing();

    // ── 위치 ──
    ImGui::TextDisabled(" 위치");
    {
        Vec3 pos = transform->Get_LocalPosition();

        if (Draw_XYZRow("Pos", pos, 0.1f))
            transform->Set_LocalPosition(pos);

        if (ImGui::IsItemActivated())
            _capturedPos = transform->Get_LocalPosition();

        if (ImGui::IsItemDeactivatedAfterEdit()) {
            Vec3 old = _capturedPos, nw = transform->Get_LocalPosition();

            EDITOR->ExecuteCommand(Action_Command::Create(
                [=]() { transform->Set_LocalPosition(old); },
                [=]() { transform->Set_LocalPosition(nw); }, "Transform Position"));
        }
    }
    ImGui::Spacing();

    // ── 회전 ──
    ImGui::TextDisabled(" 회전");
    {
        Vec3 euler = transform->Get_LocalEulerAngles();

        if (Draw_XYZRow("Rot", euler, 1.0f))
            transform->Set_LocalEulerAngles(euler.x, euler.y, euler.z);

        if (ImGui::IsItemActivated())
            _capturedRot = transform->Get_LocalEulerAngles();

        if (ImGui::IsItemDeactivatedAfterEdit()) {
            Vec3 old = _capturedRot, nw = transform->Get_LocalEulerAngles();

            EDITOR->ExecuteCommand(Action_Command::Create(
                [=]() { transform->Set_LocalEulerAngles(old.x, old.y, old.z); },
                [=]() { transform->Set_LocalEulerAngles(nw.x, nw.y, nw.z); }, "Transform Rotation"));
        }
    }
    ImGui::Spacing();

    // ── 크기 ──
    ImGui::TextDisabled(" 크기");
    {
        Vec3 scale = transform->Get_LocalScale();

        if (Draw_XYZRow("Scl", scale, 0.01f))
            transform->Set_LocalScale(scale);

        if (ImGui::IsItemActivated())
            _capturedScale = transform->Get_LocalScale();

        if (ImGui::IsItemDeactivatedAfterEdit()) {
            Vec3 old = _capturedScale, nw = transform->Get_LocalScale();

            EDITOR->ExecuteCommand(Action_Command::Create(
                [=]() { transform->Set_LocalScale(old); },
                [=]() { transform->Set_LocalScale(nw); }, "Transform Scale"));
        }
    }
    ImGui::Spacing();
}

bool Transform_Inspector::XYZ_DragFloat(const char* label, ImVec4 color, float& value, float speed,
    const char* uniqueId, float fieldWidth)
{
    // 컬러 버튼 (X/Y/Z)
    ImGui::PushStyleColor(ImGuiCol_Button, color);
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, color);
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, color);

    ImGui::Button(label, ImVec2(20.f, ImGui::GetFrameHeight()));
    ImGui::PopStyleColor(3);
    ImGui::SameLine(0.f, 2.f);

    ImGui::SetNextItemWidth(fieldWidth);

    string dragId = string("##") + uniqueId;

    return ImGui::DragFloat(dragId.c_str(), &value, speed, 0.f, 0.f, "%.3f");
}

bool Transform_Inspector::Draw_XYZRow(const char* id, Vec3& v, float speed)
{
    bool changed = false;
    ImGui::PushID(id);

    const float colW = (ImGui::GetContentRegionAvail().x - 8.f) / 3.f;
    const float dragW = colW - 22.f;

    changed |= XYZ_DragFloat("X", ImVec4(0.75f, 0.18f, 0.18f, 1.f), v.x, speed, "X", dragW);
    ImGui::SameLine(0.f, 4.f);

    changed |= XYZ_DragFloat("Y", ImVec4(0.25f, 0.60f, 0.25f, 1.f), v.y, speed, "Y", dragW);
    ImGui::SameLine(0.f, 4.f);

    changed |= XYZ_DragFloat("Z", ImVec4(0.20f, 0.40f, 0.75f, 1.f), v.z, speed, "Z", dragW);
    ImGui::PopID();

    return changed;
}
