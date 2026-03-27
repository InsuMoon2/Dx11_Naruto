#pragma once

#include "EditorWindow.h"
#include "UI_AnimSequencerAdapter.h"

NS_BEGIN(Engine)
class       UIObject;
class       UI_AnimPlayer;
struct      FUIAnimAsset;
struct      FUIAnimTrack;
struct      FUIAnimKey;
enum class  EUIAnimProperty;
class       RenderTarget;
NS_END

NS_BEGIN(Editor)

struct FUIAnimObjectState
{
    bool valid = false;

    float posX = 0.f;
    float posY = 0.f;
    float sizeX = 0.f;
    float sizeY = 0.f;
    float rotationZ = 0.f;
    float opacity = 1.f;
    Color tint = Color(1.f, 1.f, 1.f, 1.f);
};

class UI_Animation_View : public EditorWindow
{
    friend class UI_AnimSequencerAdapter;

public:
    explicit UI_Animation_View();
    virtual ~UI_Animation_View() = default;

public:
    void    Initialize() override;
    void    Update(float timeDelta) override;
    void    OnGui() override;

    bool    CanSave() const override;
    void    Save() override;

    void    Pre_Render() override;

public:
    void    Create_NewAnimation();
    void    Load_Animation(const string& path);
    void    Save_Animation(const string& path);

    bool    Has_Track(EUIAnimProperty property) const;
    void    Remove_Track(int trackIndex);

    static FUIAnimObjectState Capture_UIState(const Shared<UIObject>& ui);
    static void Apply_UIState(const Shared<UIObject>& ui, const FUIAnimObjectState& state);
    static void Normalize_TrackKeys(FUIAnimTrack& track);

    static fs::path Get_UIAnimFolderPath() {
        return fs::path("../../Client/Bin/Resources/Data/json/UIAnimations");
    }


private:
    void    Draw_ToolBar();
    void    Draw_FilePopup();
    void    Draw_LeftPanel();
    void    Draw_RightPanel();
    void    Draw_Sequencer();
    void    Draw_KeyInspector();
    void    Draw_TargetBinding();

    void    Refresh_PlayerBinding();
    void    Apply_CurrentFrame(int frame);
    void    Sync_TargetNameBuffer();
    void    Sync_SequencerSelection();

    void    Add_Track(EUIAnimProperty property);

    void    Add_Key_AtCurrentFrame();
    void    Delete_SelectedKey();

    FUIAnimTrack*   Get_SelectedTrack();
    FUIAnimKey*     Get_SelectedKey();

    Shared<UIObject> Find_SelectedUIObject() const;

    void    Handle_PlayerbackShortcut();
    void    Handle_EditShortcut();
    void    On_KeyFrameDragged(FUIAnimTrack& track, int keyIndex);
    void    On_KeyFrameDragFinished(FUIAnimTrack& track, int frame);

private:
    void    Draw_KeyList(FUIAnimTrack& track);
    void    Draw_UIBindingPopup();   
    void    Draw_PreviewPanel();     //preview RT 표시 UI

    int     Find_KeyIndex_ByFrame(const FUIAnimTrack& track, int frame) const; // sort 후 key 재선택용
    void    Capture_PreviewOrigin(); 
    void    Clear_PreviewOrigin();   
    void    Bind_TargetUI(Shared<UIObject> ui);

    void    Resolve_BoundTarget_FromAsset();

private:
    Shared<FUIAnimAsset>    _asset;
    Shared<UI_AnimPlayer>   _player;
    Weak<UIObject>          _boundTarget;

    FSequencerUIState       _sequencerState;
    FUIAnimSequencerContext _sequencerContext;
    UI_AnimSequencerAdapter _sequencerAdapter;

    string  _currentFilePath;
    bool    _isPlaying = false;

    int     _selectedTrackIndex = -1;
    int     _selectedKeyIndex = -1;

    bool    _isOpenFilePopup = false;
    bool    _isSaveFilePopup = false;
    char    _saveFileNameBuf[128] = {};
    char    _targetNameBuf[128] = {};



    Shared<RenderTarget>    _previewRT;
    bool    _isOpenUIBindingPopup = false;

    bool    _previewEnabled = true;       
    bool    _previewCenterMode = true;    
    float   _previewScaleMultiplier = 1.f; 

    FUIAnimObjectState _previewOriginState; 


public:
    static Shared<UI_Animation_View> Create();

};

NS_END
