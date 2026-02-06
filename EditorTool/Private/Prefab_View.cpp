#include "pch.h"
#include "Prefab_View.h"
#include "GameInstance.h"
#include "GameObject.h"


Prefab_View::Prefab_View()
    : EditorWindow(TEXT("Console"))
{
}

Prefab_View::~Prefab_View()
{
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

    ImVec2 windowSize(500, 600);
    ImVec2 viewportSize = ImGui::GetMainViewport()->Size;
    ImVec2 windowPos(
        (viewportSize.x - windowSize.x) * 0.5f,
        (viewportSize.y - windowSize.y) * 0.5f
    );

    ImGui::SetNextWindowPos(windowPos, ImGuiCond_Appearing);
    ImGui::SetNextWindowSize(windowSize, ImGuiCond_Appearing);

    ImGuiWindowFlags flags = ImGuiWindowFlags_NoCollapse;

    string title = "Prefab View - " + _prefabName + "###PrefabView";
    if (ImGui::Begin(title.c_str(), &_isOpen, flags))
    {
        Draw_Header();
        ImGui::Separator();
        Draw_ComponentList();
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
        _isOpen;

    else
        LOG_ERROR("Failed to open prefab {}", prefabName);
}

void Prefab_View::Close_Prefab()
{
}

void Prefab_View::Draw_Header()
{
    ImGui::TextColored(ImVec4(1, 1, 0, 1), "Prefab: %s", _prefabName.c_str());
    ImGui::TextDisabled("Path: %s", _prefabPath.c_str());
}

void Prefab_View::Draw_ComponentList()
{
    if (_targetObject)
        return;

    ImGui::Text("Components:");

    for (const auto& pair : _targetObject->Get_Components())
    {
        auto& component = pair.second;
        if (!component) continue;

        string compName = Utils::ToString(component->Get_Name());

        if (compName.empty())
            compName = "Unknown Component";

        if (ImGui::TreeNode(compName.c_str()))
        {
            // TODO : 인스펙터 보여주기

            ImGui::TreePop();
        }
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

shared_ptr<Prefab_View> Prefab_View::Create()
{
    return make_shared<Prefab_View>();
}
