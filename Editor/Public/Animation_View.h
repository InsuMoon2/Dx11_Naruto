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
    // 시퀀서 타임라인에서 특정 Notify 인덱스가 현재 선택 집합에 포함되는지 확인할 때 호출한다.
    bool Is_NotifyIndexSelected(int32 notifyIndex) const;
    // 시퀀서 타임라인에서 특정 Notify State 인덱스가 현재 선택 집합에 포함되는지 확인할 때 호출한다.
    bool Is_StateIndexSelected(int32 stateIndex) const;
    // 시퀀서 타임라인에서 Notify 바를 클릭했을 때 Ctrl 다중선택 규칙까지 포함해 선택을 갱신할 때 호출한다.
    void Handle_NotifySelectionFromSequencer(int32 notifyIndex, int32 trackIndex, bool isCtrlHeld);
    // 시퀀서 타임라인에서 Notify State 바를 클릭했을 때 Ctrl 다중선택 규칙까지 포함해 선택을 갱신할 때 호출한다.
    void Handle_StateSelectionFromSequencer(int32 stateIndex, int32 trackIndex, bool isCtrlHeld);
    // 시퀀서 타임라인의 빈 공간을 클릭해 Notify 선택을 해제하고 현재 트랙만 유지할 때 호출한다.
    void Clear_NotifySelectionFromSequencer(int32 trackIndex);
    // 시퀀서 타임라인의 빈 공간을 클릭해 Notify State 선택을 해제하고 현재 트랙만 유지할 때 호출한다.
    void Clear_StateSelectionFromSequencer(int32 trackIndex);

private:
    // 애니메이션 뷰를 닫거나 대상 모델을 바꿀 때 전용 프리뷰 clone과 프리팹 뷰 연동 상태를 정리할 때 호출한다.
    void Close_ViewSession();
    // 전달받은 소스 모델 owner를 기반으로 애니메이션 뷰 전용 프리뷰 clone을 만들 때 호출한다.
    Shared<GameObject> Create_PreviewOwnerFromSourceModel(Shared<Model> sourceModel);
    // 애니메이션 뷰가 들고 있는 프리뷰 owner에서 실제 샘플링 대상 Model 컴포넌트를 찾을 때 호출한다.
    Shared<Model> Find_PreviewModel() const;
    // 애니메이션 뷰가 같은 프리팹을 보고 있는 동안 프리팹 뷰 live preview를 일시정지/해제할 때 호출한다.
    void Update_PrefabPreviewSuspension(bool suspend);
    // 현재 프리뷰 패널 크기를 다음 Pre_Render에서 사용할 RT 크기로 반영할 때 호출한다.
    void Update_PreviewRenderTargetRequest(const ImVec2& panelSize);

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
    // 애니메이션 뷰에서 Ctrl+C / Ctrl+V 단축키를 처리한다.
    void Handle_CopyPasteShortcut();
    // 현재 모델의 전체 애니메이션 이름 목록을 정렬 캐시로 다시 만들 때 호출한다.
    void Rebuild_ClipSortCache();
    // 검색어/ShowAll/AnimState 필터 기준으로 화면에 보여줄 애니메이션 목록만 다시 계산할 때 호출한다.
    void Refresh_VisibleClipEntries();

    bool Passes_ClipSearch(const string& clipName) const;
    bool Passes_AnimStateClipFilter(const string& clipName) const;
    bool Has_SelectedNotify() const;
    bool Has_SelectedState() const;
    bool Has_SelectedNotifyTrack() const;
    bool Has_SelectedNotifyStateTrack() const;
    // 현재 단일/다중 선택 상태를 합쳐 복사 대상 Notify 인덱스 목록을 정리할 때 호출한다.
    vector<int32> Collect_SelectedNotifyIndices() const;
    // 현재 단일/다중 선택 상태를 합쳐 복사 대상 Notify State 인덱스 목록을 정리할 때 호출한다.
    vector<int32> Collect_SelectedStateIndices() const;
    // Notify 항목 클릭 시 Ctrl 다중선택과 단일선택을 공통 처리할 때 호출한다.
    void Handle_NotifySelection(int32 notifyIndex, int32 trackIndex, bool isCtrlHeld);
    // Notify State 항목 클릭 시 Ctrl 다중선택과 단일선택을 공통 처리할 때 호출한다.
    void Handle_StateSelection(int32 stateIndex, int32 trackIndex, bool isCtrlHeld);
    // 붙여넣기 전에 Notify 트랙 개수가 필요한 수보다 부족하면 자동으로 확장할 때 호출한다.
    void Ensure_NotifyTrackCount(int32 requiredTrackCount);
    // 붙여넣기 전에 Notify State 트랙 개수가 필요한 수보다 부족하면 자동으로 확장할 때 호출한다.
    void Ensure_StateTrackCount(int32 requiredTrackCount);
    // 현재 클립의 Notify 배열을 트랙/시간 기준으로 정렬할 때 호출한다.
    void Sort_CurrentClipNotifies();
    // 현재 클립의 Notify State 배열을 트랙/시간 기준으로 정렬할 때 호출한다.
    void Sort_CurrentClipStates();
    // 시퀀서 우클릭 좌표가 어느 트랙 row 위인지 계산한다.
    bool Try_GetSequencerTrackContext(
        const ImVec2& sequencerCanvasPos,
        const ImVec2& mousePos,
        bool& outIsStateTrack,
        int32& outTrackIndex) const;
    void Clear_SelectedEntries();
    void Select_NotifyTrack(int32 trackIndex);
    void Select_NotifyStateTrack(int32 trackIndex);

    // 현재 선택된 단일 Notify를 내부 복사 버퍼에 저장한다.
    void Copy_SelectedNotify();
    // 현재 선택된 Notify State를 내부 복사 버퍼에 저장한다.
    void Copy_SelectedNotifyState();
    // 복사 버퍼에 저장된 Notify를 현재 클립/프레임/트랙 기준으로 붙여넣는다.
    void Paste_CopiedNotify();
    // 복사 버퍼에 저장된 Notify State를 현재 클립/프레임/트랙 기준으로 붙여넣는다.
    void Paste_CopiedNotifyState();

    FAnimNotifyClipData* Get_CurrentClip();
    const FAnimNotifyClipData* Get_CurrentClip() const;

    float Get_TopPanelHeight() const;
    float Get_TopChildHeight() const;

    void Start_CurrentClipPlaybackFromFrame(int32 frame);

private:
    // 애니메이션 클립 브라우저의 정렬/필터 캐시에 사용하는 고정 메타데이터다.
    struct FClipListEntry
    {
        // 모델 내부 원본 애니메이션 인덱스다.
        uint32 index = 0;
        // 화면 표시와 검색에 같이 쓰는 애니메이션 이름이다.
        string label;
    };

    // 다중 복사한 Notify 한 개의 타입/페이로드/원래 배치 위치를 저장하는 클립보드 엔트리다.
    struct FCopiedNotifyEntry
    {
        string typeName = "";
        json payload = json::object();
        float timeSec = 0.f;
        int32 trackIndex = 0;
    };

    // 다중 복사한 Notify State 한 개의 타입/페이로드/길이/원래 배치 위치를 저장하는 클립보드 엔트리다.
    struct FCopiedNotifyStateEntry
    {
        string typeName = "";
        json payload = json::object();
        float startSec = 0.f;
        float durationSec = 0.f;
        int32 trackIndex = 0;
    };

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
    // Ctrl 다중선택으로 잡힌 Notify 인덱스 집합이다.
    unordered_set<int32> _selectedNotifyIndices;
    // Ctrl 다중선택으로 잡힌 Notify State 인덱스 집합이다.
    unordered_set<int32> _selectedStateIndices;

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
    // 다음 프레임 Pre_Render에서 사용할 애니메이션 프리뷰 RT 목표 크기다.
    ImVec2 _previewRTRequestedSize = ImVec2(0.f, 0.f);

    string _clipSearchText;
    unordered_set<string> _animStateClipNames;
    bool _showAllClips = false;
    // 모델의 전체 애니메이션 목록을 이름 기준으로 한 번만 정렬해 보관하는 캐시다.
    vector<FClipListEntry> _sortedClipEntries;
    // 현재 검색/필터 조건을 통과한 정렬 캐시 엔트리 인덱스 목록이다.
    vector<uint32> _visibleClipIndices;
    // 다음 Draw_ClipList 전에 전체 애니메이션 정렬 캐시를 다시 만들어야 하는지 표시한다.
    bool _clipSortCacheDirty = true;
    // 애니메이션 뷰가 현재 점유 중인 원본 프리팹 이름이다.
    string _sourcePrefabName;
    // ImGui가 현재 프레임 draw list를 소비하기 전 GPU 자원을 해제하지 않도록 close cleanup을 다음 프레임으로 미룰 때 사용한다.
    bool _pendingCloseViewSession = false;
    // pending close를 처리한 프레임에는 OnGui를 건너뛰어 이미 종료된 창을 다시 그리지 않도록 제어한다.
    bool _skipGuiThisFrame = false;
    // 애니메이션 뷰가 프리뷰 owner를 직접 생성해서 Prefab 레벨에 등록했는지 추적한다.
    bool _ownsPreviewObject = false;
    // 애니메이션 뷰 전용 프리뷰 owner에 BeginPlay를 한 번만 호출하기 위한 플래그다.
    bool _previewHasBegunPlay = false;

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

private:
    // 복사 버퍼에 현재 무엇이 들어 있는지 구분하기 위한 타입이다.
    enum class ENotifyClipboardKind
    {
        None,
        Notify,
        NotifyState
    };

    // 내부 복사 버퍼에 저장된 엔트리 종류를 기록한다.
    ENotifyClipboardKind _copiedNotifyKind = ENotifyClipboardKind::None;
    // 다중 복사한 Notify 엔트리들의 타입/페이로드/원래 배치 정보를 저장한다.
    vector<FCopiedNotifyEntry> _copiedNotifyEntries;
    // 다중 복사한 Notify State 엔트리들의 타입/페이로드/원래 배치 정보를 저장한다.
    vector<FCopiedNotifyStateEntry> _copiedNotifyStateEntries;

public:
    static Shared<Animation_View> Create();
};

NS_END
