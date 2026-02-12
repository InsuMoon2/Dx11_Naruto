#include "pch.h"
#include "Transform_Inspector.h"
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

    if (ImGui::DragFloat3("##Position", pos, 0.1f))
    {
        transform->Set_LocalPosition(Vec3(pos[0], pos[1], pos[2]));
    }

    // Rotation
    ImGui::SeparatorText("Rotation");

    Vec3 eulerRad = transform->Get_LocalEulerAngles();
    Vec3 eulerDeg = eulerRad * (180.f / XM_PI);
    float rot[3] = { eulerDeg.x, eulerDeg.y, eulerDeg.z };

    if (ImGui::DragFloat3("##Rotation", rot, 1.0f))
    {
        Vec3 newEulerRad = Vec3(rot[0], rot[1], rot[2]) * (XM_PI / 180.f);
        transform->Set_LocalEulerAngles(newEulerRad.x, newEulerRad.y, newEulerRad.z);
    }

    // Scale
    ImGui::SeparatorText("Scale");

    Vec3 scale = transform->Get_LocalScale();
    float scl[3] = { scale.x, scale.y, scale.z };
    if (ImGui::DragFloat3("##Scale", scl, 0.1f))
    {
        transform->Set_LocalScale(Vec3(scl[0], scl[1], scl[2]));
    }

}
