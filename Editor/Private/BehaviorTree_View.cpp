#include "pch.h"
#include "BehaviorTree_View.h"
#include <fstream>
#include <commdlg.h>
#include "Blackboard.h"
#include "BehaviorTree.h"
#include "BTNode.h"
#include "Notification_Manager.h"
#include "Reflection_Inspector.h"

BehaviorTree_View::BehaviorTree_View()
    : EditorWindow(TEXT("BehaviorTree"))
    , _nextId(1)
{
    _isActive = false;
}

BehaviorTree_View::~BehaviorTree_View()
{
    if (_editorContext)
    {
        ed::SetCurrentEditor(nullptr);
        ed::DestroyEditor(_editorContext);
        _editorContext = nullptr;
    }
}

void BehaviorTree_View::Initialize()
{
    EditorWindow::Initialize();
    Recreate_EditorContext();
    Create_BehaviorTree();
}

void BehaviorTree_View::Recreate_EditorContext()
{
    if (_editorContext)
    {
        ed::SetCurrentEditor(nullptr);
        ed::DestroyEditor(_editorContext);
        _editorContext = nullptr;
    }

    ed::Config config;
    config.SettingsFile = nullptr;

    _editorContext = ed::CreateEditor(&config);

    _selectedNodeId = ed::NodeId();
    _pendingPositions.clear();

    for (const auto& node : _nodes)
    {
        _pendingPositions.insert(node.id.Get());
    }
}

void BehaviorTree_View::Update(float timeDelta)
{
    EditorWindow::Update(timeDelta);

    if (!_debugTarget.expired())
    {
        _nodeStateCache = _debugTarget.lock()->Get_AllNodeResults();
    }
    else
    {
        _nodeStateCache.clear();
    }
}

void BehaviorTree_View::OnGui()
{
    if (!_isActive)
        return;

    string str = Utils::ToString(Get_Name());

    if (_isDirty)
        str += " *";

    str += "###" + Utils::ToString(Get_Name());

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8, 8));
    if (!ImGui::Begin(str.c_str(), &_isActive, ImGuiWindowFlags_NoDocking))
    {
        ImGui::End();
        ImGui::PopStyleVar();
        return;
    }

    if (_requestDebugSession)
    {
        Process_PendingDebugRequest();
    }

    _isFocused = ImGui::IsWindowFocused(ImGuiFocusedFlags_ChildWindows);

    Draw_ToolBar();
    ImGui::Separator();

    if (_requestEditorContextReset)
    {
        Recreate_EditorContext();
        _requestEditorContextReset = false;
    }

    // 좌측 : 노드 에디터
    ImGui::BeginChild("NodeEditorArea", ImVec2(ImGui::GetContentRegionAvail().x * 0.7f, 0), true);
    {
        ed::SetCurrentEditor(_editorContext);
        ed::Begin("BT Canvas", ImGui::GetContentRegionAvail());

        for (const auto& node : _nodes)
            Draw_Node(node);

        for (const auto& link : _links)
        {
            ImColor linkColor = ImColor(200, 200, 200, 128); 
            float linkThickness = 1.0f;

            if (_isDebugMode && !_nodeStateCache.empty())
            {
                int childNodeId = Find_NodeIdByInputPin(link.endPinId);
                if (childNodeId != -1 && _nodeStateCache.contains(childNodeId))
                {
                    EBTNodeResult childState = _nodeStateCache[childNodeId];
                    switch (childState)
                    {
                    case EBTNodeResult::Succeeded:
                        linkColor = ImColor(0, 200, 0, 255);
                        linkThickness = 2.5f;
                        break;
                    case EBTNodeResult::InProgress:
                        linkColor = ImColor(255, 200, 0, 255);
                        linkThickness = 3.0f;
                        break;
                    case EBTNodeResult::Failed:
                        linkColor = ImColor(200, 0, 0, 200);
                        linkThickness = 1.5f;
                        break;
                    }
                }
            }

            ed::Link(link.id, link.startPinId, link.endPinId, linkColor, linkThickness);
        }

        if (!_isDebugMode)
        {
            Handle_LinkCreation();
            Handle_Deletion();
            Draw_ContextMenu();
        }
        else
        {
            ed::Suspend();
            ed::ShowBackgroundContextMenu();
            ed::Resume();
        }

        Update_SelectedNode();

        for (auto& node : _nodes)
        {
            if (!_pendingPositions.contains(node.id.Get()))
            {
                node.position = ed::GetNodePosition(node.id);
            }
        }

        if (_requestNavigateToContent)
        {
            // Debug In Node Editor 진입 시 이전 그래프의 pan/zoom/selection 상태를 버리고
            // 새로 로드된 그래프 기준으로 뷰를 다시 맞춘다.
            ed::ClearSelection();
            ed::NavigateToContent(0.f);
            _requestNavigateToContent = false;
        }

        ed::End();
        ed::SetCurrentEditor(nullptr);
    }
    ImGui::EndChild();

    ImGui::SameLine();

    // 우측: Node Inspector
    ImGui::BeginChild("RightPanel", ImVec2(0, 0), false);
    {
        ImGui::BeginChild("NodeInspector", ImVec2(0, ImGui::GetContentRegionAvail().y * 0.6f), true);
        {
            if (_selectedNodeId.Get() != 0)
                Draw_NodeInspector();
            else
                ImGui::TextDisabled("No node selected");
        }
        ImGui::EndChild();

        ImGui::BeginChild("Blackboard", ImVec2(0, 0), true);
        {
            Draw_Blackboard();
        }
        ImGui::EndChild();
    }
    ImGui::EndChild();

    ImGui::End();
    ImGui::PopStyleVar();
}

bool BehaviorTree_View::CanSave() const
{
    return !_currentFilePath.empty();
}

void BehaviorTree_View::Save()
{
    Save_BehaviorTree(_currentFilePath);
    ClearDirty();

    string name = fs::path(_currentFilePath).stem().string();
    if (name.ends_with(".bt.json"))
        name = name.substr(0, name.size() - 8);

    EDITOR->Get_Notification()->Add_Notification("Behavior Tree Saved : {}", name);
}

void BehaviorTree_View::Load_BehaviorTree(const string& path)
{
    ifstream file(path);
    if (!file.is_open())
        return;

    json root;
    file >> root;

    Desirialize_FromJson(root);

    _currentFilePath = path;
    _isDirty = false;
    _requestEditorContextReset = true;
    _requestNavigateToContent = true;

    _isActive = true;
}

void BehaviorTree_View::Save_BehaviorTree(const string& path)
{
    json root = Serialize_ToJson();
    ofstream file(path);

    file << root.dump(4);

    _currentFilePath = path;
    _isDirty = false;

    GAME->Register_Asset(Utils::ToWString(path));
}

void BehaviorTree_View::Create_BehaviorTree()
{
    _nodes.clear();
    _links.clear();

    _currentFilePath.clear();
    _selectedNodeId = ed::NodeId();

    _nextId = 1;

    Create_Node("Root", ImVec2(100, 300));
    _blackboard = Blackboard::Create();

    _isDirty = false;
    _requestEditorContextReset = true;
    _requestNavigateToContent = true;
}

void BehaviorTree_View::Request_DebugSession(const string& path, Weak<BehaviorTree> targetBehavior)
{
    _pendingDebugPath = path;
    _pendingDebugTarget = targetBehavior;
    _requestDebugSession = true;
    _isActive = true;
}

void BehaviorTree_View::Set_DebugMode(bool enable)
{
    _isDebugMode = enable;
    //_requestEditorContextReset = true;
    //_requestNavigateToContent = true;
}

void BehaviorTree_View::Draw_ToolBar()
{
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(8, 4));

    if (_isDebugMode)
        ImGui::BeginDisabled();

    if (ImGui::Button(ICON_FA_FILE "  New  "))
        Create_BehaviorTree();

    ImGui::SameLine();

    if (ImGui::Button(ICON_FA_FOLDER_OPEN "  Open  "))
    {
        _isOpenFilePopup = true;
        _isSaveFilePopup = false;
        ImGui::OpenPopup("FileBrowserPopup");
    }

    ImGui::SameLine();

    if (ImGui::Button(ICON_FA_FLOPPY_DISK "  Save  "))
    {
        if (!_currentFilePath.empty())
        {
            Save_BehaviorTree(_currentFilePath);
        }
        else
        {
            _isOpenFilePopup = false;
            _isSaveFilePopup = true;
            memset(_saveFileNameBuf, 0, sizeof(_saveFileNameBuf));
            ImGui::OpenPopup("FileBrowserPopup");
        }
    }

    if (_isDebugMode)
        ImGui::EndDisabled();

    ImGui::PopStyleVar();

    if (_isDirty)
    {
        ImGui::SameLine();
        ImGui::TextColored(ImVec4(1, 0.5f, 0, 1),  " " ICON_FA_TRIANGLE_EXCLAMATION " Unsaved Changes");
    }

    if (_isDebugMode)
    {
        ImGui::SameLine();
        ImGui::TextColored(ImVec4(0.3f, 0.8f, 1.0f, 1.0f),
            ICON_FA_BUG " Debug Mode (Read-Only)");

        ImGui::SameLine();
        if (ImGui::SmallButton("Stop Debug"))
        {
            Clear_DebugMode();
        }
    }

    Draw_FilePopup();
}

void BehaviorTree_View::Draw_FilePopup()
{
    if (ImGui::BeginPopupModal("FileBrowserPopup", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
        // [경로 구하기]
        char buffer[MAX_PATH];
        GetModuleFileNameA(NULL, buffer, MAX_PATH);
        filesystem::path exeDir = filesystem::path(buffer).parent_path();
        filesystem::path projectRoot = (exeDir / "../..").lexically_normal();
        filesystem::path targetPath = projectRoot / "Client" / "Bin" / "Resources" / "Data" / "json" / "BehaviorTrees";

        if (!filesystem::exists(targetPath))
            filesystem::create_directories(targetPath);

        if (_isOpenFilePopup)
        {
            ImGui::Text("Select File to Open:");
            ImGui::Separator();

            ImGui::BeginChild("FileList", ImVec2(300, 200), true);
            {
                for (const auto& entry : filesystem::directory_iterator(targetPath))
                {
                    string filename = entry.path().filename().string();
                    if (entry.is_regular_file() && filename.ends_with(".bt.json"))
                    {
                        if (ImGui::Selectable(filename.c_str(), _selectedFileName == filename))
                        {
                            _selectedFileName = filename;
                        }

                        // 더블클릭
                        if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0))
                        {
                            Load_BehaviorTree(entry.path().string());
                            ImGui::CloseCurrentPopup();
                        }
                    }
                }
            }
            ImGui::EndChild();

            if (ImGui::Button("Open", ImVec2(120, 0)))
            {
                if (!_selectedFileName.empty())
                {
                    filesystem::path fullPath = targetPath / _selectedFileName;
                    Load_BehaviorTree(fullPath.string());
                    ImGui::CloseCurrentPopup();
                }
            }
        }
        else if (_isSaveFilePopup)
        {
            ImGui::Text("Save As:");
            ImGui::InputText("##SaveFileName", _saveFileNameBuf, sizeof(_saveFileNameBuf));

            if (ImGui::Button("Save", ImVec2(120, 0)))
            {
                string filename = _saveFileNameBuf;
                if (!filename.empty())
                {
                    if (filename.find(".bt.json") == string::npos)
                        filename += ".bt.json";

                    filesystem::path fullPath = targetPath / filename;
                    Save_BehaviorTree(fullPath.string());
                    ImGui::CloseCurrentPopup();
                }
            }
        }

        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(120, 0)))
        {
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }
}

void BehaviorTree_View::Draw_Blackboard()
{
    ImGui::Text("Blackboard");
    ImGui::Separator();

    Shared<Blackboard> displayBB = _blackboard;
    if (_isDebugMode && !_debugTarget.expired())
    {
        displayBB = _debugTarget.lock()->Get_Blackboard();
        ImGui::TextColored(ImVec4(0.3f, 0.8f, 1.0f, 1.0f), "(Live Runtime Values)");
    }

    if (!_isDebugMode)
    {
        // 새 키 추가
        ImGui::InputText("Key Name", _newKeyNameBuf, sizeof(_newKeyNameBuf));

        const char* types[] = { "Int", "Float", "Bool", "Vector3" };
        ImGui::Combo("Type", &_newKeyTypeIndex, types, IM_ARRAYSIZE(types));

        if (ImGui::Button("Add Key", ImVec2(-1, 0)))
        {
            string keyName = _newKeyNameBuf;
            if (!keyName.empty() && _blackboard)
            {
                // 타입에 따라 기본값 초기 세팅
                // Int = 0, Float = 1, Bool = 2, Vector = 3
                switch (_newKeyTypeIndex)
                {
                case 0: _blackboard->Set_ValueAsInt(keyName, 0); break;
                case 1: _blackboard->Set_ValueAsFloat(keyName, 0.f); break;
                case 2: _blackboard->Set_ValueAsBool(keyName, false); break;
                case 3: _blackboard->Set_ValueAsVector(keyName, Vec3(0.f)); break;
                }

                memset(_newKeyNameBuf, 0, sizeof(_newKeyNameBuf));
                _isDirty = true;
            }
        }
    }
    ImGui::Separator();

    if (displayBB)
    {
        if (_blackboard)
        {
            vector<FBlackboardKeyInfo> allKeys = _blackboard->Get_AllKeys();

            if (allKeys.empty())
            {
                ImGui::TextDisabled("No Key Registered");
            }
            else
            {
                for (const auto& keyInfo : allKeys)
                {
                    ImGui::PushID(keyInfo.name.c_str());

                    // 키 이름 + 타입까지 표시
                    const char* typeStr = "";
                    switch (keyInfo.type)
                    {
                    case EBlackboardValueType::Int:     typeStr = "(Int)";    break;
                    case EBlackboardValueType::Float:   typeStr = "(Float)";  break;
                    case EBlackboardValueType::Bool:    typeStr = "(Bool)";   break;
                    case EBlackboardValueType::Vector3: typeStr = "(Vec3)";   break;
                    }
                    ImGui::Text("%s %s", keyInfo.name.c_str(), typeStr);
                    ImGui::SameLine();

                    // 타입별로 값 편집
                    switch (keyInfo.type)
                    {
                    case EBlackboardValueType::Int:
                    {
                        int val = _blackboard->Get_ValueAsInt(keyInfo.name);
                        if (ImGui::InputInt("##value", &val))
                        {
                            _blackboard->Set_ValueAsInt(keyInfo.name, val);
                            _isDirty = true;
                        }
                        break;
                    }
                    case EBlackboardValueType::Float:
                    {
                        float val = _blackboard->Get_ValueAsFloat(keyInfo.name);
                        if (ImGui::InputFloat("##value", &val))
                        {
                            _blackboard->Set_ValueAsFloat(keyInfo.name, val);
                            _isDirty = true;
                        }
                        break;
                    }
                    case EBlackboardValueType::Bool:
                    {
                        bool val = _blackboard->Get_ValueAsBool(keyInfo.name);
                        if (ImGui::Checkbox("##value", &val))
                        {
                            _blackboard->Set_ValueAsBool(keyInfo.name, val);
                            _isDirty = true;
                        }
                        break;
                    }
                    case EBlackboardValueType::Vector3:
                    {
                        Vec3 val = _blackboard->Get_ValueAsVector(keyInfo.name);
                        if (ImGui::InputFloat3("##value", &val.x))
                        {
                            _blackboard->Set_ValueAsVector(keyInfo.name, val);
                            _isDirty = true;
                        }
                        break;
                    }
                    }


                    ImGui::PopID();
                }
            }
        }
    }
}

void BehaviorTree_View::Draw_Node(const FBTEditorNode& node)
{
    if (_pendingPositions.contains(node.id.Get()))
    {
        ed::SetNodePosition(node.id, node.position);
        _pendingPositions.erase(node.id.Get());
    }

    ed::PushStyleVar(ed::StyleVar_NodePadding, ImVec4(8, 8, 8, 8));
    ed::PushStyleVar(ed::StyleVar_NodeRounding, 10.0f);
    ed::PushStyleColor(ed::StyleColor_NodeBg, ImColor(80, 80, 80, 255));

    // 상태에 따른 테두리 테두리 색상 처리
    EBTNodeResult nodeState = EBTNodeResult::Failed; // 기본 Failed
    bool hasState = _nodeStateCache.contains(node.id.Get());
    if (hasState)
    {
        nodeState = _nodeStateCache[node.id.Get()];
    }

    bool shouldHightlight = hasState && nodeState != EBTNodeResult::NotExecuted;

    // 디버그 타겟이 있으면 색상 처리, 없으면 기본 회색
    if (shouldHightlight)
    {
        ImColor borderColor;
        float borderThickness = 1.f;

        switch (nodeState)
        {
        case EBTNodeResult::Succeeded:
            borderColor = ImColor(0, 255, 0, 255); 
            borderThickness = 2.f;
            break;
        case EBTNodeResult::InProgress:
            borderColor = ImColor(255, 200, 0, 255);
            borderThickness = 3.f;
            break;
        case EBTNodeResult::Failed:
            borderColor = ImColor(200, 0, 0, 200);
            borderThickness = 3.f;
            break;
        }

        ed::PushStyleColor(ed::StyleColor_NodeBorder, borderColor);
        ed::PushStyleVar(ed::StyleVar_NodeBorderWidth, borderThickness);
    }

    ed::BeginNode(node.id);
    ImGui::PushID(node.id.AsPointer());

    ImDrawList* drawList = ImGui::GetWindowDrawList();
    float nodeWidth = 140.0f;

    float pinHitBoxHeight = 24.0f; 
    float pinVisualHeight = 12.0f; 

    // Input Pin (Root 노드는 Input 없음)
    if (node.nodeType != "Root")
    {
        ImGui::PushID(node.inputPin.Get());
        ed::BeginPin(node.inputPin, ed::PinKind::Input);

        ImVec2 p = ImGui::GetCursorScreenPos();
        ImGui::Dummy(ImVec2(nodeWidth, pinHitBoxHeight));

        float yOffset = (pinHitBoxHeight - pinVisualHeight) * 0.5f;
        drawList->AddRectFilled(
            ImVec2(p.x, p.y + yOffset),
            ImVec2(p.x + nodeWidth, p.y + yOffset + pinVisualHeight),
            ImColor(50, 50, 50), 4.0f);

        ed::EndPin();
        ImGui::PopID();
    }
    else
    {
        ImGui::Dummy(ImVec2(nodeWidth, 4));
    }
    ImGui::Spacing();

    // Node Body
    {
        ImGui::PushID("Body");

        ImVec2 p = ImGui::GetCursorScreenPos();
        ImGui::Dummy(ImVec2(nodeWidth, 28));

        ImColor nodeColor = Get_NodeColor(node.nodeType);
        drawList->AddRectFilled(p, ImVec2(p.x + nodeWidth, p.y + 28), nodeColor, 4.0f);

        string name = node.name;
        ImVec2 textSize = ImGui::CalcTextSize(name.c_str());
        ImVec2 textPos = ImVec2(p.x + (nodeWidth - textSize.x) * 0.5f, p.y + (28 - textSize.y) * 0.5f);

        drawList->AddText(textPos, ImColor(255, 255, 255), name.c_str());

        ImGui::PopID();
    }
    // Output Pin
    if (!node.outputPins.empty())
    {
        ImGui::Spacing();
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));

        float pinWidth = nodeWidth / (float)node.outputPins.size();
        if (pinWidth < 10.0f) pinWidth = 10.0f;
        for (size_t i = 0; i < node.outputPins.size(); ++i)
        {
            if (i > 0) ImGui::SameLine();

            ImGui::PushID(node.outputPins[i].Get());
            ed::BeginPin(node.outputPins[i], ed::PinKind::Output);

            ImVec2 p = ImGui::GetCursorScreenPos();
            ImGui::Dummy(ImVec2(pinWidth, 12));

            drawList->AddRectFilled(p, ImVec2(p.x + pinWidth, p.y + 12), ImColor(50, 50, 50), 4.0f);

            ed::EndPin();

            ImGui::PopID();
        }
        ImGui::PopStyleVar();
    }

    ImGui::PopID();
    ed::EndNode();

    if (shouldHightlight)
    {
        ed::PopStyleColor();
        ed::PopStyleVar();
    }

    ed::PopStyleColor();
    ed::PopStyleVar(2);
}

void BehaviorTree_View::Draw_ContextMenu()
{
    ed::Suspend();

    if (ed::ShowBackgroundContextMenu())
    {
        ImGui::OpenPopup("CreateNodeMenu");

        _popupPosition = ed::ScreenToCanvas(ImGui::GetMousePos());
    }

    ed::Resume();
    ed::Suspend();

    if (ImGui::BeginPopup("CreateNodeMenu"))
    {
#pragma region Legacy

        /*if (ImGui::BeginMenu("Composite"))
        {
            if (ImGui::MenuItem("Sequence")) Create_Node("Sequence", _popupPosition);
            if (ImGui::MenuItem("Selector")) Create_Node("Selector", _popupPosition);
            if (ImGui::MenuItem("Parallel")) Create_Node("Parallel", _popupPosition);

            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Decorator"))
        {
            if (ImGui::MenuItem("Inverter")) Create_Node("Inverter", _popupPosition);
            if (ImGui::MenuItem("Repeater")) Create_Node("Repeater", _popupPosition);

            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Task"))
        {
            if (ImGui::MenuItem("Move To")) Create_Node("Task_Move", _popupPosition);
            if (ImGui::MenuItem("Attack"))  Create_Node("Task_Attack", _popupPosition);

            ImGui::EndMenu();
        }*/

#pragma endregion
        auto& registeredNode = GAME->Get_RegisteredBTNodes();

        // 정렬
        map<string, vector<string>> categoryMap;
        for (const auto& pair : registeredNode)
        {
            const string& nodeName = pair.first;
            const string& nodeCategory = pair.second.category;

            // Root 무시
            if (nodeCategory == "Hidden") continue;

            categoryMap[nodeCategory].push_back(nodeName);
        }

        for (const auto& categoryPair : categoryMap)
        {
            const string& categoryName = categoryPair.first;
            const vector<string>& nodeNames = categoryPair.second;

            if (ImGui::BeginMenu(categoryName.c_str()))
            {
                for (const string& nodeName : nodeNames)
                {
                    if (ImGui::MenuItem(nodeName.c_str())) 
                    {
                        Create_Node(nodeName, _popupPosition);
                    }
                }
                ImGui::EndMenu();
            }
        }

        ImGui::EndPopup();
    }
    ed::Resume();
}

void BehaviorTree_View::Draw_NodeInspector()
{
    ImGui::Text("Node Inspector");
    ImGui::Separator();

    // 선택된 노드 찾기
    FBTEditorNode* selectedNode = nullptr;

    for (auto& node : _nodes)
    {
        if (node.id == _selectedNodeId)
        {
            selectedNode = &node;
            break;
        }
    }

    if (!selectedNode)
        return;

    if (_isDebugMode)
        ImGui::BeginDisabled();

    ImGui::Text("Node Type: %s", selectedNode->nodeType.c_str());

    // 이름 편집
    char nameBuf[128];
    strcpy_s(nameBuf, selectedNode->name.c_str());

    if (ImGui::InputText("Name", nameBuf, sizeof(nameBuf)))
    {
        selectedNode->name = nameBuf;
        _isDirty = true;
    }

    ImGui::Separator();

    // 파라미터 편집
    ImGui::Text("Parameters:");
    ImGui::Separator();
#pragma region Legacy

    /*for (auto& [key, value] : selectedNode->parameters)
    {
        vector<FBlackboardKeyInfo> allKeys = _blackboard->Get_AllKeys();

        if (ImGui::BeginCombo(key.c_str(), value.c_str()))
        {
            for (const auto& keyInfo : allKeys)
            {
                bool isSelected = (value == keyInfo.name);
                if (ImGui::Selectable(keyInfo.name.c_str(), isSelected))
                {
                    value = keyInfo.name;
                    _isDirty = true;
                }
            }
            ImGui::EndCombo();
        }
        else
        {
            char valueBuf[256];
            strcpy_s(valueBuf, value.c_str());
            if (ImGui::InputText(key.c_str(), valueBuf, sizeof(valueBuf)))
            {
                value = valueBuf;
                _isDirty = true;
            }
        }
    }*/

#pragma endregion
    if (selectedNode->runtimeInstance)
    {
        const auto& reflInfo = selectedNode->runtimeInstance->GetReflectionInfo();
        if (!reflInfo.properties.empty())
        {
            Reflection_Inspector::Draw_Properties_Only(
                selectedNode->runtimeInstance.get(), reflInfo);
        }

        // 이제는 클라랑 ImGui랑 분리시켜놔서 사용 불가
        //selectedNode->runtimeInstance->OnDraw_Inspector();
    }

    if (_isDebugMode)
        ImGui::EndDisabled();

}

void BehaviorTree_View::Create_Node(const string& nodeType, ImVec2 position)
{
    _nodes.push_back(FBTEditorNode());

    FBTEditorNode& newNode = _nodes.back();

    newNode.id = ed::NodeId(_nextId++);
    newNode.name = nodeType;
    newNode.nodeType = nodeType;
    newNode.position = position;
    newNode.runtimeInstance = GAME->Instantiate_BTNode(nodeType);

    if (nodeType == "Root")
        newNode.inputPin = ed::PinId(0);
    else
        newNode.inputPin = ed::PinId(_nextId++);

    int outputCount = 0;
    if (nodeType == "Root")
    {
        outputCount = 1;
    }
    else if (nodeType == "Sequence" || nodeType == "Selector" || nodeType == "Parallel")
    {
        outputCount = 3;
    }
    else if (nodeType == "Inverter" || nodeType == "Repeater")
    {
        outputCount = 1;
    }

    // Task는 0개 (리프 노드)
    for (int i = 0; i < outputCount; ++i)
    {
        newNode.outputPins.push_back(ed::PinId(_nextId++));
    }

    //_nodes.push_back(newNode);
    _isDirty = true;

    _pendingPositions.insert(newNode.id.Get());
}

void BehaviorTree_View::Delete_Node(ed::NodeId nodeId)
{
    // Root 노드 삭제 방지
    auto rootNodeIter = find_if(_nodes.begin(), _nodes.end(),
        [nodeId](const FBTEditorNode& node) { return node.id == nodeId; });

    if (rootNodeIter != _nodes.end())
    {
        if (rootNodeIter->nodeType == "Root")
            return; // 삭제 불가
    }

    // 노드 제거
    auto nodeIter = remove_if(_nodes.begin(), _nodes.end(),
        [nodeId](const FBTEditorNode& node)
        {
            return node.id == nodeId;
        });

    if (nodeIter != _nodes.end())
    {
        _nodes.erase(nodeIter, _nodes.end());
        _isDirty = true;
    }

    // 관련된 링크도 제거
    auto linkIter = remove_if(_links.begin(), _links.end(),
        [this, nodeId](const FBTEditorLink& link)
        {
            // 노드와 연결된 Pin이 있는지 확인
            for (const auto& node : _nodes)
            {
                if (node.id == nodeId)
                {
                    // Input/Ouput 둘 중 하나라도 매칭된다면, 제거
                    if (link.startPinId == node.inputPin || link.endPinId == node.inputPin)
                        return true;

                    for (const auto& pin : node.outputPins)
                    {
                        if (link.startPinId == pin || link.endPinId == pin)
                            return true;
                    }
                }
            }
            return false;
        });

    if (linkIter != _links.end())
    {
        _links.erase(linkIter, _links.end());
    }

}

void BehaviorTree_View::Create_Link(ed::PinId startPin, ed::PinId endPin)
{
    FBTEditorLink newLink;
    newLink.id = ed::LinkId(_nextId++);
    newLink.startPinId = startPin;
    newLink.endPinId = endPin;

    _links.push_back(newLink);
    _isDirty = true;
}

void BehaviorTree_View::Delete_Link(ed::LinkId linkId)
{
    auto it = remove_if(_links.begin(), _links.end(),
        [linkId](const FBTEditorLink& link) { return link.id == linkId; });

    if (it != _links.end())
    {
        _links.erase(it, _links.end());
        _isDirty = true;
    }
}

void BehaviorTree_View::Handle_LinkCreation()
{
    ed::BeginCreate();
    {
        ed::PinId startPinId, endPinId;

        if (ed::QueryNewLink(&startPinId, &endPinId))
        {
            // Ouput -> Input 방향인지 검증
            if (startPinId && endPinId)
            {
                if (ed::AcceptNewItem())
                {
                    Create_Link(startPinId, endPinId);
                }
            }
        }
        ed::EndCreate();
    }
}

void BehaviorTree_View::Handle_Deletion()
{
    ed::BeginDelete();
    {

        // 노드 삭제
        ed::NodeId nodeId;

        if (ed::QueryDeletedNode(&nodeId))
        {
            if (ed::AcceptDeletedItem())
            {
                Delete_Node(nodeId);

                if (_selectedNodeId == nodeId)
                    _selectedNodeId = ed::NodeId();
            }
        }

        // 링크 삭제
        ed::LinkId linkId;
        if (ed::QueryDeletedLink(&linkId))
        {
            if (ed::AcceptDeletedItem())
            {
                Delete_Link(linkId);
            }
        }
        ed::EndDelete();
    }
}

void BehaviorTree_View::Update_SelectedNode()
{
    if (ed::GetSelectedObjectCount() == 1)
    {
        ed::NodeId selectedNodes[1];
        ed::GetSelectedNodes(selectedNodes, 1);

        _selectedNodeId = selectedNodes[0];
    }
    else if (ed::GetSelectedObjectCount() == 0)
    {
        _selectedNodeId = ed::NodeId();
    }
}

void BehaviorTree_View::Process_PendingDebugRequest()
{
    _requestDebugSession = false;

    if (!_pendingDebugPath.empty() && _pendingDebugPath != _currentFilePath)
    {
        Load_BehaviorTree(_pendingDebugPath);
    }

    _debugTarget = _pendingDebugTarget;
    _pendingDebugTarget.reset();
    _pendingDebugPath.clear();

    Set_DebugMode(true);
}

json BehaviorTree_View::Serialize_ToJson() const
{
    json root;

    // Root 노드 ID 찾기
    int rootId = -1;
    for (const auto& node : _nodes)
    {
        if (node.nodeType == "Root")
        {
            rootId = node.id.Get();
            break;
        }
    }
    root["root_node_id"] = rootId;

    // TODO : 블랙보드 경로 추가 예정
    // root["blackboard_path] = "blackboard.json"

    json nodesArray = json::array();

    for (const auto& node : _nodes)
    {
        json nodeJson;
        nodeJson["id"] = node.id.Get();
        nodeJson["name"] = node.name;
        nodeJson["type"] = node.nodeType;

        nodeJson["pos_x"] = node.position.x;
        nodeJson["pos_y"] = node.position.y;

        nodeJson["parameters"] = node.parameters;

        if (node.runtimeInstance)
        {
            json runtimeData = node.runtimeInstance->Serialize_ToJson();
            nodeJson.merge_patch(runtimeData);
        }

        // 핀 ID 저장
        nodeJson["input_pin_id"] = node.inputPin.Get();

        json outPinsArray = json::array();

        for (const auto& pin : node.outputPins)
            outPinsArray.push_back(pin.Get());

        nodeJson["output_pin_ids"] = outPinsArray;
        nodesArray.push_back(nodeJson);
    }
    root["nodes"] = nodesArray;

    json linksArray = json::array();

    for (const auto& link : _links)
    {
        json linkJson;

        linkJson["id"] = link.id.Get();
        linkJson["start"] = link.startPinId.Get();
        linkJson["end"] = link.endPinId.Get();
        linksArray.push_back(linkJson);
    }

    root["links"] = linksArray;

    if (_blackboard)
    {
        root["blackboard"] = _blackboard->Serialize_ToJson();
    }

    return root;
}

void BehaviorTree_View::Desirialize_FromJson(const json& jsonRoot)
{
    _nodes.clear();
    _links.clear();

    int maxId = 0;
    bool hasRootNode = false;

    if (jsonRoot.contains("nodes"))
    {
        for (const auto& nodeJson : jsonRoot["nodes"])
        {
            FBTEditorNode node;

            int nodeIdVal = nodeJson["id"].get<int>();
            node.id = ed::NodeId(nodeIdVal);
            maxId = max(maxId, nodeIdVal);
            node.name = nodeJson["name"].get<string>();
            node.nodeType = nodeJson["type"].get<string>();

            // Root 노드 발견 체크
            if (node.nodeType == "Root")
                hasRootNode = true;

            node.position.x = nodeJson["pos_x"].get<float>();
            node.position.y = nodeJson["pos_y"].get<float>();

            if (nodeJson.contains("parameters"))
                node.parameters = nodeJson["parameters"].get<map<string, string>>();

            if (nodeJson.contains("input_pin_id"))
            {
                int inId = nodeJson["input_pin_id"].get<int>();

                node.inputPin = ed::PinId(inId);
                maxId = max(maxId, inId);
            }

            if (nodeJson.contains("output_pin_ids"))
            {
                for (const auto& pinJson : nodeJson["output_pin_ids"])
                {
                    int outId = pinJson.get<int>();

                    node.outputPins.push_back(ed::PinId(outId));
                    maxId = max(maxId, outId);
                }
            }

            node.runtimeInstance = GAME->Instantiate_BTNode(node.nodeType);
            if (node.runtimeInstance)
            {
                node.runtimeInstance->Deserialize_FromJson(nodeJson);
            }

            _nodes.push_back(node);
        }
    }

    // 링크 로드
    if (jsonRoot.contains("links"))
    {
        for (const auto& linkJson : jsonRoot["links"])
        {
            FBTEditorLink link;

            int linkIdVal = linkJson["id"].get<int>();
            link.id = ed::LinkId(linkIdVal);
            link.startPinId = ed::PinId(linkJson["start"].get<int>());
            link.endPinId = ed::PinId(linkJson["end"].get<int>());

            _links.push_back(link);
            maxId = max(maxId, linkIdVal);
        }
    }

    _nextId = maxId + 1;

    if (!hasRootNode)
    {
        Create_Node("Root", ImVec2(100, 300));
    }

    // 로드된 노드들 대기열에 추가 -> 불러올 때 위치세팅되게
    for (const auto& node : _nodes)
    {
        _pendingPositions.insert(node.id.Get());
    }

    _isDirty = false;

    if (jsonRoot.contains("blackboard"))
    {
        if (!_blackboard)
            _blackboard = Blackboard::Create();

        _blackboard->Deserialize_FromJson(jsonRoot["blackboard"]);
    }
}

ImColor BehaviorTree_View::Get_NodeColor(const string& nodeType) const
{
    if (nodeType.find("Root") != string::npos)
    {
        return ImColor(30, 30, 30);     // 루트는 회색으로
    }

    else if (nodeType.find("Sequence") != string::npos ||
        nodeType.find("Selector") != string::npos ||
        nodeType.find("Parallel") != string::npos)
    {
        return ImColor(50, 100, 150);  // 파랑 (Composite)
    }
    else if (nodeType.find("Inverter") != string::npos ||
             nodeType.find("Repeater") != string::npos)
    {
        return ImColor(150, 100, 50);  // 주황 (Decorator)

    }
    else
    {
        return ImColor(120, 70, 180);   // 보라 (Task)
    }
}

int BehaviorTree_View::Find_NodeIdByInputPin(ed::PinId pinId) const
{
    for (const auto& node : _nodes)
    {
        if (node.inputPin == pinId)
            return node.id.Get();
    }

    return -1;
}

void BehaviorTree_View::Clear_DebugMode()
{
    _isDebugMode = false;
    _debugTarget.reset();
    _nodeStateCache.clear();
    _requestEditorContextReset = true;
    _requestNavigateToContent = true;
}

Shared<BehaviorTree_View> BehaviorTree_View::Create()
{
    auto instance = make_shared<BehaviorTree_View>();
    instance->Initialize();

    return instance;
}
