#include "pch.h"
#include "Hierarchy.h"

#include "Action_Command.h"
#include "GameInstance.h"
#include "GameObject.h"
#include "Inspector.h"
#include "Editor_Manager.h"
#include "EditorInstance.h"
#include "Event_Manager.h"
#include "Layer.h"
#include "Scene_View.h"
#include "UIObject.h"
#include <magic_enum/magic_enum.hpp>

#include "StaticMeshActor.h"

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
        if (layerTag == TEXT("Layer_CollisionProxy")) continue;

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
#pragma region Layer별로 출력

    Draw_LightingNode();

    // 검색어 있을 때는 기존처럼.
    if (!_currentSearchFilter.empty())
    {
        for (auto& [layerTag, layer] : _levelLayers)
        {
            if (!layer)
                continue;
            if (layerTag == TEXT("Layer_CollisionProxy"))
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

        for (uint32 i = 0; i < ETOI(EUILayer::END); ++i)
        {
            EUILayer uiLayerEnum = static_cast<EUILayer>(i);
            const auto& uiObjects = GAME->Get_UILayers(uiLayerEnum);

            int index = 0;

            for (auto& uiObj : uiObjects)
            {
                if (!uiObj) continue;

                wstring nameW = uiObj->Get_Name();
                string nameStr = Utils::ToString(nameW);

                if (nameStr.find(_currentSearchFilter) != string::npos)
                {
                    Draw_ObjectNode(uiObj, index);
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
        if (layerTag == TEXT("Layer_CollisionProxy"))
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

    // UI 출력
    for (uint32 i = 0; i < ETOI(EUILayer::END); i++)
    {
        EUILayer uiLayerEnum = static_cast<EUILayer>(i);

        const auto& uiObjects = GAME->Get_UILayers(uiLayerEnum);

        if (uiObjects.empty())
            continue;

        string layerName = "[UI] Layer_" + string(magic_enum::enum_name(uiLayerEnum));

        bool isOpen = ImGui::TreeNodeEx(layerName.c_str(),
            ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_SpanAvailWidth);

        if (isOpen)
        {
            int index = 0;

            for (auto& uiObj : uiObjects)
            {
                if (!uiObj)
                    continue;

                Draw_ObjectNode(uiObj, index++);
            }
            ImGui::TreePop();
        }

    }

#pragma endregion

}

void Hierarchy::Draw_LightingNode()
{
    const bool hasPrimaryShadowLight = (GAME->Get_PrimaryShadowLightDesc() != nullptr);
    const ImGuiTreeNodeFlags rootFlags =
        ImGuiTreeNodeFlags_DefaultOpen |
        ImGuiTreeNodeFlags_SpanAvailWidth;

    if (!ImGui::TreeNodeEx("Lighting", rootFlags))
        return;

    if (!hasPrimaryShadowLight)
        ImGui::BeginDisabled();

    ImGuiTreeNodeFlags itemFlags =
        ImGuiTreeNodeFlags_Leaf |
        ImGuiTreeNodeFlags_NoTreePushOnOpen |
        ImGuiTreeNodeFlags_SpanAvailWidth;

    if (_isPrimaryShadowLightSelected)
        itemFlags |= ImGuiTreeNodeFlags_Selected;

    ImGui::TreeNodeEx("Primary Shadow Light", itemFlags, ICON_FA_SUN " Directional Light");

    if (ImGui::IsItemClicked() && hasPrimaryShadowLight)
    {
        vector<Shared<GameObject>> prevSelected = _selectedObjects;
        _selectedObjects.clear();
        _isPrimaryShadowLightSelected = true;
        Update_SelectOutline(prevSelected);

        auto inspector = dynamic_pointer_cast<Inspector>(EDITOR->Get_Window(TEXT("Inspector")));
        if (inspector)
            inspector->Set_PrimaryShadowLightTarget(true);
    }

    if (!hasPrimaryShadowLight)
        ImGui::EndDisabled();

    ImGui::TreePop();
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

    // 단일 선택
    if (ImGui::IsItemClicked())
    {
        bool isMultiSelect = ImGui::GetIO().KeyCtrl;
        Select_Object(gameObject, isMultiSelect);
    }

    // 더블 클릭
    if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
    {
        auto sceneView = dynamic_pointer_cast<Scene_View>(
            EDITOR->Get_Window(L"Scene"));

        if (sceneView)
        {
            auto transform = gameObject->Get_Component<Transform>();
            auto owner = transform->Get_Owner();

            // UI 는 이동 X
            if (auto ui = dynamic_pointer_cast<UIObject>(gameObject))
                return;

            if (transform)
            {
                sceneView->Focus_OnPosition(transform->Get_WorldPosition());
            }

        }
    }

    if (ImGui::BeginPopupContextItem())
    {
        if (!Is_Selected(gameObject))
            Select_Object(gameObject, false);

        auto staticMesh = dynamic_pointer_cast<StaticMeshActor>(gameObject);
        if (staticMesh && ImGui::MenuItem("충돌체 만들기"))
        {
            auto sceneView = dynamic_pointer_cast<Scene_View>(EDITOR->Get_Window(TEXT("Scene")));
            if (sceneView)
            {
                sceneView->Create_CollisionProxySetFromStaticMesh(staticMesh);
            }
        }

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
    vector<Shared<GameObject>> prevSelected = _selectedObjects;
    _isPrimaryShadowLightSelected = false;

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

    Update_SelectOutline(prevSelected);

    auto inspector = dynamic_pointer_cast<Inspector>(EDITOR->Get_Window(TEXT("Inspector")));

    if (inspector)
    {
        inspector->Set_PrimaryShadowLightTarget(false);

        if (!_selectedObjects.empty())
            inspector->Set_Target(_selectedObjects.back());

        else
            inspector->Set_Target(nullptr);
    }
}

void Hierarchy::Update_SelectOutline(vector<Shared<GameObject>>& obj)
{
    auto applyOutline = [](const vector<Shared<GameObject>>& objects, bool enabled)
        {
            for (auto& obj : objects)
            {
                auto staticMesh = dynamic_pointer_cast<StaticMeshActor>(obj);
                if (!staticMesh)
                    continue;

                staticMesh->Set_OutlineEnabled(enabled);

                if (enabled)
                {
                    staticMesh->Set_OutlineColor(Vec4(0.1f, 1.f, 0.1f, 1.f));
                    staticMesh->Set_OutlineThickness(0.03f);
                }
            }
        };

    applyOutline(obj, false);
    applyOutline(_selectedObjects, true);
}

void Hierarchy::Handle_Shotcuts()
{
    if (ImGui::IsKeyPressed(ImGuiKey_Delete))
    {
        vector<Shared<GameObject>> objsToDelete = _selectedObjects;
        uint32 level = GAME->Current_Level();
        wstring layer = L"Layer_GameObject";

        auto cmd = Action_Command::Create(
            [=]() // Undo 되살리기
            {
                for (auto& obj : objsToDelete)
                {
                    obj->Set_Destroy(false);
                    EVENT->Publish(FEvent_Object::Create(EEventType::Create_Object, obj));

                }
            },
            [=]() // Redo 다시 죽이기
            {
                for (auto& obj : objsToDelete)
                {
                    EVENT->Publish(FEvent_Object::Create(EEventType::Delete_Object, obj));
                }
            },
            "Delete Object"
        );

        cmd->Redo();
        EDITOR->ExecuteCommand(cmd);

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
            vector<Shared<GameObject>> clones;

            for (auto& copy : _copiedObjects)
            {
                auto clone = copy->Clone(nullptr);
                // Transform 복사
                auto srcT = copy->Get_Component<Transform>();
                auto dstT = clone->Get_Component<Transform>();

                if (srcT && dstT)
                {
                    dstT->Set_LocalPosition(srcT->Get_LocalPosition());
                    dstT->Set_LocalRotation(srcT->Get_LocalRotation());
                    dstT->Set_LocalScale(srcT->Get_LocalScale());
                }

                clone->Set_Name(copy->Get_Name() + L"_Copy");
                clones.push_back(clone);
            }

            uint32 level = GAME->Current_Level();
            wstring layer = L"Layer_GameObject";

            for (auto& obj : clones)
                GAME->Add_GameObject(level, layer, obj);

            _selectedObjects.clear();
            for (int i = 0; i < (int)clones.size(); ++i)
            {
                bool isMulti = (i > 0);
                Select_Object(clones[i], isMulti);
            }

            auto cmd = Action_Command::Create(
                [=]()// Undo -> 클론들 제거
                {  
                    for (auto& obj : clones)
                        EVENT->Publish(FEvent_Object::Create(EEventType::Delete_Object, obj));
                },
                [=]()// Redo -> 클론들 다시 추가
                {  
                    for (auto& obj : clones)
                    {
                        obj->Set_Destroy(false);
                        EVENT->Publish(FEvent_Object::Create(EEventType::Create_Object, obj));
                    }
                },
                "Paste Object"
            );
            EDITOR->ExecuteCommand(cmd);
        }
    }
}

shared_ptr<Hierarchy> Hierarchy::Create()
{
    return make_shared<Hierarchy>();
}
