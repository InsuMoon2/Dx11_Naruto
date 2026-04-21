#include "pch.h"
#include "WaveTrigger_Inspector.h"

#include <algorithm>
#include <magic_enum/magic_enum.hpp>

bool WaveTrigger_Inspector::Draw_Inspector(const Shared<Client::WaveTrigger>& waveTrigger)
{
    if (!waveTrigger)
        return false;

    bool isChanged = false;

    ImGui::TextColored(ImVec4(1.f, 0.8f, 0.3f, 1.f), "[ WaveTrigger ] Spawn Entries");
    ImGui::Separator();
    ImGui::Spacing();

    auto& spawnEntries = waveTrigger->Get_SpawnEntries();

    ImGui::Text("Spawn Count");
    ImGui::SameLine(120.f);
    ImGui::TextDisabled("%zu", spawnEntries.size());

    if (ImGui::Button("Add Spawn Entry", ImVec2(170.f, 0.f)))
    {
        Client::WaveTrigger::FWaveSpawnEntry newEntry{};
        newEntry.scale = Vec3::One;
        spawnEntries.emplace_back(std::move(newEntry));
        isChanged = true;
    }

    ImGui::SameLine();

    if (ImGui::Button("Clear All", ImVec2(100.f, 0.f)))
    {
        if (!spawnEntries.empty())
        {
            spawnEntries.clear();
            isChanged = true;
        }
    }

    ImGui::Spacing();

    const vector<string> prefabNames = Collect_AvailablePrefabNames();
    int32 removeIndex = -1;
    int32 duplicateIndex = -1;

    for (size_t i = 0; i < spawnEntries.size(); ++i)
    {
        auto& entry = spawnEntries[i];
        ImGui::PushID(static_cast<int32>(i));

        string headerLabel = "[" + to_string(i) + "] ";
        headerLabel += entry.prefabName.empty() ? "Empty Spawn Entry" : entry.prefabName;

        if (ImGui::CollapsingHeader(headerLabel.c_str(), ImGuiTreeNodeFlags_DefaultOpen))
        {
            isChanged |= Draw_SpawnEntryPrefabSelector(entry, prefabNames);
            isChanged |= Draw_SpawnEntryObjectType(entry);
            isChanged |= Draw_SpawnEntryTransform(entry);

            if (ImGui::Button("Duplicate", ImVec2(100.f, 0.f)))
            {
                duplicateIndex = static_cast<int32>(i);
            }

            ImGui::SameLine();

            if (ImGui::Button("Remove", ImVec2(100.f, 0.f)))
            {
                removeIndex = static_cast<int32>(i);
            }
        }

        ImGui::PopID();
    }

    if (duplicateIndex >= 0 && duplicateIndex < static_cast<int32>(spawnEntries.size()))
    {
        spawnEntries.insert(
            spawnEntries.begin() + duplicateIndex + 1,
            spawnEntries[duplicateIndex]);
        isChanged = true;
    }

    if (removeIndex >= 0 && removeIndex < static_cast<int32>(spawnEntries.size()))
    {
        spawnEntries.erase(spawnEntries.begin() + removeIndex);
        isChanged = true;
    }

    return isChanged;
}

vector<string> WaveTrigger_Inspector::Collect_AvailablePrefabNames() const
{
    vector<string> prefabNames;
    const fs::path prefabFolder = "../../Client/Bin/Resources/Data/json/Prefabs";
    const string suffix = ".prefab.json";

    if (!fs::exists(prefabFolder))
        return prefabNames;

    for (const auto& entry : fs::directory_iterator(prefabFolder))
    {
        if (!entry.is_regular_file())
            continue;

        const string fileName = entry.path().filename().string();
        if (fileName.size() < suffix.size())
            continue;

        if (fileName.compare(fileName.size() - suffix.size(), suffix.size(), suffix) != 0)
            continue;

        prefabNames.emplace_back(fileName.substr(0, fileName.size() - suffix.size()));
    }

    sort(prefabNames.begin(), prefabNames.end());
    prefabNames.erase(unique(prefabNames.begin(), prefabNames.end()), prefabNames.end());

    return prefabNames;
}

bool WaveTrigger_Inspector::Draw_SpawnEntryPrefabSelector(
    Client::WaveTrigger::FWaveSpawnEntry& entry,
    const vector<string>& prefabNames)
{
    bool isChanged = false;

    ImGui::Text("Prefab");
    ImGui::SameLine(120.f);

    const string buttonLabel = entry.prefabName.empty() ? string("<Select Prefab>") : entry.prefabName;
    const float totalWidth = ImGui::GetContentRegionAvail().x;
    const float clearButtonWidth = 60.f;
    const float spacingWidth = ImGui::GetStyle().ItemSpacing.x;
    const float pickerButtonWidth = (std::max)(140.f, totalWidth - (clearButtonWidth + spacingWidth));

    if (ImGui::Button((buttonLabel + "##SpawnPrefabPicker").c_str(), ImVec2(pickerButtonWidth, 0.f)))
    {
        memset(_prefabSearchBuf, 0, sizeof(_prefabSearchBuf));
        ImGui::OpenPopup("WaveTriggerSpawnPrefabPicker");
    }

    ImGui::SameLine();

    if (ImGui::Button("Clear##SpawnPrefab", ImVec2(clearButtonWidth, 0.f)))
    {
        if (!entry.prefabName.empty())
        {
            entry.prefabName.clear();
            isChanged = true;
        }
    }

    if (ImGui::BeginPopup("WaveTriggerSpawnPrefabPicker"))
    {
        ImGui::Text("Select Spawn Prefab");
        ImGui::Separator();

        ImGui::SetNextItemWidth(420.f);
        ImGui::InputTextWithHint(
            "##WaveTriggerSpawnPrefabSearch",
            "Search prefab name...",
            _prefabSearchBuf,
            IM_ARRAYSIZE(_prefabSearchBuf));

        ImGui::Spacing();

        const string searchLower = Utils::ToLowerCopy(_prefabSearchBuf);
        ImGui::BeginChild("WaveTriggerSpawnPrefabList", ImVec2(460.f, 280.f), true);

        bool hasVisibleItem = false;

        for (const auto& prefabName : prefabNames)
        {
            if (!searchLower.empty())
            {
                const string prefabLower = Utils::ToLowerCopy(prefabName);
                if (prefabLower.find(searchLower) == string::npos)
                    continue;
            }

            hasVisibleItem = true;

            const bool isSelected = (entry.prefabName == prefabName);
            if (ImGui::Selectable(prefabName.c_str(), isSelected))
            {
                entry.prefabName = prefabName;
                isChanged = true;
                ImGui::CloseCurrentPopup();
            }
        }

        if (!hasVisibleItem)
        {
            ImGui::TextDisabled("(검색 결과 없음)");
        }

        ImGui::EndChild();
        ImGui::EndPopup();
    }

    return isChanged;
}

bool WaveTrigger_Inspector::Draw_SpawnEntryObjectType(Client::WaveTrigger::FWaveSpawnEntry& entry)
{
    const auto currentType = static_cast<Protocol::OBJECT_TYPE>(entry.objectTypeOverride);
    const auto currentTypeName = magic_enum::enum_name(currentType);
    string comboLabel = currentTypeName.empty() ? to_string(entry.objectTypeOverride) : string(currentTypeName);

    bool isChanged = false;

    if (ImGui::BeginCombo("Object Type Override", comboLabel.c_str()))
    {
        for (const auto enumValue : magic_enum::enum_values<Protocol::OBJECT_TYPE>())
        {
            const auto enumName = magic_enum::enum_name(enumValue);
            if (enumName.empty())
                continue;

            const uint32 enumValueInt = static_cast<uint32>(enumValue);
            const bool isSelected = (entry.objectTypeOverride == enumValueInt);

            if (ImGui::Selectable(string(enumName).c_str(), isSelected))
            {
                entry.objectTypeOverride = enumValueInt;
                isChanged = true;
            }

            if (isSelected)
                ImGui::SetItemDefaultFocus();
        }

        ImGui::EndCombo();
    }

    return isChanged;
}

bool WaveTrigger_Inspector::Draw_SpawnEntryTransform(Client::WaveTrigger::FWaveSpawnEntry& entry)
{
    bool isChanged = false;

    float position[3] = { entry.position.x, entry.position.y, entry.position.z };
    if (ImGui::DragFloat3("Position", position, 0.1f))
    {
        entry.position = Vec3(position[0], position[1], position[2]);
        isChanged = true;
    }

    float rotation[3] = { entry.rotation.x, entry.rotation.y, entry.rotation.z };
    if (ImGui::DragFloat3("Rotation", rotation, 0.5f))
    {
        entry.rotation = Vec3(rotation[0], rotation[1], rotation[2]);
        isChanged = true;
    }

    float scale[3] = { entry.scale.x, entry.scale.y, entry.scale.z };
    if (ImGui::DragFloat3("Scale", scale, 0.05f, 0.f, 1000.f))
    {
        entry.scale = Vec3(scale[0], scale[1], scale[2]);
        isChanged = true;
    }

    return isChanged;
}
