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

    // 시퀀서 우클릭 시 열리는 컨텍스트 메뉴를 그린다.
    void Handle_SequencerContextMenu();
    void Handle_CreateNotifyPopup();
    void Handle_CreateStatePopup();

    // 시퀀서 우클릭 메뉴를 열 때 클릭 위치의 프레임/트랙 문맥을 저장한다.
    void Begin_SequencerContextMenu(int32 frame, bool hasTrackContext, bool isStateTrack, int32 trackIndex);
    void Begin_CreateNotifyPopup(int32 frame);
    void Begin_CreateStatePopup(int32 startFrame, int32 endFrame);
    // 시퀀서 우클릭 시 현재 문맥에 맞는 Notify 생성 팝업을 연다.
    void Open_CreateNotifyFromContext();
    // 시퀀서 우클릭 시 현재 문맥에 맞는 Notify State 생성 팝업을 연다.
    void Open_CreateStateFromContext();
    // 시퀀서 우클릭 메뉴에서 선택된 트랙 종류에 맞는 트랙을 즉시 추가한다.
    void Add_Track_FromContext(bool isStateTrack);

    void Load_NotifyAsset();
    void Save_NotifyAsset();
    void Refresh_ClipFilter();

    void Refresh_CurrentClip();
    void Apply_CurrentFrame_ToPreview();

    void Ensure_PreviewCamera();
    void Fit_PreviewCamera_ToOwner();

    void Ensure_PreviewGridResources();
    void Draw_PreviewGrid() const;

    void Handle_PlaybackShortcut();
    void Handle_DeleteShortcut();

    bool Passes_ClipSearch(const string& clipName) const;
    bool Passes_AnimStateClipFilter(const string& clipName) const;
    bool Has_SelectedNotify() const;
    bool Has_SelectedState() const;
    bool Has_SelectedNotifyTrack() const;
    bool Has_SelectedNotifyStateTrack() const;
    // 시퀀서 우클릭 좌표가 어느 트랙 row 위인지 계산한다.
    bool Try_GetSequencerTrackContext(
        const ImVec2& sequencerCanvasPos,
        const ImVec2& mousePos,
        bool& outIsStateTrack,
        int32& outTrackIndex) const;
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
    // 애니메이션 프리뷰 RT에 그리드를 그릴 때 사용하는 라인 배치 객체다.
    Shared<DirectX::PrimitiveBatch<DirectX::VertexPositionColor>> _previewGridBatch;
    // 애니메이션 프리뷰 그리드용 정점 컬러 이펙트다.
    Shared<DirectX::BasicEffect> _previewGridEffect;
    // 애니메이션 프리뷰 그리드 렌더링 시 필요한 입력 레이아웃이다.
    ComPtr<ID3D11InputLayout> _previewGridInputLayout;
    // 애니메이션 프리뷰 그리드를 항상 보이게 그리기 위한 depth off 상태다.
    ComPtr<ID3D11DepthStencilState> _previewGridDepthDisabledState;

    string                      _modelGuid;

    FAnimNotifyAsset            _asset;

    int32 _selectedClipIndex = -1;
    int32 _selectedNotifyIndex = -1;
    int32 _selectedStateIndex = -1;
    int32 _selectedNotifyTrackIndex = 0;
    int32 _selectedNotifyStateTrackIndex = 0;

    bool  _isPlaying = false;
    bool  _isPreviewHovered = false;
    // 애니메이션 프리뷰 그리드 표시 여부를 제어한다.
    bool  _showPreviewGrid = true;
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
    // 시퀀서 우클릭 컨텍스트 메뉴를 다음 프레임에 열기 위한 플래그다.
    bool _openSequencerContextPopup = false;
    bool _openCreateNotifyPopup = false;
    bool _openCreateStatePopup = false;

    // 시퀀서 우클릭 메뉴가 참조하는 기준 프레임이다.
    int32 _sequencerContextFrame = 0;
    // 시퀀서 우클릭이 특정 트랙 row 위에서 발생했는지 여부다.
    bool _sequencerContextHasTrack = false;
    // 시퀀서 우클릭이 Notify State 트랙 위였는지 기록한다.
    bool _sequencerContextIsStateTrack = false;
    // 시퀀서 우클릭이 발생한 실제 트랙 인덱스다.
    int32 _sequencerContextTrackIndex = -1;
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
