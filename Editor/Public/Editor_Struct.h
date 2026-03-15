#pragma once

namespace ed = ax::NodeEditor;

NS_BEGIN(Engine)
    struct FUIAnimAsset;
    class BTNode;
NS_END

NS_BEGIN(Editor)

class UI_Animation_View;

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

struct FSequencerUIState
{
    int     currentFrame = 0;
    int     selectedEntry = -1;
    int     firstFrame = 0;
    bool    expanded = true;
};

struct FUIAnimSequencerContext
{
    UI_Animation_View*      view = nullptr;
    Shared<FUIAnimAsset>*   asset = nullptr;

    int* selectedTrackIndex = nullptr;
    int* selectedKeyIndex = nullptr;

};

NS_END
