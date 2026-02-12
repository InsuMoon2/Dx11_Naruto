#pragma once

namespace ed = ax::NodeEditor;

struct FBTEditorNode
{
    ed::NodeId id;
    string name;
    string nodeType;                // Sequence, Selector, Task.. 컴포짓들

    ImVec2 position;
    ed::PinId inputPin;             // 부모 연결

    vector<ed::PinId> outputPins;   // 자식 연결들
    map<string, string> parameters;

};

struct FBTEditorLink
{
    ed::LinkId id;
    ed::PinId startPinId;
    ed::PinId endPinId;
};

