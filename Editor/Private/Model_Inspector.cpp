#include "pch.h"
#include "Model_Inspector.h"
#include "Model.h"
#include "Asset_Manager.h"

void Model_Inspector::Draw_Inspector(shared_ptr<Component> component)
{
    auto model = static_pointer_cast<Model>(component);
    json data = component->To_Json();

    if (!Draw_Header("Model"))
        return;

    ImGui::Spacing();

    string modelGuid = data.value("model_guid", string(""));
    string displayName = "(None)";

    if (!modelGuid.empty())
    {
        wstring resolved = GAME->Resolve_AssetPath(modelGuid);
        if (!resolved.empty())
            displayName = fs::path(resolved).stem().string();
    }

    string modelType = data.value("model_type", string("SkeletalMesh"));
    ImGui::Text("Type"); ImGui::SameLine(80.f);
    ImGui::TextDisabled("%s", modelType.c_str());
    ImGui::Spacing();

    ImGui::Text("Mesh"); ImGui::SameLine(80.f);
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.2f, 0.2f, 1.f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.3f, 0.3f, 1.f));

    if (ImGui::Button(displayName.c_str(), ImVec2(ImGui::GetContentRegionAvail().x - 10.f, 28.f)))
    {
        ImGui::OpenPopup("##ModelPicker");
    }
    ImGui::PopStyleColor(2);

    if (ImGui::BeginPopup("##ModelPicker"))
    {
        ImGui::Text(ICON_FA_CUBE " Model 선택");
        ImGui::Separator();
        ImGui::Spacing();

        auto assets = GAME->Get_AssetByType("model");
        if (assets.empty())
            assets = GAME->Get_AssetByType("Model");

        if (assets.empty())
        {
            ImGui::TextDisabled("(등록된 모델 없음)");
        }

        for (auto* meta : assets)
        {
            string name = fs::path(meta->fullPath).stem().string();
            bool isSelected = (meta->guid == modelGuid);

            string label = name;
            if (!meta->modelType.empty())
                label += "  [" + meta->modelType + "]";

            if (isSelected)
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.3f, 0.8f, 0.3f, 1.f));

            if (ImGui::Selectable(label.c_str(), isSelected, ImGuiSelectableFlags_None, ImVec2(260.f, 0)))
            {
                data["model_guid"] = meta->guid;
                data["model_type"] = meta->modelType;
                model->From_Json(data);

                ImGui::CloseCurrentPopup();
            }

            if (isSelected)
                ImGui::PopStyleColor();
        }

        ImGui::Spacing();
        ImGui::Separator();

        if (ImGui::Selectable("(None) - 해제", false))
        {
            data["model_guid"] = "";
            model->From_Json(data);

            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();

    }
    if (ImGui::BeginDragDropTarget())
    {
        if (auto* payload = ImGui::AcceptDragDropPayload("CONTENT_MESH"))
        {
            string guid = (const char*)payload->Data;
            wstring resolved = GAME->Resolve_AssetPath(guid);
            if (!resolved.empty())
            {
                auto* meta = GAME->Find_AssetByGUID(guid);
                string mType = meta ? meta->modelType : "SkeletalMesh";

                data["model_guid"] = guid;
                data["model_type"] = mType;

                model->From_Json(data);

                LOG_INFO("Model Assigned: {}", Utils::ToString(resolved));
            }
        }
        ImGui::EndDragDropTarget();
    }

    ImGui::Spacing();

    if (ImGui::Button("Clear", ImVec2(60, 28)))
    {
        data["model_guid"] = "";
        model->From_Json(data);
    }
}
