#pragma once

#include "EditorWindow.h"
#include "AnimSequencerAdapter.h"
#include "AnimNotify_Types.h"

NS_BEGIN(Engine)
class Model;
class RenderTarget;
NS_END

NS_BEGIN(Editor)

class Animation_View : public EditorWindow
{
public:
    explicit Animation_View();
    virtual ~Animation_View() = default;

public:
    void Initialize() override;
    void Update(float timeDelta) override;
    void OnGui() override;
    void Pre_Render() override;

    bool CanSave() const override;
    void Save() override;

public:
    void Open_Model(Shared<Model> model);
    void Focus_Clip(const string& clipName);

    void Add_Notify_AtFrame(int32 frame, const string& typeName);
    void Add_State_ByFrameRange(int32 startFrame, int32 endFrame, const string& typeName);

    void Delete_SelectedNotify();
    void Delete_SelectedState();

public:
    int32 Get_FrameMin() const { return 0; }
    int32 Get_FrameMax() const;

    int32 Get_CurrentClipFps() const;
    int32 Get_CurrentClipFrameMax() const;

private:
    void Draw_ToolBar();
    void Draw_LeftPanel();
    void Draw_Sequencer();
    void Draw_RightPanel();
    void Draw_PreviewPanel();

    void Draw_ClipList();
    void Draw_NotifyList();
    void Draw_StateList();

    void Draw_SelectedNotifyInspector();
    void Draw_SelectedStateInspector();

    void Draw_CreateNotifySection();
    void Draw_CreateStateSection();

    void Handle_CreateNotifyPopup();
    void Handle_CreateStatePopup();

    void Begin_CreateNotifyPopup(int32 frame);
    void Begin_CreateStatePopup(int32 startFrame, int32 endFrame);

    void Load_NotifyAsset();
    void Save_NotifyAsset();

    void Refresh_CurrentClip();
    void Apply_CurrentFrame_ToPreview();

    int32 Pixel_ToFrame_InSequencer(float pixelX, float trackMinX, float trackMaxX) const;

    FAnimNotifyClipData* Get_CurrentClip();
    const FAnimNotifyClipData* Get_CurrentClip() const;

private:
    Shared<Model>       _model;
    string              _modelGuid;

    FAnimNotifyAsset    _asset;

    int32 _selectedClipIndex = -1;
    int32 _selectedNotifyIndex = -1;
    int32 _selectedStateIndex = -1;

    bool  _isPlaying = false;
    float _previewPlaybackTimeSec = 0.f;

    FSequencerUIState       _sequencerState;
    FAnimSequencerContext   _sequencerContext;
    AnimSequencerAdapter    _sequencerAdapter;

    Shared<RenderTarget>    _previewRT;

private:
    bool _openCreateNotifyPopup = false;
    bool _openCreateStatePopup = false;

    int32 _requestedCreateNotifyFrame = 0;
    int32 _requestedCreateStateStartFrame = 0;
    int32 _requestedCreateStateEndFrame = 0;

    int32 _createNotifyTypeIndex = 0;
    int32 _createStateTypeIndex = 0;


public:
    static Shared<Animation_View> Create();
};

NS_END
