#include "pch.h"
#include "Reflection_Inspector.h"
#include "Component.h"

void Reflection_Inspector::Draw_Inspector(shared_ptr<Component> component)
{
}

void Reflection_Inspector::Draw_FromReflection(void* basePtr, const Engine::FClassReflectionInfo& info)
{
    if (!Draw_Header(info.className))
        return;

    for (const auto& prop : info.properties)
    {
        Draw_Property(basePtr, prop);
    }
}

void Reflection_Inspector::Draw_Property(void* basePtr, const Engine::FPropertyInfo& prop)
{
    void* memberPtr = static_cast<char*>(basePtr) + prop.offset;
    string label = "##" + prop.name;

    switch (prop.type)
    {
    case EPropertyType::Float:
    {
        float* val = static_cast<float*>(memberPtr);
        ImGui::Text("%s", prop.name.c_str());
        ImGui::SameLine(120.f);
        ImGui::PushItemWidth(-1);
        ImGui::DragFloat(label.c_str(), val, prop.dragSpeed,prop.minVal, prop.maxVal);
        ImGui::PopItemWidth();
        break;
    }
    case EPropertyType::Int:
    {
        int* val = static_cast<int*>(memberPtr);
        ImGui::Text("%s", prop.name.c_str());
        ImGui::SameLine(120.f);
        ImGui::PushItemWidth(-1);
        ImGui::DragInt(label.c_str(), val, prop.dragSpeed,(int)prop.minVal, (int)prop.maxVal);
        ImGui::PopItemWidth();
        break;
    }
    case EPropertyType::Bool:
    {
        bool* val = static_cast<bool*>(memberPtr);
        ImGui::Checkbox(prop.name.c_str(), val);
        break;
    }
    case EPropertyType::Vec3:
    {
        float* val = static_cast<float*>(memberPtr); // Vec3 = {x,y,z}
        ImGui::Text("%s", prop.name.c_str());
        ImGui::SameLine(120.f);
        ImGui::PushItemWidth(-1);
        ImGui::DragFloat3(label.c_str(), val, prop.dragSpeed);
        ImGui::PopItemWidth();
        break;
    }
    case EPropertyType::Color:
    {
        float* val = static_cast<float*>(memberPtr); // Vec4 = {r,g,b,a}
        ImGui::Text("%s", prop.name.c_str());
        ImGui::SameLine(120.f);
        ImGui::PushItemWidth(-1);
        ImGui::ColorEdit4(label.c_str(), val);
        ImGui::PopItemWidth();
        break;
    }
    case EPropertyType::ReadOnly:
    {
        float* val = static_cast<float*>(memberPtr);
        ImGui::BeginDisabled();
        ImGui::Text("%s", prop.name.c_str());
        ImGui::SameLine(120.f);
        ImGui::PushItemWidth(-1);
        ImGui::DragFloat(label.c_str(), val);
        ImGui::PopItemWidth();
        ImGui::EndDisabled();
        break;
    }
    case EPropertyType::Enum:
    {
        int* val = static_cast<int*>(memberPtr);
        ImGui::Text("%s", prop.name.c_str());
        ImGui::SameLine(120.f);
        ImGui::PushItemWidth(-1);

        // prop.enumNames는 MPROPERTY_ENUM 매크로가 magic_enum으로 자동 채워둔 값
        const char* preview = (*val >= 0 && *val < (int)prop.enumNames.size())
            ? prop.enumNames[*val].c_str() : "???";

        if (ImGui::BeginCombo(label.c_str(), preview))
        {
            for (int i = 0; i < (int)prop.enumNames.size(); ++i)
            {
                bool isSelected = (*val == i);
                if (ImGui::Selectable(prop.enumNames[i].c_str(), isSelected))
                    *val = i;
                if (isSelected) ImGui::SetItemDefaultFocus();
            }
            ImGui::EndCombo();
        }
        ImGui::PopItemWidth();
        break;
    }
    }
}
