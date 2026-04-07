#pragma once

#include "EditorWindow.h"
#include "Editor_Struct.h"

namespace ed = ax::NodeEditor;

NS_BEGIN(Engine)
class Blackboard;
class BehaviorTree;
NS_END

NS_BEGIN(Editor)

class BehaviorTree_View : public EditorWindow
{
public:
    explicit BehaviorTree_View();
    virtual ~BehaviorTree_View();

public:
    void Initialize() override;
    void Update(float timeDelta) override;
    void OnGui() override;

    bool CanSave() const override;
    void Save() override;

public:
    void Load_BehaviorTree(const string& path);
    void Save_BehaviorTree(const string& path);
    void Create_BehaviorTree();
    void Request_DebugSession(const string& path, Weak<BehaviorTree> targetBehavior);
    void Set_DebugMode(bool enable);

private:
    // UI
    void Draw_ToolBar();
    void Draw_Node(const FBTEditorNode& node);
    void Draw_ContextMenu();
    void Draw_NodeInspector();

    void Draw_FilePopup();

    void Draw_Blackboard();
    void Update_SelectedNode();
    void Process_PendingDebugRequest();

    // 노드 관리
    void Create_Node(const string& nodeType, ImVec2 position);
    void Delete_Node(ed::NodeId nodeId);

    void Create_Link(ed::PinId startPin, ed::PinId endPin);
    void Delete_Link(ed::LinkId linkId);

    void Handle_LinkCreation();
    void Handle_Deletion();

    // 직렬화
    json Serialize_ToJson() const;
    void Desirialize_FromJson(const json& json);

    ImColor Get_NodeColor(const string& nodeType) const;

    bool    Is_InputPin(ed::PinId pinId) const;
    bool    Is_OutputPin(ed::PinId pinId) const;
    ed::NodeId Find_NodeIdByPin(ed::PinId pinId) const;

    int     Find_NodeIdByInputPin(ed::PinId pinId) const;
    void    Recreate_EditorContext();

public:
    void    Set_DebugTarget(Weak<BehaviorTree> targetBehavior) { _debugTarget = targetBehavior; }
    bool    Is_DebugMode() const { return _isDebugMode; }
    void    Clear_DebugMode();

private:
    ed::EditorContext* _editorContext = {};

    vector<FBTEditorNode> _nodes;
    vector<FBTEditorLink> _links;
    set<int>    _pendingPositions;  // 노드 위치 옮기기

    int         _nextId = 1;        // 노드 ID

    string      _currentFilePath;
    ed::NodeId  _selectedNodeId = 0;

    ImVec2      _popupPosition;

    // Popup 열기
    bool        _isOpenFilePopup = false;
    bool        _isSaveFilePopup = false;
    string      _selectedFileName;
    char        _saveFileNameBuf[128] = "";

    const ed::NodeId _rootNodeId = 1;

private: /* Blackboard */
    Shared<Blackboard> _blackboard;

    char    _newKeyNameBuf[64] = "";

    int     _newKeyTypeIndex = 0;

    // 현재 보고있는 AI의 Behavior
    Weak<BehaviorTree> _debugTarget;
    Weak<BehaviorTree> _pendingDebugTarget;
    map<int, EBTNodeResult> _nodeStateCache; // 매 프레임 업데이트되는 상태값 캐싱
    string  _pendingDebugPath;

    bool    _isDebugMode = false;
    bool    _requestDebugSession = false;
    bool    _requestEditorContextReset = false;
    bool    _requestNavigateToContent = false;

public:
    static Shared<BehaviorTree_View> Create();

};

NS_END
