#include "pch.h"
#include "BehaviorTree_Inspector.h"
#include "BehaviorTree.h"
#include "BehaviorTree_View.h"
#include "Asset_Manager.h"
#include "BTTask_Wait.h"

void BehaviorTree_Inspector::Draw_Inspector(shared_ptr<Component> component)
{
    auto behavior = static_pointer_cast<BehaviorTree>(component);
    json data = component->To_Json();

    if (!Draw_Header("BehaviorTree"))
        return;

    ImGui::Spacing();

    //string fullPath = data.value("bt_filepath", string("(None)"));
    //string fileName = fs::path(fullPath).filename().stem().string();
    //
    //
    //ImGui::Text("Asset : ");
    //ImGui::SameLine();

    string btGuid = data.value("bt_guid", string(""));
    string displayName = "(None)";

    if (!btGuid.empty())
    {
        wstring resolved = GAME->Resolve_AssetPath(btGuid);
        if (!resolved.empty())
            displayName = fs::path(resolved).stem().string();
    }

    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.2f, 0.2f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.25f, 0.25f, 0.25f, 1.0f));

    if (ImGui::Button(displayName.c_str(), ImVec2(ImGui::GetContentRegionAvail().x - 10.f, 30)))
    {
        ImGui::OpenPopup("##BTPicker");
    }

    ImGui::PopStyleColor(2);

    // Picker
    if (ImGui::BeginPopup("##BTPicker"))
    {
        ImGui::Text(ICON_FA_MAGNIFYING_GLASS " BehaviorTree 선택");
        ImGui::Separator();
        ImGui::Spacing();

        auto assets = GAME->Get_AssetByType("behavior_tree");

        if (assets.empty())
        {
            ImGui::TextDisabled("(등록된 BehaviorTree 없음)");
        }

        for (auto* meta : assets)
        {
            string name = fs::path(meta->fullPath).stem().string();
            // 현재 세팅된 거 하이라이트
            bool isSelected = (meta->guid == btGuid);

            if (isSelected)
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.3f, 0.8f, 0.3f, 1.0f));

            if (ImGui::Selectable(name.c_str(), isSelected, ImGuiSelectableFlags_None, ImVec2(200.f, 0)))
            {
                data["bt_guid"] = meta->guid;
                behavior->From_Json(data);

                LOG_INFO("BehaviorTree Assigned : {}", name);

                ImGui::CloseCurrentPopup();
            }

            if (isSelected)
                ImGui::PopStyleColor();
        }

        ImGui::Spacing();
        ImGui::Separator();

        if (ImGui::Selectable("(None) - 해제", false))
        {
            data["bt_guid"] = "(None)";
            behavior->From_Json(data);
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();

    }

    // 드래그앤 드롭
    if (ImGui::BeginDragDropTarget())
    {
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("CONTENT_BEHAVIORTREE"))
        {
            //const tchar* droppedPathW = (const tchar*)payload->Data;
            string guid = (const char*)payload->Data;
            wstring resolvedPath = GAME->Resolve_AssetPath(guid);

            if (resolvedPath.empty())
            {
                LOG_ERROR("Unknown BT asset GUID: {}", guid);
                ImGui::EndDragDropTarget();
                return;
            }

            string droppedPath = Utils::ToString(resolvedPath);
            string pureName = fs::path(droppedPath).filename().stem().string();

            // 받아온 경로로 Json 갱신
            data["bt_guid"] = guid;

            behavior->From_Json(data);

            LOG_INFO("Behavior Tree Assgined : {}", pureName);
        }

        ImGui::EndDragDropTarget();
    }

    ImGui::Spacing();

    if (ImGui::Button("Clear", ImVec2(60, 30)))
    {
        data["bt_guid"] = "";
        behavior->From_Json(data);
    }

    ImGui::Spacing();

    if (GAME->Get_GameState() == EGameState::Play)
    {
        ImGui::Separator();
        ImGui::Spacing();

        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.6f, 0.8f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.7f, 0.9f, 1.0f));

        if (ImGui::Button(ICON_FA_BUG " Debug In Node Editor", ImVec2(-1, 35)))
        {
            auto btView = dynamic_pointer_cast<BehaviorTree_View>(
                EDITOR->Get_Window(TEXT("BehaviorTree")));

            if (btView)
            {
                string resolvedPath;
                if (!btGuid.empty())
                {
                    wstring resolved = GAME->Resolve_AssetPath(btGuid);
                    if (!resolved.empty())
                        resolvedPath = Utils::ToString(resolved);
                }

                btView->Request_DebugSession(resolvedPath, behavior);
            }
        }

        ImGui::PopStyleColor(2);
    }
}

void BehaviorTree_Inspector::Draw_WaitMode(Shared<BTTask_Wait> node)
{
    float waitTime = node->Get_WaitTime();

    if (ImGui::InputFloat("Wait Time ", &waitTime, 0.1f, 1.f, "%.2f"))
    {
        node->Set_WaitTime(waitTime);
    }

    ImGui::Text("Elapsed: %.2f", node->Get_Elapsed());

}
