#include "pch.h"
#include "Hierarchy.h"
#include "GameInstance.h"
#include "GameObject.h"
#include "Inspector.h"
#include "Editor_Manager.h"
#include "EditorInstance.h"

Hierarchy::Hierarchy()
    : EditorWindow(TEXT("Hieararchy"))
{
}

Hierarchy::~Hierarchy()
{
}

void Hierarchy::Initialize()
{
    EditorWindow::Initialize();
}

void Hierarchy::Update(float timeDelta)
{
    EditorWindow::Update(timeDelta);

    uint32 currentLevelndex = GAME->Current_Level();
    _levelObjects = GAME->Get_GameObjects(currentLevelndex);
}

void Hierarchy::OnGui()
{
    string str = Utils::ToString(Get_Name());

    ImGui::Begin(str.c_str(), nullptr, ImGuiWindowFlags_NoNavInputs);
    {
        Draw_SearchBar();
        Draw_ObjectList();
    }
    ImGui::End();
}

void Hierarchy::Draw_SearchBar()
{
    // 검색창
    static char searchBuf[128] = "";
    ImGui::InputTextWithHint("##SearchHierarchy", "Search...", searchBuf, IM_ARRAYSIZE(searchBuf));
    ImGui::Separator();

    _currentSearchFilter = searchBuf;

    int filteredCount = 0;

    if (_currentSearchFilter.empty())
    {
        filteredCount = static_cast<int>(_levelObjects.size());
    }
    else
    {
        for (auto& obj : _levelObjects)
        {
            if (!obj) continue;

            wstring nameW = obj->Get_Name();
            string nameStr = string(nameW.begin(), nameW.end());
            
            if (nameStr.find(_currentSearchFilter) != string::npos)
                filteredCount++;
        }
    }

    ImGui::Text("Objects: %d / %d", filteredCount, static_cast<int>(_levelObjects.size()));
    ImGui::Separator();
}

void Hierarchy::Draw_ObjectList()
{
    sort(_levelObjects.begin(), _levelObjects.end(),
        [](shared_ptr<GameObject>& first, shared_ptr<GameObject>& second)
        {
            if (!first || !second)
                return false;

            return first->Get_Name() < second->Get_Name();
        });

    for (int i = 0; i < _levelObjects.size(); i++)
    {
        auto& obj = _levelObjects[i];
        if (!obj) continue;

        // 검색 필터링
        wstring nameW = obj->Get_Name();
        string nameStr = string(nameW.begin(), nameW.end());

        if (!_currentSearchFilter.empty() && nameStr.find(_currentSearchFilter) == string::npos)
            continue;

        // 선택 상태
        bool isSelected = (_selectedObject == obj);

        ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
        if (isSelected)
            flags |= ImGuiTreeNodeFlags_Selected;

        // 트리 노드 표시
        ImGui::TreeNodeEx((void*)(intptr_t)i, flags, nameStr.c_str());

        // 클릭 시 선택
        if (ImGui::IsItemClicked())
        {
            _selectedObject = obj;

            // Inspector에 전달
            auto inspector = dynamic_pointer_cast<Inspector>(
                EDITOR->Get_Window(TEXT("Inspector")));

            if (inspector)
                inspector->Set_Target(obj);
        }
    }
}

shared_ptr<Hierarchy> Hierarchy::Create()
{
    return make_shared<Hierarchy>();
}
