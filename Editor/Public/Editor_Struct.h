#pragma once

namespace ed = ax::NodeEditor;

NS_BEGIN(Engine)
class BTNode;
NS_END

NS_BEGIN(Editor)

struct FBTEditorNode
{
    ed::NodeId id;
    string name;
    string nodeType;                // Sequence, Selector, Task.. 컴포짓들

    ImVec2 position;
    ed::PinId inputPin;             // 부모 연결

    vector<ed::PinId> outputPins;   // 자식 연결들
    map<string, string> parameters;

    Shared<BTNode> runtimeInstance;
};

struct FBTEditorLink
{
    ed::LinkId id;
    ed::PinId startPinId;
    ed::PinId endPinId;
};

NS_END
