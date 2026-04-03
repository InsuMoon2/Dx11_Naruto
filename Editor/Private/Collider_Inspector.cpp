#include "pch.h"
#include "Collider_Inspector.h"
#include "Collider.h"

#include "Bounding_AABB.h"
#include "Bounding_OBB.h"
#include "Bounding_Sphere.h"
#include "Bounding_Capsule.h"

void Collider_Inspector::Draw_Inspector(shared_ptr<Engine::Component> component)
{
    auto collider = static_pointer_cast<Collider>(component);
    if (!collider)
        return;

    if (!Draw_Header("Collider"))
        return;

    BeginEdit();

    bool isActive = collider->Get_IsActive();
    if (ImGui::Checkbox("Render (Is Active)", &isActive))
    {
        collider->Set_IsActive(isActive);
    }

    int currentPresetIdx = static_cast<int>(collider->Get_CollisionPreset());
    constexpr auto presetValues = magic_enum::enum_values<Engine::Collision_Preset>();
    constexpr auto presetNames = magic_enum::enum_names<Engine::Collision_Preset>();
    if (ImGui::BeginCombo("Collision Preset", string(presetNames[currentPresetIdx]).c_str()))
    {
        // END 제외
        for (int i = 0; i < presetValues.size() - 1; ++i)
        {
            bool isSelected = (currentPresetIdx == i);
            if (ImGui::Selectable(string(presetNames[i]).c_str(), isSelected))
            {
                collider->Set_CollisionPreset(presetValues[i]);
            }
            if (isSelected)
                ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
    }

    ImGui::TextDisabled("Channel: %s", string(magic_enum::enum_name(collider->Get_Channel())).c_str());

    auto bounding = collider->Get_Bounding();
    if (!bounding)
    {
        EndEdit(component);
        return;
    }

    ImGui::Separator();

    EShape shape = collider->Get_Shape();

    switch (shape)
    {
    case EShape::AABB:
    {
        ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "Shape : AABB");
        auto pAABB = static_pointer_cast<Bounding_AABB>(bounding);

        BoundingBox& origin = pAABB->Get_OriginAABB();
        Vec3 center = origin.Center;
        Vec3 extents = origin.Extents;

        bool changed = false;
        if (ImGui::DragFloat3("Center##AABB", (float*)&center, 0.01f)) changed = true;
        if (ImGui::DragFloat3("Extents##AABB", (float*)&extents, 0.01f, 0.01f, FLT_MAX)) changed = true;

        if (changed)
        {
            origin.Center = center;
            origin.Extents = extents;
        }
        break;
    }

    case EShape::OBB:
    {
        ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "Shape : OBB");
        auto pOBB = static_pointer_cast<Bounding_OBB>(bounding);

        BoundingOrientedBox& origin = pOBB->Get_OriginOBB();
        Vec3 center = origin.Center;
        Vec3 extents = origin.Extents;

        bool changed = false;
        if (ImGui::DragFloat3("Center##OBB", (float*)&center, 0.01f)) changed = true;
        if (ImGui::DragFloat3("Extents##OBB", (float*)&extents, 0.01f, 0.01f, FLT_MAX)) changed = true;

        if (changed)
        {
            origin.Center = center;
            origin.Extents = extents;
        }
        break;
    }

    case EShape::Sphere:
    {
        ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "Shape : Sphere");
        auto pSphere = static_pointer_cast<Bounding_Sphere>(bounding);

        BoundingSphere& origin = pSphere->Get_OriginSphere();
        Vec3 center = origin.Center;
        float radius = origin.Radius;

        bool changed = false;
        if (ImGui::DragFloat3("Center##Sphere", (float*)&center, 0.01f)) changed = true;
        if (ImGui::DragFloat("Radius##Sphere", &radius, 0.01f, 0.01f, FLT_MAX)) changed = true;

        if (changed)
        {
            origin.Center = center;
            origin.Radius = radius;
        }
        break;
    }

    case EShape::Capsule:
    {
        ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "Shape : Capsule");
        auto pCapsule = static_pointer_cast<Bounding_Capsule>(bounding);

        Vec3& center = pCapsule->Get_LocalCenter();
        float& radius = pCapsule->Get_OriginRadius();
        float& halfHeight = pCapsule->Get_OriginHalfHeight();
        Vec3& euler = pCapsule->Get_LocalEuler();

        Vec3  tempCenter = center;
        float tempRadius = radius;
        float tempHalfHeight = halfHeight;
        Vec3  tempEuler = euler;

        bool centerChanged = false;
        bool radiusChanged = false;
        bool heightChanged = false;
        bool rotationChanged = false;

        if (ImGui::DragFloat3("Local Center##Cap", (float*)&tempCenter, 0.01f))           centerChanged = true;
        if (ImGui::DragFloat("Radius##Cap", &tempRadius, 0.01f, 0.01f, FLT_MAX)) radiusChanged = true;
        if (ImGui::DragFloat("Half Height##Cap", &tempHalfHeight, 0.01f, 0.01f, FLT_MAX)) heightChanged = true;
        if (ImGui::DragFloat3("Local Rotation##Cap", (float*)&tempEuler, 0.5f))            rotationChanged = true;

        if (centerChanged || radiusChanged || heightChanged || rotationChanged)
        {
            center = tempCenter;
            radius = tempRadius;
            halfHeight = tempHalfHeight;

            if (radiusChanged || heightChanged)
                center.y = tempHalfHeight + tempRadius;

            pCapsule->Set_LocalEuler(tempEuler);
        }
        break;
    }

    }




    EndEdit(component);
}
