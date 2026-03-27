#include "pch.h"
#include "UI_Animation_View.h"
#include "RenderTarget.h"
#include "Hierarchy.h"
#include "UIObject.h"
#include "UI_AnimPlayer.h"
#include "UI_AnimSerializer.h"
#include "UI_AnimUtility.h"

UI_Animation_View::UI_Animation_View()
    : EditorWindow(TEXT("UI Animation"))
    , _sequencerAdapter(&_sequencerState, &_sequencerContext)
{
}

void UI_Animation_View::Initialize()
{
    EditorWindow::Initialize();

    _sequencerContext.view = this;
    _sequencerContext.asset = &_asset;
    _sequencerContext.selectedTrackIndex = &_selectedTrackIndex;
    _sequencerContext.selectedKeyIndex = &_selectedKeyIndex;

    _player = UI_AnimPlayer::Create();

    memset(_saveFileNameBuf, 0, sizeof(_saveFileNameBuf));
    memset(_targetNameBuf, 0, sizeof(_targetNameBuf));

    Create_NewAnimation();

    _isActive = false;
}

void UI_Animation_View::Update(float timeDelta)
{
    EditorWindow::Update(timeDelta);

    if (!_asset || !_player)
        return;

    if (_player->Is_Playing())
    {
        _player->Update(timeDelta);
    }

    _isPlaying = _player->Is_Playing();
    _sequencerState.currentFrame = _player->Get_CurrentFrame();

    Sync_SequencerSelection();
}

void UI_Animation_View::OnGui()
{
    string title = Utils::ToString(Get_Name());

    if (!ImGui::Begin(title.c_str(), &_isActive, ImGuiWindowFlags_NoDocking))
    {
        _isFocused = ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows);
        ImGui::End();
        return;
    }

    _isFocused = ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows);

    Handle_PlayerbackShortcut();
    Handle_EditShortcut();

    Draw_ToolBar();
    ImGui::Separator();

    Draw_PreviewPanel();
    ImGui::Separator();

    if (ImGui::BeginTable("UIAnimLayout", 3, ImGuiTableFlags_Resizable | ImGuiTableFlags_BordersInnerV))
    {
        ImGui::TableSetupColumn("Left", ImGuiTableColumnFlags_WidthFixed, 260.f);
        ImGui::TableSetupColumn("Timeline", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn("Right", ImGuiTableColumnFlags_WidthFixed, 320.f);

        ImGui::TableNextColumn();
        Draw_LeftPanel();

        ImGui::TableNextColumn();
        Draw_Sequencer();

        ImGui::TableNextColumn();
        Draw_RightPanel();

        ImGui::EndTable();
    }

    Draw_FilePopup();
    ImGui::End();
}


FUIAnimObjectState UI_Animation_View::Capture_UIState(const Shared<UIObject>& ui)
{
    FUIAnimObjectState state;
    if (!ui)
        return state;

    state.valid = true;
    state.posX = ui->Get_UIPosX();
    state.posY = ui->Get_UIPosY();
    state.sizeX = ui->Get_UISizeX();
    state.sizeY = ui->Get_UISizeY();
    state.rotationZ = ui->Get_UIRotationZ();
    state.opacity = ui->Get_UIOpacity();
    state.tint = ui->Get_UITint();

    return state;
}

void UI_Animation_View::Apply_UIState(const Shared<UIObject>& ui, const FUIAnimObjectState& state)
{
    if (!ui || !state.valid)
        return;

    ui->Set_UIPosition(state.posX, state.posY);
    ui->Set_UIScale(state.sizeX, state.sizeY);
    ui->Set_UIRotationZ(state.rotationZ);
    ui->Set_UIOpacity(state.opacity);
    ui->Set_UITint(state.tint);
}

void UI_Animation_View::Normalize_TrackKeys(FUIAnimTrack& track)
{
    UI_AnimUtility::Sort_Keys(track);

    vector<FUIAnimKey> uniqueKeys;
    uniqueKeys.reserve(track.keys.size());

    for (const auto& key : track.keys)
    {
        if (!uniqueKeys.empty() && uniqueKeys.back().frame == key.frame)
        {
            // 같은 frame이 중복되면 마지막 편집값으로 덮어쓴다
            uniqueKeys.back() = key;
        }
        else
        {
            uniqueKeys.push_back(key);
        }
    }

    track.keys.swap(uniqueKeys);
}

bool UI_Animation_View::CanSave() const
{
    return _asset != nullptr;
}

void UI_Animation_View::Save()
{
    if (_currentFilePath.empty())
    {
        _isSaveFilePopup = true;
        return;
    }

    Save_Animation(_currentFilePath);
}

void UI_Animation_View::Pre_Render()
{
    EditorWindow::Pre_Render();

    if (!_previewEnabled)
        return;

    auto target = _boundTarget.lock();
    if (!target)
        return;

    const uint32 rtWidth = static_cast<uint32>(max(1.f, GAME->Get_ViewportWidth()));
    const uint32 rtHeight = static_cast<uint32>(max(1.f, GAME->Get_ViewportHeight()));

    if (!_previewRT)
        _previewRT = RenderTarget::Create(GAME->Get_Device(), rtWidth, rtHeight);
    else
        _previewRT->Resize(rtWidth, rtHeight);

    FUIAnimObjectState currentState = Capture_UIState(target);
    if (!currentState.valid)
        return;

    if (!_previewOriginState.valid)
        Capture_PreviewOrigin();

    FUIAnimObjectState previewState = currentState;

    if (_previewCenterMode && _previewOriginState.valid)
    {
        // Position 애니메이션을 화면 중앙 기준으로 보이게 하기 위한 오프셋 계산
        const float centerX = GAME->Get_WindowWidth() * 0.5f;
        const float centerY = GAME->Get_WindowHeight() * 0.5f;

        const float deltaX = currentState.posX - _previewOriginState.posX;
        const float deltaY = currentState.posY - _previewOriginState.posY;

        previewState.posX = centerX + deltaX;
        previewState.posY = centerY + deltaY;
    }

    previewState.sizeX *= _previewScaleMultiplier;
    previewState.sizeY *= _previewScaleMultiplier;

    Apply_UIState(target, previewState);

    _previewRT->Clear(Color(0.12f, 0.12f, 0.12f, 1.f));
    _previewRT->BindAsTarget();
    GAME->Set_TextTarget_Texture(_previewRT->Get_Texture2D());

    target->Render();

    GAME->BindBackBuffer();
    GAME->Reset_TextTarget_BackBuffer();

    Apply_UIState(target, currentState);
}

void UI_Animation_View::Create_NewAnimation()
{
    _asset = make_shared<FUIAnimAsset>();
    _asset->name = "NewUIAnimation";
    _asset->fps = 60;
    _asset->startFrame = 0;
    _asset->endFrame = 60;
    _asset->loop = false;
    _asset->targetName = L"";

    _currentFilePath.clear();
    _isPlaying = false;

    _sequencerState.currentFrame = 0;
    _sequencerState.selectedEntry = -1;
    _sequencerState.firstFrame = 0;
    _sequencerState.expanded = true;

    _selectedKeyIndex = -1;
    _selectedTrackIndex = -1;

    Clear_PreviewOrigin();

    Sync_TargetNameBuffer();
    Refresh_PlayerBinding();
    ClearDirty();
}

void UI_Animation_View::Load_Animation(const string& path)
{
    auto loaded = UI_AnimSerializer::Load_FromFile(Utils::ToWString(path));

    if (!loaded)
        return;

    _asset = loaded;
    _currentFilePath = path;
    _isPlaying = false;

    _sequencerState.currentFrame = _asset->startFrame;
    _sequencerState.selectedEntry = -1;
    _sequencerState.firstFrame = 0;
    _sequencerState.expanded = true;

    _selectedTrackIndex = -1;
    _selectedKeyIndex = -1;

    Clear_PreviewOrigin();

    Sync_TargetNameBuffer();
    Resolve_BoundTarget_FromAsset();
    Refresh_PlayerBinding();
    ClearDirty();
}

void UI_Animation_View::Save_Animation(const string& path)
{
    if (!_asset)
        return;

    if (!UI_AnimSerializer::Save_ToFile(Utils::ToWString(path), *_asset))
        return;

    _currentFilePath = path;
    ClearDirty();
}

void UI_Animation_View::Draw_ToolBar()
{
    if (ImGui::Button("New"))
        Create_NewAnimation();

    ImGui::SameLine();
    if (ImGui::Button("Load"))
        _isOpenFilePopup = true;

    ImGui::SameLine();
    if (ImGui::Button("Save"))
        Save();

    ImGui::SameLine();
    if (ImGui::Button("Save As"))
        _isSaveFilePopup = true;

    ImGui::Separator();

    Draw_TargetBinding();

    ImGui::Separator();

    if (ImGui::Button("Play"))
    {
        if (_player)
            _player->Play();

        _isPlaying = (_player ? _player->Is_Playing() : false);
    }

    ImGui::SameLine();
    if (ImGui::Button("Pause"))
    {
        if (_player)
            _player->Pause();

        _isPlaying = false;
    }

    ImGui::SameLine();
    if (ImGui::Button("Stop"))
    {
        if (_player)
            _player->Stop();

        _isPlaying = false;

        if (_asset)
            _sequencerState.currentFrame = _asset->startFrame;
    }

    if (!_asset)
        return;

    int fps = _asset->fps;
    int start = _asset->startFrame;
    int end = _asset->endFrame;
    bool loop = _asset->loop;

    ImGui::AlignTextToFramePadding();
    ImGui::Text("FPS");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(64.f);
    if (ImGui::InputInt("##FPS", &fps, 0, 0))
    {
        _asset->fps = max(1, fps);
        MarkDirty();
        Refresh_PlayerBinding();
    }

    ImGui::SameLine(0.f, 12.f);
    ImGui::AlignTextToFramePadding();
    ImGui::Text("Start");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(64.f);
    if (ImGui::InputInt("##Start", &start, 0, 0))
    {
        _asset->startFrame = max(0, start);

        if (_asset->endFrame < _asset->startFrame)
            _asset->endFrame = _asset->startFrame;

        MarkDirty();
        Refresh_PlayerBinding();
    }

    ImGui::SameLine(0.f, 12.f);
    ImGui::AlignTextToFramePadding();
    ImGui::Text("End");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(64.f);
    if (ImGui::InputInt("##End", &end, 0, 0))
    {
        _asset->endFrame = max(_asset->startFrame, end);
        MarkDirty();
        Refresh_PlayerBinding();
    }

    ImGui::SameLine(0.f, 12.f);
    if (ImGui::Checkbox("Loop", &loop))
    {
        _asset->loop = loop;
        MarkDirty();
        Refresh_PlayerBinding();
    }
}


void UI_Animation_View::Draw_FilePopup()
{
    fs::path baseDir = Get_UIAnimFolderPath();
    fs::create_directories(baseDir);

    if (_isSaveFilePopup)
    {
        _isSaveFilePopup = false;

        if (_asset && _asset->name.empty() == false && _saveFileNameBuf[0] == '\0')
        {
            strncpy_s(_saveFileNameBuf, _asset->name.c_str(), _TRUNCATE);
        }

        ImGui::OpenPopup("Save UI Animation");
    }

    if (_isOpenFilePopup)
    {
        _isOpenFilePopup = false;
        ImGui::OpenPopup("Load UI Animation");
    }

    if (ImGui::BeginPopupModal("Save UI Animation", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::Text("Save UI Animation");
        ImGui::Separator();

        ImGui::SetNextItemWidth(320.f);
        ImGui::InputText("File Name", _saveFileNameBuf, IM_ARRAYSIZE(_saveFileNameBuf));

        if (ImGui::Button("Save", ImVec2(120.f, 0.f)))
        {
            string fileName = _saveFileNameBuf;
            if (!fileName.empty())
            {
                if (!fileName.ends_with(".uianim.json"))
                    fileName += ".uianim.json";

                fs::path savePath = baseDir / fileName;
                Save_Animation(savePath.string());

                // 필요하면 asset scan 갱신
                GAME->Scan_Assets(TEXT("../../Client/Bin/Resources"));

                ImGui::CloseCurrentPopup();
                memset(_saveFileNameBuf, 0, sizeof(_saveFileNameBuf));
            }
        }

        ImGui::SameLine();

        if (ImGui::Button("Cancel", ImVec2(120.f, 0.f)))
        {
            ImGui::CloseCurrentPopup();
            memset(_saveFileNameBuf, 0, sizeof(_saveFileNameBuf));
        }

        ImGui::EndPopup();
    }

    if (ImGui::BeginPopupModal("Load UI Animation", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::Text("Load UI Animation");
        ImGui::Separator();

        vector<fs::path> files;

        if (fs::exists(baseDir))
        {
            for (const auto& entry : fs::directory_iterator(baseDir))
            {
                if (!entry.is_regular_file())
                    continue;

                string filename = entry.path().filename().string();
                if (filename.ends_with(".uianim.json"))
                    files.push_back(entry.path());
            }
        }

        sort(files.begin(), files.end());

        ImGui::BeginChild("UIAnimFileList", ImVec2(420.f, 240.f), true);
        for (const auto& path : files)
        {
            string filename = path.filename().string();

            if (ImGui::Selectable(filename.c_str()))
            {
                Load_Animation(path.string());
                ImGui::CloseCurrentPopup();
            }
        }
        ImGui::EndChild();

        if (ImGui::Button("Close", ImVec2(120.f, 0.f)))
        {
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }
}

void UI_Animation_View::Draw_LeftPanel()
{
    if (!_asset)
        return;

    ImGui::Text("Asset: %s", _asset->name.c_str());
    ImGui::Text("Tracks: %d", (int)_asset->tracks.size());
    ImGui::Separator();

    for (int i = 0; i < (int)_asset->tracks.size(); ++i)
    {
        const auto& track = _asset->tracks[i];
        const bool selected = (_selectedTrackIndex == i);

        string label = string(magic_enum::enum_name(track.property));
        label += " (" + to_string(track.keys.size()) + ")";

        if (ImGui::Selectable(label.c_str(), selected))
        {
            _selectedTrackIndex = i;
            _sequencerState.selectedEntry = i;
            _selectedKeyIndex = -1;
        }
    }

    ImGui::Separator();

    static int addTrackType = 0;
    const char* items[] = {
        "PositionX", "PositionY", "RotationZ", "ScaleX", "ScaleY", "Alpha", "ColorRGBA"
    };

    ImGui::Combo("Add Track Type", &addTrackType, items, IM_ARRAYSIZE(items));

    if (ImGui::Button("Add Track"))
        Add_Track((EUIAnimProperty)addTrackType);

    if (ImGui::Button("Delete Track"))
    {
        if (_selectedTrackIndex >= 0)
            Remove_Track(_selectedTrackIndex);
    }

    ImGui::BeginDisabled(_selectedTrackIndex < 0);
    if (ImGui::Button("Add Key To Selected Track"))
        Add_Key_AtCurrentFrame();
    ImGui::EndDisabled();

    if (ImGui::Button("Delete Key"))
        Delete_SelectedKey();
}

void UI_Animation_View::Draw_RightPanel()
{
    Draw_KeyInspector();
}

void UI_Animation_View::Draw_Sequencer()
{
    if (!_asset)
        return;

    ImVec2 size = ImGui::GetContentRegionAvail();
    ImGui::BeginChild("UIAnimSequencerChild", size, false, ImGuiWindowFlags_HorizontalScrollbar);
    {
        ImSequencer::Sequencer(
            &_sequencerAdapter,
            &_sequencerState.currentFrame,
            &_sequencerState.expanded,
            &_sequencerState.selectedEntry,
            &_sequencerState.firstFrame,
            ImSequencer::SEQUENCER_CHANGE_FRAME | ImSequencer::SEQUENCER_ADD
        );

        Apply_CurrentFrame(_sequencerState.currentFrame);
    }
    ImGui::EndChild();
}


void UI_Animation_View::Draw_KeyInspector()
{
    if (!_asset)
        return;

    auto* track = Get_SelectedTrack();
    if (!track)
    {
        ImGui::TextDisabled("No track selected");
        return;
    }

    ImGui::Text("Track: %s", string(magic_enum::enum_name(track->property)).c_str());
    ImGui::Text("Keys: %d", (int)track->keys.size());

    Draw_KeyList(*track);
    ImGui::Separator();

    auto* key = Get_SelectedKey();
    if (!key)
    {
        ImGui::TextDisabled("No key selected");
        return;
    }

    int frame = key->frame;
    if (ImGui::InputInt("Frame", &frame))
    {
        const int newFrame = max(0, frame);
        key->frame = newFrame;

        Normalize_TrackKeys(*track);
        _selectedKeyIndex = Find_KeyIndex_ByFrame(*track, newFrame);

        MarkDirty();
        Apply_CurrentFrame(_sequencerState.currentFrame);
    }

    if (track->property == EUIAnimProperty::ColorRGBA)
    {
        float value[4] = { key->value.x, key->value.y, key->value.z, key->value.w };
        if (ImGui::ColorEdit4("Color", value))
        {
            key->value = Vec4(value[0], value[1], value[2], value[3]);
            MarkDirty();
            Apply_CurrentFrame(_sequencerState.currentFrame);
        }
    }
    else
    {
        float value = key->value.x;
        if (ImGui::InputFloat("Value", &value))
        {
            key->value.x = value;
            MarkDirty();
            Apply_CurrentFrame(_sequencerState.currentFrame);
        }
    }
}

void UI_Animation_View::Draw_TargetBinding()
{
    if (ImGui::Button("Bind Selected UI"))
    {
        auto selected = Find_SelectedUIObject();
        if (selected && _asset)
        {
            Bind_TargetUI(selected);
        }
    }

    ImGui::SameLine();

    if (ImGui::Button("Pick UI..."))
    {
        _isOpenUIBindingPopup = true;
        ImGui::OpenPopup("UIBindingPopup");
    }

    ImGui::SameLine();

    if (ImGui::Button("Clear Target"))
    {
        _boundTarget.reset();

        if (_asset)
        {
            _asset->targetName.clear();
            Sync_TargetNameBuffer();

            Clear_PreviewOrigin();
            Refresh_PlayerBinding();

            MarkDirty();
        }
    }

    ImGui::SetNextItemWidth(220.f);
    if (ImGui::InputText("Target Name", _targetNameBuf, IM_ARRAYSIZE(_targetNameBuf)))
    {
        if (_asset)
        {
            _asset->targetName = Utils::ToWString(string(_targetNameBuf));
            MarkDirty();
        }
    }

    auto bound = _boundTarget.lock();
    if (bound)
    {
        string boundName = Utils::ToString(bound->Get_Name());
        ImGui::Text("Bound UI: %s", boundName.c_str());
    }
    else
    {
        ImGui::TextDisabled("Bound UI: None");
    }

    Draw_UIBindingPopup();
}

void UI_Animation_View::Refresh_PlayerBinding()
{
    if (!_asset || !_player)
        return;

    _player->Set_Asset(_asset);
    _player->Unbind_Target();

    auto target = _boundTarget.lock();
    if (target)
        _player->Bind_Target(target);

    _player->Set_CurrentFrame(_sequencerState.currentFrame);
}

void UI_Animation_View::Apply_CurrentFrame(int frame)
{
    if (!_asset || !_player)
        return;

    frame = clamp(frame, _asset->startFrame, _asset->endFrame);
    _sequencerState.currentFrame = frame;

    _player->Set_CurrentFrame(frame);
}

void UI_Animation_View::Sync_TargetNameBuffer()
{
    memset(_targetNameBuf, 0, sizeof(_targetNameBuf));
    if (!_asset)
        return;

    string targetName = Utils::ToString(_asset->targetName);
    strncpy_s(_targetNameBuf, targetName.c_str(), _TRUNCATE);
}

void UI_Animation_View::Sync_SequencerSelection()
{
    if (_sequencerState.selectedEntry >= 0)
        _selectedTrackIndex = _sequencerState.selectedEntry;
}

void UI_Animation_View::Add_Track(EUIAnimProperty property)
{
    if (!_asset || Has_Track(property))
        return;

    FUIAnimTrack track;
    track.property = property;
    _asset->tracks.push_back(track);

    _selectedTrackIndex = (int)_asset->tracks.size() - 1;
    _sequencerState.selectedEntry = _selectedTrackIndex;
    _selectedKeyIndex = -1;

    MarkDirty();
}

void UI_Animation_View::Remove_Track(int trackIndex)
{
    if (!_asset || trackIndex < 0 || trackIndex >= (int)_asset->tracks.size())
        return;

    _asset->tracks.erase(_asset->tracks.begin() + trackIndex);

    _selectedTrackIndex = -1;
    _selectedKeyIndex = -1;
    _sequencerState.selectedEntry = -1;

    MarkDirty();
}

void UI_Animation_View::Add_Key_AtCurrentFrame()
{
    auto* track = Get_SelectedTrack();
    if (!track)
        return;

    const int existingIndex = Find_KeyIndex_ByFrame(*track, _sequencerState.currentFrame);
    if (existingIndex >= 0)
    {
        _selectedKeyIndex = existingIndex;
        return;
    }

    FUIAnimKey key;
    key.frame = _sequencerState.currentFrame;
    key.interpolation = EUIAnimInterplation::Linear;
    key.value = UI_AnimUtility::Make_DefaultValue(track->property);

    auto target = _boundTarget.lock();
    if (target)
    {
        switch (track->property)
        {
        case EUIAnimProperty::PositionX: key.value.x = target->Get_UIPosX(); break;
        case EUIAnimProperty::PositionY: key.value.x = target->Get_UIPosY(); break;
        case EUIAnimProperty::ScaleX:    key.value.x = target->Get_UISizeX(); break;
        case EUIAnimProperty::ScaleY:    key.value.x = target->Get_UISizeY(); break;
        case EUIAnimProperty::RotationZ: key.value.x = target->Get_UIRotationZ(); break;
        case EUIAnimProperty::Alpha:     key.value.x = target->Get_UIOpacity(); break;
        case EUIAnimProperty::ColorRGBA:
        {
            const Color& c = target->Get_UITint();
            key.value = Vec4(c.x, c.y, c.z, c.w);
            break;
        }
        default:
            break;
        }
    }

    track->keys.push_back(key);

    Normalize_TrackKeys(*track);
    _selectedKeyIndex = Find_KeyIndex_ByFrame(*track, key.frame);

    MarkDirty();
    Apply_CurrentFrame(_sequencerState.currentFrame);
}

void UI_Animation_View::Delete_SelectedKey()
{
    auto* track = Get_SelectedTrack();
    if (!track)
        return;

    if (_selectedKeyIndex < 0 || _selectedKeyIndex >= (int)track->keys.size())
        return;

    track->keys.erase(track->keys.begin() + _selectedKeyIndex);
    _selectedKeyIndex = -1;

    MarkDirty();
    Apply_CurrentFrame(_sequencerState.currentFrame);
}

FUIAnimTrack* UI_Animation_View::Get_SelectedTrack()
{
    if (!_asset)
        return nullptr;
    if (_selectedTrackIndex < 0 || _selectedTrackIndex >= (int)_asset->tracks.size())
        return nullptr;
    return &_asset->tracks[_selectedTrackIndex];
}

FUIAnimKey* UI_Animation_View::Get_SelectedKey()
{
    auto* track = Get_SelectedTrack();
    if (!track)
        return nullptr;

    if (_selectedKeyIndex < 0 || _selectedKeyIndex >= (int)track->keys.size())
        return nullptr;

    return &track->keys[_selectedKeyIndex];
}

Shared<UIObject> UI_Animation_View::Find_SelectedUIObject() const
{
    auto hierarchy = dynamic_pointer_cast<Hierarchy>(EDITOR->Get_Window(TEXT("Hierarchy")));
    if (!hierarchy)
        return nullptr;

    const auto& selected = hierarchy->Get_SelectedObject();
    if (selected.empty())
        return nullptr;

    return dynamic_pointer_cast<UIObject>(selected.back());
}

void UI_Animation_View::Handle_PlayerbackShortcut()
{
    if (!_isFocused || !_asset || !_player)
        return;

    ImGuiIO& io = ImGui::GetIO();

    if (io.WantTextInput)
        return;

    if (ImGui::IsPopupOpen(nullptr, ImGuiPopupFlags_AnyPopupId))
        return;

    if (!ImGui::IsKeyPressed(ImGuiKey_Space, false))
        return;

    if (_player->Is_Playing())
    {
        _player->Pause();
        _isPlaying = false;
        return;
    }

    const bool isAtEndFrame = (_sequencerState.currentFrame >= _asset->endFrame);

    if (isAtEndFrame)
    {
        _sequencerState.currentFrame = _asset->startFrame;
        _player->Set_CurrentFrame(_asset->startFrame);
    }

    _player->Play();
    _isPlaying = true;
}

void UI_Animation_View::Handle_EditShortcut()
{
    if (!_isFocused || !_asset)
        return;

    if (_selectedTrackIndex < 0)
        return;

    ImGuiIO& io = ImGui::GetIO();

    if (io.WantTextInput)
        return;

    if (ImGui::IsPopupOpen(nullptr, ImGuiPopupFlags_AnyPopupId))
        return;

    if (!ImGui::IsKeyPressed(ImGuiKey_K, false))
        return;

    Add_Key_AtCurrentFrame();
}

void UI_Animation_View::Draw_KeyList(FUIAnimTrack& track)
{
    ImGui::Text("Key List");

    if (ImGui::BeginListBox("##KeyList", ImVec2(-1, 120.f)))
    {
        for (int i = 0; i < static_cast<int>(track.keys.size()); ++i)
        {
            const auto& key = track.keys[i];
            const bool selected = (_selectedKeyIndex == i);

            string label = "Frame ";
            label += to_string(key.frame);

            if (track.property != EUIAnimProperty::ColorRGBA)
            {
                label += "  |  ";
                label += to_string(key.value.x);
            }

            if (ImGui::Selectable(label.c_str(), selected))
            {
                _selectedKeyIndex = i;
            }
        }

        ImGui::EndListBox();
    }
}

void UI_Animation_View::On_KeyFrameDragged(FUIAnimTrack& track, int keyIndex)
{
    if (keyIndex < 0 || keyIndex >= static_cast<int>(track.keys.size()))
        return;

    _selectedKeyIndex = keyIndex;

    MarkDirty();
    Apply_CurrentFrame(_sequencerState.currentFrame);
}

void UI_Animation_View::On_KeyFrameDragFinished(FUIAnimTrack& track, int frame)
{
    Normalize_TrackKeys(track);
    _selectedKeyIndex = Find_KeyIndex_ByFrame(track, frame);

    MarkDirty();
    Apply_CurrentFrame(_sequencerState.currentFrame);
}

void UI_Animation_View::Draw_UIBindingPopup()
{
    if (!ImGui::BeginPopup("UIBindingPopup"))
        return;

    ImGui::Text("Select UIObject");
    ImGui::Separator();

    for (int layerIndex = 0; layerIndex < ETOI(EUILayer::END); ++layerIndex)
    {
        EUILayer layer = static_cast<EUILayer>(layerIndex);

        string layerLabel = string(magic_enum::enum_name(layer));

        if (ImGui::TreeNode(layerLabel.c_str()))
        {
            const auto& uiObjects = GAME->Get_UILayers(layer);

            for (const auto& ui : uiObjects)
            {
                if (!ui)
                    continue;

                string label = Utils::ToString(ui->Get_Name());
                if (label.empty())
                    label = "<Unnamed UI>";

                if (ImGui::Selectable(label.c_str()))
                {
                    Bind_TargetUI(ui);
                    ImGui::CloseCurrentPopup();
                }
            }

            ImGui::TreePop();
        }
    }

    ImGui::EndPopup();
}

void UI_Animation_View::Draw_PreviewPanel()
{
    ImGui::Text("UI Preview");

    ImGui::SameLine();
    ImGui::Checkbox("Enable Preview", &_previewEnabled);

    ImGui::SameLine();
    ImGui::Checkbox("Center Motion", &_previewCenterMode);

    ImGui::SameLine();
    if (ImGui::Button("Reset Preview Origin"))
    {
        Capture_PreviewOrigin();
    }

    ImGui::SameLine();
    ImGui::SetNextItemWidth(140.f);
    ImGui::SliderFloat("Preview Scale", &_previewScaleMultiplier, 0.25f, 3.0f, "%.2f");

    const float previewHeight = 300.f;
    ImVec2 previewSize(ImGui::GetContentRegionAvail().x, previewHeight);

    ImGui::BeginChild(
        "UIAnimPreviewChild",
        previewSize,
        true,
        ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

    {
        ImVec2 avail = ImGui::GetContentRegionAvail();

        if (_previewRT)
        {
            const float texWidth = max(1.f, GAME->Get_ViewportWidth());
            const float texHeight = max(1.f, GAME->Get_ViewportHeight());
            const float texAspect = texWidth / texHeight;

            ImVec2 drawSize = avail;

            if (avail.x / texAspect <= avail.y)
            {
                drawSize.x = avail.x;
                drawSize.y = avail.x / texAspect;
            }
            else
            {
                drawSize.y = avail.y;
                drawSize.x = avail.y * texAspect;
            }

            ImVec2 cursor = ImGui::GetCursorPos();
            cursor.x += (avail.x - drawSize.x) * 0.5f;
            cursor.y += (avail.y - drawSize.y) * 0.5f;
            ImGui::SetCursorPos(cursor);

            ImGui::Image((ImTextureID)_previewRT->Get_SRV(), drawSize);
        }
        else
        {
            const char* text = "No preview target";
            ImVec2 textSize = ImGui::CalcTextSize(text);

            ImVec2 cursor = ImGui::GetCursorPos();
            cursor.x += (avail.x - textSize.x) * 0.5f;
            cursor.y += (avail.y - textSize.y) * 0.5f;
            ImGui::SetCursorPos(cursor);

            ImGui::TextDisabled("%s", text);
        }
    }

    ImGui::EndChild();
}


int UI_Animation_View::Find_KeyIndex_ByFrame(const FUIAnimTrack& track, int frame) const
{
    for (int i = 0; i < static_cast<int>(track.keys.size()); ++i)
    {
        if (track.keys[i].frame == frame)
            return i;
    }

    return -1;
}

void UI_Animation_View::Capture_PreviewOrigin()
{
    auto target = _boundTarget.lock();

    _previewOriginState = Capture_UIState(target);
}

void UI_Animation_View::Clear_PreviewOrigin()
{
    _previewOriginState = {};
}

void UI_Animation_View::Bind_TargetUI(Shared<UIObject> ui)
{
    if (!ui || !_asset)
        return;

    _boundTarget = ui;
    _asset->targetName = ui->Get_Name();

    Sync_TargetNameBuffer();

    Capture_PreviewOrigin();
    Refresh_PlayerBinding();

    MarkDirty();
}

void UI_Animation_View::Resolve_BoundTarget_FromAsset()
{
    _boundTarget.reset();

    if (!_asset)
        return;

    if (_asset->targetName.empty())
        return;

    auto target = GAME->Find_UI(_asset->targetName);
    if (!target)
        return;

    _boundTarget = target;
    Capture_PreviewOrigin();
}

bool UI_Animation_View::Has_Track(EUIAnimProperty property) const
{
    if (!_asset)
        return false;

    for (const auto& track : _asset->tracks)
    {
        if (track.property == property)
            return true;
    }

    return false;
}

Shared<UI_Animation_View> UI_Animation_View::Create()
{
    return make_shared<UI_Animation_View>();
}
