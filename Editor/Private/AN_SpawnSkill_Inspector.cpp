#include "pch.h"
#include "AN_SpawnSkill_Inspector.h"
#include "AN_SpawnSkill.h"
#include "GameObject_Factory.h"
#include "Reflection_Inspector.h"
#include "Utils.h"

vector<pair<Protocol::OBJECT_TYPE, string>> AN_SpawnSkill_Inspector::Build_SpawnableItems()
{
    vector<pair<Protocol::OBJECT_TYPE, string>> items;

    auto entries = Engine::GameObject_Factory::Get_RegisteredObjectsByCategory("SkillSpawn");
    items.reserve(entries.size());

    for (const auto& entry : entries)
    {
        items.emplace_back(entry.type, Utils::ToString(entry.name));
    }

    return items;
}

void AN_SpawnSkill_Inspector::Draw_Inspector(Shared<AnimNotify> notify)
{
    auto spawnNotify = dynamic_pointer_cast<Client::AN_SpawnSkill>(notify);
    if (!spawnNotify)
    {
        ImGui::TextDisabled("Invalid AN_SpawnSkill");
        return;
    }

    auto items = Build_SpawnableItems();
    if (items.empty())
    {
        ImGui::TextDisabled("No SkillSpawn objects registered");
    }
    else
    {
        Protocol::OBJECT_TYPE currentType = spawnNotify->Get_SpawnObjectType();
        int32 currentIndex = 0;

        for (int32 i = 0; i < static_cast<int32>(items.size()); ++i)
        {
            if (items[i].first == currentType)
            {
                currentIndex = i;
                break;
            }
        }

        const char* preview = items[currentIndex].second.c_str();

        ImGui::Text("스폰 오브젝트");
        ImGui::SameLine(120.f);
        ImGui::PushItemWidth(-1);

        if (ImGui::BeginCombo("##SpawnObjectType", preview))
        {
            for (int32 i = 0; i < static_cast<int32>(items.size()); ++i)
            {
                const bool isSelected = (currentIndex == i);
                if (ImGui::Selectable(items[i].second.c_str(), isSelected))
                {
                    spawnNotify->Set_SpawnObjectType(items[i].first);
                }

                if (isSelected)
                    ImGui::SetItemDefaultFocus();
            }

            ImGui::EndCombo();
        }

        ImGui::PopItemWidth();
    }

    Reflection_Inspector::Draw_Properties_Only(spawnNotify.get(), spawnNotify->Get_ReflectionInfo());
}
