#include "pch.h"
#include "Model_Inspector.h"
#include "Asset_Manager.h"
#include "Model.h"
#include "ModelMaterial.h"
#include "Animation.h"
#include "Editor_Helper.h"

// 모델 선택 팝업에서 같은 stem 이름을 가진 자산도 경로/GUID로 구분해서 확인하기 위한 표시 문자열을 만든다.
static string Build_ModelPickerLabel(const FAssetMeta& meta)
{
    const fs::path fullPath(meta.fullPath);
    const string assetName = fullPath.stem().string();
    const string relativePath = Utils::ToString(meta.relativePath);
    const string shortGuid = meta.guid.size() > 8 ? meta.guid.substr(0, 8) : meta.guid;

    string label = assetName;

    if (!meta.modelType.empty())
        label += "  [" + meta.modelType + "]";

    if (!relativePath.empty())
        label += "  {" + relativePath + "}";

    if (!shortGuid.empty())
        label += "  <" + shortGuid + ">";

    return label;
}

static const struct
{
    EMaterialTextureSlot type; 
    const char* label;
}
s_TextureTypes[] =
{
    { EMaterialTextureSlot::BaseColor, "Base Color" }, 
    { EMaterialTextureSlot::Normal,    "Normal"     }, 
    { EMaterialTextureSlot::Specular,  "Specular"   }, 
    { EMaterialTextureSlot::Emissive,  "Emissive"   }, 
    { EMaterialTextureSlot::Metalness, "Metallic"   }, 
    { EMaterialTextureSlot::Roughness, "Roughness"  }, 
};

json Model_Inspector::Build_ModelSwapJson(const json& sourceData, const string& newGuid, const string& newModelType)
{
    json result = sourceData;

    result.erase("materials");

    result["model_guid"] = newGuid;
    result["model_type"] = newModelType;

    return result;
}

bool Model_Inspector::Is_SkeletalMeshAsset(const FAssetMeta* meta)
{
    if (!meta)
        return false;

    if (meta->type != "model" && meta->type != "Model")
        return false;

    if (meta->modelType != "SkeletalMesh")
        return false;

    const fs::path assetPath(meta->fullPath);
    return Utils::ToLowerCopy(assetPath.extension().string()) == ".meshbin";
}

void Model_Inspector::Draw_Inspector(shared_ptr<Component> component)
{
    auto model = static_pointer_cast<Model>(component);
    json data = component->To_Json();

    if (!Draw_Header("Model"))
        return;

    ImGui::Spacing();

    // 모델 선택
    Draw_ModelPicker(model, data);

    ImGui::Spacing();
    ImGui::Separator();

    // Mesh 리스트
    Draw_MeshList(model);

    ImGui::Spacing();
    ImGui::Separator();

    // Material 리스트
    Draw_MaterialSlots(model);
}

void Model_Inspector::Draw_ModelPicker(Shared<Model> model, json& data)
{
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

        bool hasSelectableModel = false;

        for (auto* meta : assets)
        {
            if (!Is_SkeletalMeshAsset(meta))
                continue;

            hasSelectableModel = true;

            bool isSelected = (meta->guid == modelGuid);
            const string label = Build_ModelPickerLabel(*meta);

            if (isSelected)
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.3f, 0.8f, 0.3f, 1.f));

            if (ImGui::Selectable(label.c_str(), isSelected, ImGuiSelectableFlags_None, ImVec2(260.f, 0)))
            {
                json swapData = Build_ModelSwapJson(data, meta->guid, meta->modelType);
                model->From_Json(swapData);

                ImGui::CloseCurrentPopup();
            }

            if (isSelected)
                ImGui::PopStyleColor();
        }

        if (!hasSelectableModel)
        {
            ImGui::TextDisabled("(meshbin 스켈레탈 메시 없음)");
        }

        ImGui::Spacing();
        ImGui::Separator();

        if (ImGui::Selectable("(None) - 해제", false))
        {
            json clearData = Build_ModelSwapJson(
                data,
                "",
                data.value("model_type", string("SkeletalMesh")));
            model->From_Json(clearData);

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
                if (!Is_SkeletalMeshAsset(meta))
                {
                    LOG_WARN("Model Assigned skipped - skeletal meshbin only: {}", guid);
                    ImGui::EndDragDropTarget();
                    return;
                }

                string mType = meta->modelType;

                json swapData = Build_ModelSwapJson(data, guid, mType);
                model->From_Json(swapData);

                LOG_INFO("Model Assigned: {}", Utils::ToString(resolved));
            }
        }
        ImGui::EndDragDropTarget();
    }

   /* ImGui::Spacing();

    if (ImGui::Button("Clear", ImVec2(60, 28)))
    {
        data["model_guid"] = "";
        model->From_Json(data);
    }*/

    // 스켈레탈 메시일 경우 애니메이션 세팅
    if (model->Get_ModelType() == EMeshVertexType::SkeletalMesh)
    {
        ImGui::Separator();
        ImGui::Text("Attached AnimBins");

        ImGui::BeginChild("AttachedAnimList", ImVec2(0.f, 100.f), true);
        for (uint32 i = 0; i < model->Get_AnimationCount(); ++i)
        {
            const string animName = model->Get_AnimationName(i);
            const string displayAnimName = Editor_Helper::Build_AnimatoinDisplayName(animName);

            ImGui::Text(" - %s", displayAnimName.c_str());

            // Hoever 시 전체 이름 출력
            if (ImGui::IsItemHovered() && displayAnimName != animName)
            {
                ImGui::BeginTooltip();
                ImGui::TextUnformatted(animName.c_str());
                ImGui::EndTooltip();
            }
        }
        ImGui::EndChild();
        
        if (ImGui::Button("Add AnimBin..."))
            ImGui::OpenPopup("Add AnimBin Popup");

        ImGui::SetNextWindowSize(ImVec2(560.f, 360.f), ImGuiCond_Appearing);

        if (ImGui::BeginPopup("Add AnimBin Popup"))
        {
            static char searchBuf[128] = "";

            ImGui::InputText("Search", searchBuf, IM_ARRAYSIZE(searchBuf));
            ImGui::Separator();

            auto allAnims = GAME->Get_All_Animations();

            string modelGuid = data.value("model_guid", string(""));
            vector<Shared<Animation>> filteredAnims;

            if (!modelGuid.empty())
            {
                wstring resolvedPath = GAME->Resolve_AssetPath(modelGuid);
                if (!resolvedPath.empty())
                {
                    fs::path modelFolder = fs::path(resolvedPath).parent_path();
                    filteredAnims = GAME->Get_Animations_InFolder(modelFolder.string());
                }
            }

            int32 visibleIndex = 0;

            for (auto& anim : filteredAnims)
            {
                if (!anim) continue;

                const string animName = anim->Get_Name();
                const string displayAnimName = Editor_Helper::Build_AnimatoinDisplayName(animName);

                if (!Editor_Helper::Passes_AnimationDisplayFilter(animName, searchBuf))
                    continue;

                // 화면에는 짧은 이름을 보여주고, 숨은 ID로 raw name을 붙여 중복 충돌 방지
                const string itemLabel = displayAnimName + "##AnimBin_" + to_string(visibleIndex++);

                if (ImGui::Selectable(itemLabel.c_str(), false, ImGuiSelectableFlags_SpanAvailWidth))
                {
                    model->Add_Animation(anim);

                    data = model->To_Json();
                }

                // 원본 이름은 tooltip으로 확인
                if (ImGui::IsItemHovered() && displayAnimName != animName)
                {
                    ImGui::BeginTooltip();
                    ImGui::TextUnformatted(animName.c_str());
                    ImGui::EndTooltip();
                }
            }
            ImGui::EndPopup();
        }

        ImGui::SameLine();
        if (ImGui::Button("Clear All Anims"))
        {
            model->Set_Animations({}); // 비우기
            data = model->To_Json();
        }
    }
}

void Model_Inspector::Draw_MeshList(Shared<Model> model)
{
    size_t numMeshs = model->Get_NumMeshes();

    if (numMeshs == 0)
    {
        ImGui::TextDisabled("메쉬 없음");
        return;
    }

    ImGui::TextColored(ImVec4(0.5f, 0.9f, 1.f, 1.f), ICON_FA_CUBE " Meshes (%zu)", numMeshs);
    ImGui::Spacing();

    if (ImGui::BeginTable("MeshTable", 3, ImGuiTableFlags_RowBg | ImGuiTableFlags_Borders))
    {
        ImGui::TableSetupColumn("Idx", ImGuiTableColumnFlags_WidthFixed, 30.f);
        ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn("Mat", ImGuiTableColumnFlags_WidthFixed, 35.f);
        ImGui::TableHeadersRow();

        for (uint32 i = 0; i < numMeshs; ++i)
        {
            ImGui::TableNextRow();

            // 인덱스
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("%u", i);

            // 이름
            ImGui::TableSetColumnIndex(1);
            string name = model->Get_MeshName(i);
            if (name.empty())
                name = "Mesh_" + to_string(i);

            ImGui::Text("%s", name.c_str());

            ImGui::TableSetColumnIndex(2);
            uint32 matIndex = model->Get_MeshMaterialIndex(i);
            ImGui::Text("[%u]", matIndex);
        }
        ImGui::EndTable();
    }

}

void Model_Inspector::Draw_MaterialSlots(Shared<Model> model)
{
    size_t numMats = model->Get_NumMaterials();

    if (numMats == 0)
    {
        ImGui::TextDisabled("(머티리얼 없음)");
        return;
    }

    ImGui::TextColored(ImVec4(1.f, 0.8f, 0.3f, 1.f), ICON_FA_PALETTE " Materials (%zu)", numMats);
    ImGui::Spacing();

    for (uint32 i = 0; i < numMats; ++i)
    {
        auto mat = model->Get_Material(i);

        if (!mat)
            continue;
        string matName = mat->Get_MaterialName();

        if (matName.empty())
            matName = "Material_" + to_string(i);

        string header = "[" + to_string(i) + "] " + matName;

        if (ImGui::TreeNodeEx(header.c_str(), ImGuiTreeNodeFlags_DefaultOpen))
        {
            ImGui::Indent(8.f);
            
            for (auto& [type, label] : s_TextureTypes)
            {
                uint32 count = mat->Get_TextureCount(type);
                if (count == 0)
                {
                    Draw_TextureSlot(mat, type, label, 0);
                }
                else
                {
                    for (uint32 ti = 0; ti < count; ++ti)
                        Draw_TextureSlot(mat, type, label, ti);
                }
            }

            ImGui::Unindent(8.f);
            ImGui::TreePop();
        }
    }
}

void Model_Inspector::Draw_TextureSlot(Shared<ModelMaterial> material, EMaterialTextureSlot slot, const char* label,uint32 index)
{
    // GUID -> 이름
    string guid = material->Get_TextureGuid(slot, index);
    string displayName = "(embedded)";
    string resolvedPath = "";

    if (!guid.empty())
    {
        wstring resolved = GAME->Resolve_AssetPath(guid);
        if (!resolved.empty())
        {
            displayName = fs::path(resolved).filename().string();
            resolvedPath = Utils::ToString(resolved);
        }
    }
    else if (material->Get_TextureCount(slot) > index)
    {
        displayName = "(embedded)";
    }
    else
    {
        displayName = "(None)";
    }

    ImGui::Text("%s", label);
    ImGui::SameLine(90.f);

    string btnId = string(label) + "_" + to_string(index) + "_"+ to_string(static_cast<uint32>(slot));

    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.18f, 0.18f, 0.18f, 1.f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.28f, 0.28f, 0.28f, 1.f));

    float btnWidth = ImGui::GetContentRegionAvail().x - 10.f;

    ImGui::Button((displayName + "##" + btnId).c_str(), ImVec2(btnWidth, 24.f));
    ImGui::PopStyleColor(2);
    
    if (ImGui::BeginDragDropTarget())
    {
        if (auto* payload = ImGui::AcceptDragDropPayload("CONTENT_TEXTURE"))
        {
            string newGuid = (const char*)payload->Data;
            
            if (SUCCEEDED(material->Override_Texture(slot, index, newGuid)))
                LOG_INFO("Texture Override [{}]: GUID={}", label, newGuid);
        }
        ImGui::EndDragDropTarget();
    }

    // 툴팁: GUID + 경로
    if (ImGui::IsItemHovered() && !guid.empty())
    {
        ImGui::BeginTooltip();
        ImGui::Text("GUID: %s", guid.c_str());
        if (!resolvedPath.empty())
            ImGui::TextDisabled("%s", resolvedPath.c_str());
        ImGui::EndTooltip();
    }

}

