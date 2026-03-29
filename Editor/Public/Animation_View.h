#pragma once

#include "EditorWindow.h"
#include "AnimSequencerAdapter.h"
#include "AnimNotify_Types.h"
#include <unordered_set>

NS_BEGIN(Engine)
class Model;
class RenderTarget;
class GameObject;
NS_END

NS_BEGIN(Editor)

class Editor_Camera_Free;

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

    void Add_NotifyTrack(const string& trackName);
    void Add_NotifyStateTrack(const string& trackName);

    void Add_Notify_AtFrame(int32 frame, const string& typeName);
    void Add_State_ByFrameRange(int32 startFrame, int32 endFrame, const string& typeName);

    void Delete_SelectedNotify();
    void Delete_SelectedState();
    void Delete_SelectedNotifyTrack();
    void Delete_SelectedNotifyStateTrack();

public:
    int32 Get_FrameMin() const { return 0; }
    int32 Get_FrameMax() const;

    int32 Get_CurrentClipFps() const;
    int32 Get_CurrentClipFrameMax() const;
    float Get_CurrentFrameTimeSec() const;

    float Get_CurrentClipLengthSec() const;

    int32 Pixel_ToFrame_InSequencer(float pixelX, float trackMinX, float trackMaxX) const;

private:
    void Draw_ToolBar();
    void Draw_TopLayout();
    void Draw_BottomLayout();

    void Draw_ClipBrowserPanel();
    void Draw_PreviewPanel();
    void Draw_SelectedDetailPanel();

    void Draw_EventListPanel();
    void Draw_Sequencer();
    void Draw_CreatePanel();

    void Draw_ClipList();
    void Draw_NotifyList();
    void Draw_StateList();

    void Draw_SelectedNotifyInspector();
    void Draw_SelectedStateInspector();

    void Draw_NotifyTrackSection();
    void Draw_NotifyStateTrackSection();
    void Draw_CreateNotifySection();
    void Draw_CreateStateSection();

    void Handle_CreateNotifyPopup();
    void Handle_CreateStatePopup();

    void Begin_CreateNotifyPopup(int32 frame);
    void Begin_CreateStatePopup(int32 startFrame, int32 endFrame);

    void Load_NotifyAsset();
    void Save_NotifyAsset();
    void Refresh_ClipFilter();

    void Refresh_CurrentClip();
    void Apply_CurrentFrame_ToPreview();

    void Ensure_PreviewCamera();
    void Fit_PreviewCamera_ToOwner();
    void Handle_PlaybackShortcut();
    void Handle_DeleteShortcut();

    bool Passes_ClipSearch(const string& clipName) const;
    bool Passes_AnimStateClipFilter(const string& clipName) const;
    bool Has_SelectedNotify() const;
    bool Has_SelectedState() const;
    bool Has_SelectedNotifyTrack() const;
    bool Has_SelectedNotifyStateTrack() const;
    void Clear_SelectedEntries();
    void Select_NotifyTrack(int32 trackIndex);
    void Select_NotifyStateTrack(int32 trackIndex);

    FAnimNotifyClipData* Get_CurrentClip();
    const FAnimNotifyClipData* Get_CurrentClip() const;

    float Get_TopPanelHeight() const;
    float Get_TopChildHeight() const;

    void Start_CurrentClipPlaybackFromFrame(int32 frame);

private:
    Shared<Model>               _model;
    Shared<GameObject>          _previewOwner;
    Shared<Editor_Camera_Free>  _previewCamera;
    Shared<RenderTarget>        _previewRT;

    string                      _modelGuid;

    FAnimNotifyAsset            _asset;

    int32 _selectedClipIndex = -1;
    int32 _selectedNotifyIndex = -1;
    int32 _selectedStateIndex = -1;
    int32 _selectedNotifyTrackIndex = 0;
    int32 _selectedNotifyStateTrackIndex = 0;

    bool  _isPlaying = false;
    bool  _isPreviewHovered = false;
    float _previewPlaybackTimeSec = 0.f;

    FSequencerUIState       _sequencerState;
    FAnimSequencerContext   _sequencerContext;
    AnimSequencerAdapter    _sequencerAdapter;

    Matrix _previewView = Matrix::Identity;
    Matrix _previewProj = Matrix::Identity;

    ImVec2 _previewScreenPos = ImVec2(0.f, 0.f);
    ImVec2 _previewImGuiSize = ImVec2(0.f, 0.f);

    string _clipSearchText;
    unordered_set<string> _animStateClipNames;
    bool _showAllClips = false;

private:
    bool _openCreateNotifyPopup = false;
    bool _openCreateStatePopup = false;

    int32 _requestedCreateNotifyFrame = 0;
    int32 _requestedCreateStateStartFrame = 0;
    int32 _requestedCreateStateEndFrame = 0;

    int32 _createNotifyTypeIndex = 0;
    int32 _createStateTypeIndex = 0;

    string _newNotifyTrackName = "Notify Track";
    string _newNotifyStateTrackName = "Notify State Track";

public:
    static Shared<Animation_View> Create();
};

NS_END
