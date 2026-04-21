#include "pch.h"
#include "Effect_Inspector.h"
#include "EffectComponent.h"
#include "EffectAsset_Serializer.h"

// effect.json 파일 경로를 인스펙터 재생용 asset 이름으로 바꿀 때 호출한다.
static string Build_EffectInspectorAssetName(const fs::path& effectPath)
{
    string fileName = effectPath.filename().string();
    const string suffix = ".effect.json";

    if (fileName.size() >= suffix.size() &&
        fileName.compare(fileName.size() - suffix.size(), suffix.size(), suffix) == 0)
    {
        return fileName.substr(0, fileName.size() - suffix.size());
    }

    return effectPath.stem().string();
}

void Effect_Inspector::Draw_Inspector(shared_ptr<Component> component)
{
    auto effectCom = dynamic_pointer_cast<Engine::EffectComponent>(component);
    if (!effectCom)
        return;

    if (!Draw_Header("EffectComponent"))
        return;

    ImGui::Spacing();

    const string currentAssetName = effectCom->Get_CurrentAssetName().empty()
        ? "<None>"
        : effectCom->Get_CurrentAssetName();

    ImGui::Text("Current Asset");
    ImGui::SameLine(120.f);
    ImGui::TextDisabled("%s", currentAssetName.c_str());

    ImGui::Text("Playing");
    ImGui::SameLine(120.f);
    ImGui::TextDisabled("%s", effectCom->Is_Playing() ? "true" : "false");

    ImGui::Text("LifeSpan");
    ImGui::SameLine(120.f);
    ImGui::TextDisabled("%.3f", effectCom->Get_LifeSpan());

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    Draw_PlaybackSection(effectCom);

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    Draw_LayerSection(effectCom);
}

void Effect_Inspector::Draw_PlaybackSection(Shared<Engine::EffectComponent> effectCom)
{
    ImGui::Text("Preview Asset");
    ImGui::SameLine(120.f);

    const string previewAssetLabel = (_previewAssetName[0] == '\0')
        ? string("<None>")
        : string(_previewAssetName);

    const float totalWidth = ImGui::GetContentRegionAvail().x;
    const float selectButtonWidth = 72.f;
    const float spacingWidth = ImGui::GetStyle().ItemSpacing.x;
    const float mainButtonWidth = (std::max)(120.f, totalWidth - (selectButtonWidth + spacingWidth));

    if (ImGui::Button((previewAssetLabel + "##EffectPreviewAssetButton").c_str(), ImVec2(mainButtonWidth, 0.f)))
    {
        memset(_effectAssetSearchBuf, 0, sizeof(_effectAssetSearchBuf));
        ImGui::OpenPopup("EffectPreviewAssetPicker");
    }

    ImGui::SameLine();

    if (ImGui::Button("Select", ImVec2(selectButtonWidth, 0.f)))
    {
        memset(_effectAssetSearchBuf, 0, sizeof(_effectAssetSearchBuf));
        ImGui::OpenPopup("EffectPreviewAssetPicker");
    }

    Draw_EffectAssetPickerPopup();

    if (ImGui::Button("Play Preview", ImVec2(120.f, 0.f)))
    {
        if (_previewAssetName[0] != '\0')
        {
            Engine::EffectComponent::FPlayDesc playDesc{};
            playDesc.effectAssetName = _previewAssetName;
            playDesc.loopOverride = false;

            effectCom->Play_Effect(playDesc);
        }
    }

    ImGui::SameLine();

    if (ImGui::Button("Loop Preview", ImVec2(120.f, 0.f)))
    {
        if (_previewAssetName[0] != '\0')
        {
            Engine::EffectComponent::FPlayDesc playDesc{};
            playDesc.effectAssetName = _previewAssetName;
            playDesc.loopOverride = true;

            effectCom->Play_Effect(playDesc);
        }
    }

    ImGui::SameLine();

    if (ImGui::Button("Stop", ImVec2(120.f, 0.f)))
    {
        effectCom->Stop_Effect();
    }
}

void Effect_Inspector::Draw_EffectAssetPickerPopup()
{
    if (!ImGui::BeginPopup("EffectPreviewAssetPicker"))
        return;

    ImGui::Text("Select Effect Asset");
    ImGui::Separator();

    ImGui::SetNextItemWidth(420.f);
    ImGui::InputTextWithHint(
        "##EffectInspectorAssetSearch",
        "Search by file name...",
        _effectAssetSearchBuf,
        IM_ARRAYSIZE(_effectAssetSearchBuf));

    ImGui::Spacing();

    const string searchLower = Utils::ToLowerCopy(_effectAssetSearchBuf);
    const vector<fs::path> effectFiles = Engine::EffectAsset_Serializer::Get_EffectFiles();

    ImGui::BeginChild("EffectInspectorAssetList", ImVec2(520.f, 320.f), true);

    if (effectFiles.empty())
    {
        ImGui::TextDisabled("(등록된 effect asset 없음)");
    }
    else
    {
        bool hasVisibleItem = false;

        for (const auto& effectPath : effectFiles)
        {
            const string assetName = Build_EffectInspectorAssetName(effectPath);

            if (!searchLower.empty())
            {
                const string assetNameLower = Utils::ToLowerCopy(assetName);
                if (assetNameLower.find(searchLower) == string::npos)
                    continue;
            }

            hasVisibleItem = true;

            const bool isSelected = (assetName == _previewAssetName);
            if (ImGui::Selectable(assetName.c_str(), isSelected))
            {
                strcpy_s(_previewAssetName, assetName.c_str());
                ImGui::CloseCurrentPopup();
            }

            if (ImGui::IsItemHovered())
            {
                ImGui::SetTooltip("%s", effectPath.string().c_str());
            }
        }

        if (!hasVisibleItem)
        {
            ImGui::TextDisabled("(검색 결과 없음)");
        }
    }

    ImGui::EndChild();

    if (ImGui::Button("Clear", ImVec2(80.f, 0.f)))
    {
        _previewAssetName[0] = '\0';
    }

    ImGui::EndPopup();
}

void Effect_Inspector::Draw_LayerSection(Shared<Engine::EffectComponent> effectCom)
{
    const auto& layers = effectCom->Get_ActiveLayers();

    ImGui::Text("Active Layers");
    ImGui::SameLine(120.f);
    ImGui::TextDisabled("%zu", layers.size());

    if (layers.empty())
    {
        ImGui::TextDisabled("(No Active Layers)");
        return;
    }

    for (size_t i = 0; i < layers.size(); ++i)
    {
        const auto& layer = layers[i];

        string label = "[" + to_string(i) + "] ";
        if (!layer.desc.base.layerName.empty())
            label += layer.desc.base.layerName;
        else
            label += "Unnamed Layer";

        if (ImGui::TreeNode(label.c_str()))
        {
            const auto kindName = magic_enum::enum_name(layer.desc.base.kind);
            const string kindLabel = kindName.empty() ? "Unknown" : string(kindName);

            ImGui::Text("Kind");
            ImGui::SameLine(120.f);
            ImGui::TextDisabled("%s", kindLabel.c_str());

            ImGui::Text("Enabled");
            ImGui::SameLine(120.f);
            ImGui::TextDisabled("%s", layer.desc.base.enabled ? "true" : "false");

            ImGui::Text("Started");
            ImGui::SameLine(120.f);
            ImGui::TextDisabled("%s", layer.started ? "true" : "false");

            ImGui::Text("Finished");
            ImGui::SameLine(120.f);
            ImGui::TextDisabled("%s", layer.finished ? "true" : "false");

            ImGui::Text("Elapsed");
            ImGui::SameLine(120.f);
            ImGui::TextDisabled("%.3f", layer.elapsed);

            ImGui::Text("Start Delay");
            ImGui::SameLine(120.f);
            ImGui::TextDisabled("%.3f", layer.desc.base.startDelay);

            ImGui::Text("Duration");
            ImGui::SameLine(120.f);
            ImGui::TextDisabled("%.3f", layer.desc.base.duration);

            ImGui::Text("Loop");
            ImGui::SameLine(120.f);
            ImGui::TextDisabled("%s", layer.desc.base.loop ? "true" : "false");

            ImGui::Text("Has Object");
            ImGui::SameLine(120.f);
            ImGui::TextDisabled("%s", layer.obj ? "true" : "false");

            ImGui::TreePop();
        }
    }
}
