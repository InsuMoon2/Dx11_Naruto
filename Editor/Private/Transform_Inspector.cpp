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

    // Position
    ImGui::SeparatorText("Position");

    Vec3 position = transform->Get_LocalPosition();
    float pos[3] = { position.x, position.y, position.z };

    if (ImGui::IsItemActivated())
        _capturedPos = position;

    if (ImGui::DragFloat3("##Position", pos, 0.1f))
        transform->Set_LocalPosition(Vec3(pos[0], pos[1], pos[2]));

    if (ImGui::IsItemDeactivatedAfterEdit())
    {
        Vec3 oldPos = _capturedPos;
        Vec3 newPos = transform->Get_LocalPosition();

        auto cmd = Action_Command::Create(
            [=]() { transform->Set_LocalPosition(oldPos); },
            [=]() { transform->Set_LocalPosition(newPos); },
            "Transform Position"
        );
        EDITOR->ExecuteCommand(cmd);
    }

    // Rotation
    ImGui::SeparatorText("Rotation");
    Vec3 eulerDeg = transform->Get_LocalEulerAngles();
    float rot[3] = { eulerDeg.x, eulerDeg.y, eulerDeg.z };

    if (ImGui::IsItemActivated())
        _capturedRot = eulerDeg;

    if (ImGui::DragFloat3("##Rotation", rot, 1.0f))
        transform->Set_LocalEulerAngles(rot[0], rot[1], rot[2]);

    if (ImGui::IsItemDeactivatedAfterEdit())
    {
        Vec3 oldRot = _capturedRot;
        Vec3 newRot = transform->Get_LocalEulerAngles();

        auto cmd = Action_Command::Create(
            [=]() { transform->Set_LocalEulerAngles(oldRot.x, oldRot.y, oldRot.z); },
            [=]() { transform->Set_LocalEulerAngles(newRot.x, newRot.y, newRot.z); },
            "Transform Rotation"
        );
        EDITOR->ExecuteCommand(cmd);
    }

    // Scale
    ImGui::SeparatorText("Scale");
    Vec3 scale = transform->Get_LocalScale();
    float scl[3] = { scale.x, scale.y, scale.z };

    if (ImGui::IsItemActivated())
        _capturedScale = scale;

    if (ImGui::DragFloat3("##Scale", scl, 0.1f))
    {
        transform->Set_LocalScale(Vec3(scl[0], scl[1], scl[2]));
    }

    if (ImGui::IsItemDeactivatedAfterEdit())
    {
        Vec3 oldScale = _capturedScale;
        Vec3 newScale = transform->Get_LocalScale();

        auto cmd = Action_Command::Create(
            [=]() {transform->Set_LocalScale(oldScale); },
            [=]() {transform->Set_LocalScale(newScale); },
            "Transform Scale"
        );
        EDITOR->ExecuteCommand(cmd);
    }

}
