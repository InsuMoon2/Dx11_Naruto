#include "pch.h"
#include "BehaviorTree_Inspector.h"
#include "BehaviorTree.h"
#include "BehaviorTree_View.h"

void BehaviorTree_Inspector::Draw_Inspector(shared_ptr<Component> component)
{
    auto behavior = static_pointer_cast<BehaviorTree>(component);
    json data = component->To_Json();

    if (!Draw_Header("BehaviorTree"))
        return;

    ImGui::Spacing();

    string fullPath = data.value("bt_filepath", string("(None)"));
    string fileName = fs::path(fullPath).filename().stem().string();

    ImGui::Text("Asset : ");
    ImGui::SameLine();

    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.2f, 0.2f, 1.0f));
    ImGui::Button(fileName.c_str(), ImVec2(ImGui::GetContentRegionAvail().x - 10.f, 30));
    ImGui::PopStyleColor();

    // 드래그앤 드롭
    if (ImGui::BeginDragDropTarget())
    {
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("CONTENT_BEHAVIORTREE"))
        {
            const tchar* droppedPathW = (const tchar*)payload->Data;

            string droppedPath = Utils::ToString(droppedPathW);
            string pureName = fs::path(droppedPath).filename().stem().string();

            // 받아온 경로로 Json 갱신
            data["bt_filepath"] = droppedPath;

            behavior->From_Json(data);

            LOG_INFO("Behavior Tree Assgined : {}", pureName);
        }

        ImGui::EndDragDropTarget();
    }

    ImGui::Spacing();

    if (ImGui::Button("Clear", ImVec2(60, 30)))
    {
        data["bt_filepath"] = "(None)";
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
                if (!fullPath.empty() && fullPath != "(None)")
                {
                    btView->Load_BehaviorTree(fullPath);
                }

                btView->Set_DebugTarget(behavior);
                btView->Set_DebugMode(true);
                btView->Set_Active(true);

            }


        }

        ImGui::PopStyleColor(2);
    }
}
