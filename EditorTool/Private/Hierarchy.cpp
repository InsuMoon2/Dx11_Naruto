#include "pch.h"
#include "Hierarchy.h"
#include "GameInstance.h"
#include "GameObject.h"
#include "Inspector.h"
#include "Editor_Manager.h"
#include "EditorInstance.h"
#include "Event_Manager.h"
#include "Layer.h"

Hierarchy::Hierarchy()
    : EditorWindow(TEXT("Hierarchy"))
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

#pragma region Legacy -> 정렬 후 출력
    //uint32 currentLevelndex = GAME->Current_Level();
    //_levelObjects = GAME->Get_GameObjects(currentLevelndex);
#pragma endregion


#pragma region Layer별로 출력
    uint32 currentLevelndex = GAME->Current_Level();
    _levelLayers = GAME->Get_Layers(currentLevelndex);
#pragma endregion
    
}

void Hierarchy::OnGui()
{
    string str = Utils::ToString(Get_Name());

    ImGui::Begin(str.c_str(), nullptr, ImGuiWindowFlags_NoNavInputs);
    {
        Draw_SearchBar();
        Draw_ObjectList();

        if (ImGui::IsWindowFocused())
        {
            Handle_Shotcuts();
        }
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
    int totalCount = 0;

    for (auto& [layerTag, layer] : _levelLayers)
    {
        if (!layer) continue;
        auto& objects = layer->Get_GameObjects();

        totalCount += static_cast<int>(objects.size());

        if (_currentSearchFilter.empty())
        {
            filteredCount += static_cast<int>(objects.size());
        }
        else
        {
            for (auto& obj : objects)
            {
                if (!obj) continue;
                wstring nameW = obj->Get_Name();
                string nameStr = Utils::ToString(nameW);

                if (nameStr.find(_currentSearchFilter) != string::npos)
                    filteredCount++;
            }
        }
    }

    ImGui::Text("Objects: %d / %d", filteredCount, totalCount);
    ImGui::Separator();
}

void Hierarchy::Draw_ObjectList()
{
#pragma region Legacy : 정렬 후 출력
   /* sort(_levelObjects.begin(), _levelObjects.end(),
        [](shared_ptr<GameObject>& first, shared_ptr<GameObject>& second)
        {
            if (!first || !second)
                return false;

            return first->Get_Name() < second->Get_Name();
        });*/

    //for (int i = 0; i < _levelObjects.size(); i++)
    //{
    //    auto& obj = _levelObjects[i];
    //    if (!obj) continue;

    //    // 검색 필터링
    //    wstring nameW = obj->Get_Name();
    //    string nameStr = string(nameW.begin(), nameW.end());

    //    if (!_currentSearchFilter.empty() && nameStr.find(_currentSearchFilter) == string::npos)
    //        continue;

    //    // 선택 상태
    //    bool isSelected = Is_Selected(obj);

    //    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen | ImGuiTreeNodeFlags_SpanAvailWidth;
    //    if (isSelected)
    //        flags |= ImGuiTreeNodeFlags_Selected;

    //    // 트리 노드 표시
    //    ImGui::TreeNodeEx((void*)(intptr_t)i, flags, nameStr.c_str());

    //    // 클릭 시 선택
    //    if (ImGui::IsItemClicked())
    //    {
    //        bool isMultiSelect = ImGui::GetIO().KeyCtrl;
    //        Select_Object(obj, isMultiSelect);
    //    }

    //    if (ImGui::BeginPopupContextItem())
    //    {
    //        if (!Is_Selected(obj))
    //        {
    //            Select_Object(obj, false);
    //        }

    //        if (ImGui::MenuItem("복사"))
    //        {
    //            for (auto& obj : _selectedObjects)
    //            {
    //                EVENT->Publish(FEvent_Object::Create(EEventType::Create_Object, obj));
    //            }
    //        }

    //        if (ImGui::MenuItem("삭제"))
    //        {
    //            for (auto& obj : _selectedObjects)
    //            {
    //                EVENT->Publish(FEvent_Object::Create(EEventType::Delete_Object, obj));

    //                _selectedObjects.clear();
    //            }
    //        }
    //        ImGui::EndPopup();
    //    }
    //}

#pragma endregion

#pragma region Layer별로 출력

    // 검색어 있을 때는 기존처럼.
    if (!_currentSearchFilter.empty())
    {
        for (auto& [layerTag, layer] : _levelLayers)
        {
            if (!layer)
                continue;

            auto& objects = layer->Get_GameObjects();

            int index = 0;
            for (auto& obj : objects)
            {
                if (!obj)
                    continue;

                wstring nameW = obj->Get_Name();
                string nameStr = Utils::ToString(nameW);
                
                if (nameStr.find(_currentSearchFilter) != string::npos)
                {
                    Draw_ObjectNode(obj, index);
                }

                index++;
            }
        }

        return;
    }

    // 검색어 없을 때 레이어 별로 출력
    for (auto& [layerTag, layer] : _levelLayers)
    {
        if (!layer)
            continue;

        auto& objects = layer->Get_GameObjects();
        // 비어있으면 스킵
        if (objects.empty())
            continue;

        string tagStr = Utils::ToString(layerTag);

        bool isOpen = ImGui::TreeNodeEx(tagStr.c_str(),
            ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_SpanAvailWidth);

        if (isOpen)
        {
            int index = 0;
            for (auto& obj : objects)
            {
                if (!obj)
                    continue;

                Draw_ObjectNode(obj, index++);
            }
            ImGui::TreePop(); 
        }
    }

#pragma endregion

}

void Hierarchy::Draw_ObjectNode(shared_ptr<GameObject> gameObject, int index)
{
    wstring nameW = gameObject->Get_Name();
    string nameStr = string(nameW.begin(), nameW.end());

    bool isSelected = Is_Selected(gameObject);

    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow |
                               ImGuiTreeNodeFlags_OpenOnDoubleClick |
                               ImGuiTreeNodeFlags_SpanAvailWidth;

    if (isSelected)
        flags |= ImGuiTreeNodeFlags_Selected;

    // TODO : 추후에 GameObject 에 Child를 추가할 수 있게 할지?
    flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;

    // ID 주소 겹칠 수 있어서 포인터 주소로
    ImGui::TreeNodeEx((void*)(intptr_t)gameObject.get(), flags, nameStr.c_str());

    if (ImGui::IsItemClicked())
    {
        bool isMultiSelect = ImGui::GetIO().KeyCtrl;
        Select_Object(gameObject, isMultiSelect);
    }

    if (ImGui::BeginPopupContextItem())
    {
        if (!Is_Selected(gameObject))
            Select_Object(gameObject, false);

        if (ImGui::MenuItem("복사"))
        {
            for (auto& obj : _selectedObjects)
            {
                EVENT->Publish(FEvent_Object::Create(EEventType::Create_Object, obj));
            }
        }

        if (ImGui::MenuItem("삭제"))
        {
            for (auto& obj : _selectedObjects)
            {
                EVENT->Publish(FEvent_Object::Create(EEventType::Delete_Object, obj));

                _selectedObjects.clear();
            }
        }
        ImGui::EndPopup();
    }
}

bool  Hierarchy::Is_Selected(shared_ptr<GameObject> obj)
{
    auto it = find(_selectedObjects.begin(), _selectedObjects.end(), obj);

    return it != _selectedObjects.end();
}

void Hierarchy::Select_Object(shared_ptr<GameObject> obj, bool isMultiSelect)
{
    if (isMultiSelect)
    {
        auto iter = find(_selectedObjects.begin(), _selectedObjects.end(), obj);

        if (iter != _selectedObjects.end())
        {
            _selectedObjects.erase(iter);
        }
        else
        {
            _selectedObjects.emplace_back(obj);
        }
    }
    // 단일 선택
    else
    {
        _selectedObjects.clear();
        _selectedObjects.emplace_back(obj);
    }

    auto inspector = dynamic_pointer_cast<Inspector>(EDITOR->Get_Window(TEXT("Inspector")));

    if (inspector)
    {
        if (!_selectedObjects.empty())
            inspector->Set_Target(_selectedObjects.back());

        else
            inspector->Set_Target(nullptr);
    }
}

void Hierarchy::Handle_Shotcuts()
{
    if (ImGui::IsKeyPressed(ImGuiKey_Delete))
    {
        for (auto& obj : _selectedObjects)
        {
            obj->Set_Destroy();

            EVENT->Publish(FEvent_Object::Create(EEventType::Delete_Object, obj));
        }

        _selectedObjects.clear();
    }

    if (ImGui::GetIO().KeyCtrl)
    {
        if (ImGui::IsKeyPressed(ImGuiKey_C))
        {
            _copiedObjects = _selectedObjects;
        }

        if (ImGui::IsKeyPressed(ImGuiKey_V))
        {
            for (auto& copy : _copiedObjects)
            {
                EVENT->Publish(FEvent_Object::Create(EEventType::Create_Object, copy));
            }
        }
    }
}

shared_ptr<Hierarchy> Hierarchy::Create()
{
    return make_shared<Hierarchy>();
}
