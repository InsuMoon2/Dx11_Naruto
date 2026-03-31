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

        // 편의를 위해 임시 변수에 담아서 컨트롤
        Vec3 tempCenter = center;
        float tempRadius = radius;
        float tempHalfHeight = halfHeight;

        bool changed = false;
        if (ImGui::DragFloat3("Local Center##Cap", (float*)&tempCenter, 0.01f)) changed = true;
        if (ImGui::DragFloat("Radius##Cap", &tempRadius, 0.01f, 0.01f, FLT_MAX)) changed = true;
        if (ImGui::DragFloat("Half Height##Cap", &tempHalfHeight, 0.01f, 0.01f, FLT_MAX)) changed = true;

        if (changed)
        {
            center = tempCenter;
            radius = tempRadius;
            halfHeight = tempHalfHeight;
        }
        break;
    }
    }

    EndEdit(component);
}
