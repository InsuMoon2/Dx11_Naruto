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

public:
    void Load_BehaviorTree(const string& path);
    void Save_BehaviorTree(const string& path);
    void Create_BehaviorTree();

private:
    // UI
    void Draw_ToolBar();
    void Draw_Node(const FBTEditorNode& node);
    void Draw_ContextMenu();
    void Draw_NodeInspector();

    void Draw_FilePopup();

    void Draw_Blackboard();

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

    int     Find_NodeIdByInputPin(ed::PinId pinId) const;

public:
    void    Set_DebugTarget(Weak<BehaviorTree> targetBehavior) { _debugTarget = targetBehavior; }
    void    Set_DebugMode(bool enable) { _isDebugMode = enable; }
    bool    Is_DebugMode() const { return _isDebugMode; }
    void    Clear_DebugMode();

private:
    ed::EditorContext* _editorContext = {};

    vector<FBTEditorNode> _nodes;
    vector<FBTEditorLink> _links;
    set<int>    _pendingPositions;  // 노드 위치 옮기기

    int         _nextId = 1;        // 노드 ID

    string      _currentFilePath;
    bool        _isDirty = false;
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
    map<int, EBTNodeResult> _nodeStateCache; // 매 프레임 업데이트되는 상태값 캐싱

    bool    _isDebugMode = false;

public:
    static Shared<BehaviorTree_View> Create();

};

NS_END
