#include "pch.h"
#include "Prefab_View.h"
#include "GameInstance.h"
#include "GameObject.h"
#include "Inspector.h"
#include <magic_enum/magic_enum.hpp>
#include "Component_Factory.h"

Prefab_View::Prefab_View()
    : EditorWindow(TEXT("Prefab"))
{
}

Prefab_View::~Prefab_View()
{
    Close_Prefab();
}

void Prefab_View::Initialize()
{
    EditorWindow::Initialize();
    
}

void Prefab_View::Update(float timeDelta)
{
    EditorWindow::Update(timeDelta);
}

void Prefab_View::OnGui()
{
    if (!_isOpen)
        return;

    ImVec2 windowSize(800, 600);
    ImVec2 viewportSize = ImGui::GetMainViewport()->Size;

    ImVec2 windowPos(
        (viewportSize.x - windowSize.x) * 0.5f,
        (viewportSize.y - windowSize.y) * 0.5f
    );

    ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(windowSize, ImGuiCond_Appearing);

    ImGuiWindowFlags flags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoDocking;

    string title = "Prefab View - " + _prefabName + "###PrefabView";
    if (ImGui::Begin(title.c_str(), &_isOpen, flags))
    {
        //Draw_Header();
        ImGui::Separator();

        if (ImGui::BeginTable("PrefabLayout", 2, ImGuiTableFlags_Resizable| ImGuiTableFlags_BordersInnerV))
        {
            ImGui::TableSetupColumn("ModelPreview", ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableSetupColumn("Inspector", ImGuiTableColumnFlags_WidthStretch);

            ImGui::TableNextRow();
            // [좌측] 모델 프리뷰
            ImGui::TableSetColumnIndex(0);
            {
                ImGui::Text("모델 프리뷰");
                ImVec2 previewSize(ImGui::GetContentRegionAvail().x, 300);
                ImGui::Button("##ModelPreview", previewSize); // 추후 이미지로 대체

                ImGui::Separator();
                static int currentState = 0;
                const char* states[] = { "Idle", "Run", "Attack", "Hit", "Dead" };

                ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
                ImGui::Combo("##StateCombo", &currentState, states, IM_ARRAYSIZE(states));
            }

            // [우측] 컴포넌트 리스트
            ImGui::TableSetColumnIndex(1);
            {
                Draw_ComponentList();
            }
            ImGui::EndTable();
        }
        ImGui::Separator();
        Draw_Buttons();
    }
    ImGui::End();
}

void Prefab_View::Open_Prefab(const string& prefabName, const string& prefabPath)
{
    _prefabName = prefabName;
    _prefabPath = prefabPath;

    _targetObject = nullptr;

    // 인스턴스화
    _targetObject = GAME->Instantiate_Prefab(prefabName, {});

    if (_targetObject)
        _isOpen = true;

}

void Prefab_View::Close_Prefab()
{
    _isOpen = false;
    _targetObject = nullptr;
}

void Prefab_View::Draw_Header()
{
    ImGui::TextColored(ImVec4(1, 1, 0, 1), "Prefab: %s", _prefabName.c_str());
    ImGui::TextDisabled("Path: %s", _prefabPath.c_str());
}

void Prefab_View::Draw_ComponentList()
{
    CHECK_NULL(_targetObject);

#pragma region Legacy : Inspector 방식으로 보여주기
    //Inspector::Draw_Components(_targetObject);
#pragma endregion

    ImGui::Text("Components Inspector");
    ImGui::Separator();

    auto& components = _targetObject->Get_Components();
    uint32 deleteTargetID = 0; // 삭제할 컴포넌트 ID

    for (auto& pair : components)
    {
        uint32 id = pair.first;
        auto& component = pair.second;

        if (!component)
            continue;

        ImGui::PushID(id);

        ImGui::BeginGroup();

        Inspector::Draw_Component(id, component);

        ImGui::EndGroup();

        if (ImGui::IsItemHovered() && ImGui::IsMouseReleased(ImGuiMouseButton_Right))
        {
            ImGui::OpenPopup("ComponentContextMenu");
        }
        // 4. 팝업 메뉴 열기
        if (ImGui::BeginPopup("ComponentContextMenu"))
        {
            if (ImGui::MenuItem("Remove Component"))
            {
                deleteTargetID = id;
            }
            ImGui::EndPopup();
        }
        ImGui::PopID();
    }

    if (deleteTargetID != 0)
        _targetObject->Remove_Component(deleteTargetID);

    ImGui::Separator();
    ImGui::Spacing();

    // 컴포넌트 추가 버튼
    if (ImGui::Button("Add Component", ImVec2(ImGui::GetContentRegionAvail().x, 30)))
    {
        ImGui::OpenPopup("AddComponentPopup");
    }

    if (ImGui::BeginPopup("AddComponentPopup"))
    {
        auto registeredComponents = Component_Factory::GetInstance()->Get_RegisteredComponents();

        for (const auto& pair : registeredComponents)
        {
            uint32 typeId = pair.first;
            string name = Utils::ToString(pair.second);

            if (ImGui::MenuItem(name.c_str()))
            {
                auto newComp = Component_Factory::GetInstance()->Create(typeId, GAME->Get_Device(), GAME->Get_Context());
                if (newComp)
                {
                    _targetObject->Add_Component(typeId, newComp);
                }
            }
        }
        ImGui::EndPopup();
    }
}

void Prefab_View::Draw_Buttons()
{
    if (ImGui::Button("Save", ImVec2(100.f, 0)))
    {
        if (_targetObject)
        {
            GAME->Save_Prefab(_prefabPath, _targetObject);
        }
    }

    ImGui::SameLine();

    if (ImGui::Button("Close", ImVec2(100, 0)))
    {
        _isOpen = false;
        _targetObject = nullptr;
    }
}

void Prefab_View::Add_NewComponent()
{

}

shared_ptr<Prefab_View> Prefab_View::Create()
{
    return make_shared<Prefab_View>();
}
