#include "pch.h"
#include "Reflection_Inspector.h"
#include "Component.h"
#include "Property_Command.h"

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

void Reflection_Inspector::Draw_Properties_Only(void* basePtr, const FClassReflectionInfo& info)
{
    for (const auto& prop : info.properties)
    {
        Draw_Property_Simple(basePtr, prop);
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

        if (ImGui::IsItemActivated())
            _capturedFloat = *val;

        ImGui::DragFloat(label.c_str(), val, prop.dragSpeed,prop.minVal, prop.maxVal);

        if (ImGui::IsItemDeactivatedAfterEdit())
        {
            float oldVal = _capturedFloat;
            float newVal = *val;
            auto cmd = Property_Command::Create(
                memberPtr, prop.type,json(oldVal), json(newVal));

            EDITOR->ExecuteCommand(cmd);
        }

        ImGui::PopItemWidth();
        break;
    }
    case EPropertyType::Int:
    {
        int* val = static_cast<int*>(memberPtr);
        ImGui::Text("%s", prop.name.c_str());
        ImGui::SameLine(120.f);
        ImGui::PushItemWidth(-1);

        if (ImGui::IsItemActivated())
            _capturedInt = *val;

        ImGui::DragInt(label.c_str(), val, prop.dragSpeed,(int)prop.minVal, (int)prop.maxVal);

        if (ImGui::IsItemDeactivatedAfterEdit())
        {
            auto cmd = Property_Command::Create(
                memberPtr, prop.type, json(_capturedInt), json(*val));

            EDITOR->ExecuteCommand(cmd);
        }

        ImGui::PopItemWidth();
        break;
    }
    case EPropertyType::Bool:
    {
        bool* val = static_cast<bool*>(memberPtr);
        bool oldVal = *val;

        if (ImGui::Checkbox(prop.name.c_str(), val))
        {
            auto cmd = Property_Command::Create(memberPtr, prop.type,
                json(oldVal), json(*val));

            EDITOR->ExecuteCommand(cmd);
        }
        break;
    }
    case EPropertyType::Vec3:
    {
        float* val = static_cast<float*>(memberPtr); 
        ImGui::Text("%s", prop.name.c_str());
        ImGui::SameLine(120.f);

        if (ImGui::IsItemActivated())
            memcpy(_capturedVec3, val, sizeof(float) * 3);

        ImGui::PushItemWidth(-1);
        ImGui::DragFloat3(label.c_str(), val, prop.dragSpeed);

        if (ImGui::IsItemDeactivatedAfterEdit())
        {
            json oldJ = { _capturedVec3[0], _capturedVec3[1], _capturedVec3[2] };
            json newJ = { val[0], val[1], val[2] };

            auto cmd = Property_Command::Create(
                memberPtr, prop.type, oldJ, newJ);

            EDITOR->ExecuteCommand(cmd);
        }

        ImGui::PopItemWidth();
        break;
    }
    case EPropertyType::Color:
    {
        float* val = static_cast<float*>(memberPtr); // Vec4 = {r,g,b,a}
        ImGui::Text("%s", prop.name.c_str());
        ImGui::SameLine(120.f);
        ImGui::PushItemWidth(-1);

        if (ImGui::IsItemActivated())
            memcpy(_capturedColor, val, sizeof(float) * 4);

        ImGui::ColorEdit4(label.c_str(), val);

        if (ImGui::IsItemDeactivatedAfterEdit())
        {
            json oldJ = { _capturedColor[0], _capturedColor[1], _capturedColor[2], _capturedColor[3] };
            json newJ = { val[0], val[1], val[2], val[3] };
            auto cmd = Property_Command::Create(
                memberPtr, prop.type, oldJ, newJ);
            EDITOR->ExecuteCommand(cmd);
        }

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

        int currentIndex = -1;
        for (int i = 0; i < (int)prop.enumValues.size(); ++i)
        {
            if (prop.enumValues[i] == *val) {
                currentIndex = i;
                break;
            }
        }
        const char* preview = (currentIndex != -1) ? prop.enumNames[currentIndex].c_str() : "???";
        if (ImGui::BeginCombo(label.c_str(), preview))
        {
            for (int i = 0; i < (int)prop.enumNames.size(); ++i)
            {
                bool isSelected = (currentIndex == i);
                if (ImGui::Selectable(prop.enumNames[i].c_str(), isSelected))
                {
                    int oldVal = *val;
                    int newVal = prop.enumValues[i]; 
                    *val = newVal;

                    auto cmd = Property_Command::Create(
                        memberPtr, prop.type, json(oldVal), json(newVal));
                    EDITOR->ExecuteCommand(cmd);
                }
            }
            ImGui::EndCombo();
        }
        ImGui::PopItemWidth();
        break;
    }
    }
}

void Reflection_Inspector::Draw_Property_Simple(void* basePtr, const FPropertyInfo& prop)
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
        ImGui::DragFloat(label.c_str(), val, prop.dragSpeed, prop.minVal, prop.maxVal);
        ImGui::PopItemWidth();
        break;
    }
    case EPropertyType::Int:
    {
        int* val = static_cast<int*>(memberPtr);
        ImGui::Text("%s", prop.name.c_str());
        ImGui::SameLine(120.f);
        ImGui::PushItemWidth(-1);
        ImGui::DragInt(label.c_str(), val, prop.dragSpeed, (int)prop.minVal, (int)prop.maxVal);
        ImGui::PopItemWidth();
        break;
    }
    case EPropertyType::Bool:
    {
        bool* val = static_cast<bool*>(memberPtr);
        ImGui::Checkbox(prop.name.c_str(), val);
        break;
    }
    case EPropertyType::String:
    {
        string* val = static_cast<string*>(memberPtr);
        ImGui::Text("%s", prop.name.c_str());
        ImGui::SameLine(120.f);
        ImGui::PushItemWidth(-1);

        char buf[256] = {};
        strncpy_s(buf, val->c_str(), sizeof(buf) - 1);
        if (ImGui::InputText(label.c_str(), buf, sizeof(buf)))
            *val = buf;

        ImGui::PopItemWidth();
        break;
    }

    case EPropertyType::Vec3:
{
    float* val = static_cast<float*>(memberPtr);
    ImGui::Text("%s", prop.name.c_str());
    ImGui::SameLine(120.f);
    ImGui::PushItemWidth(-1);
    ImGui::DragFloat3(label.c_str(), val, prop.dragSpeed);
    ImGui::PopItemWidth();
    break;
}

    case EPropertyType::Enum:
    {
        int* val = static_cast<int*>(memberPtr);
        ImGui::Text("%s", prop.name.c_str());
        ImGui::SameLine(120.f);
        ImGui::PushItemWidth(-1);

        int currentIndex = -1;
        for (int i = 0; i < static_cast<int>(prop.enumValues.size()); ++i)
        {
            if (prop.enumValues[i] == *val)
            {
                currentIndex = i;
                break;
            }
        }

        const char* preview =
            (currentIndex >= 0 && currentIndex < static_cast<int>(prop.enumNames.size()))
            ? prop.enumNames[currentIndex].c_str()
            : "???";

        if (ImGui::BeginCombo(label.c_str(), preview))
        {
            for (int i = 0; i < static_cast<int>(prop.enumNames.size()); ++i)
            {
                const bool isSelected = (currentIndex == i);

                if (ImGui::Selectable(prop.enumNames[i].c_str(), isSelected))
                {
                    *val = prop.enumValues[i];
                    currentIndex = i;
                }

                if (isSelected)
                    ImGui::SetItemDefaultFocus();
            }
            ImGui::EndCombo();
        }

        ImGui::PopItemWidth();
        break;
    }

    default:
        break;
    }
}
