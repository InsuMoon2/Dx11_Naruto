#include "pch.h"
#include "Animation_View.h"
#include "Notification_Manager.h"
#include "AnimNotify_Serializer.h"
#include "AnimNotify_Factory.h"
#include "AnimNotify.h"
#include "AnimNotifyState.h"
#include "Model.h"
#include "RenderTarget.h"
#include "AnimationStateComponent.h"

#include "Editor_Camera_Free.h"
#include "GameObject.h"
#include "GameInstance.h"
#include "Transform.h"
#include "DebugDraw.h"

#include "ContainerObject.h"
#include "PartObject.h"
#include "Reflection_Inspector.h"

#include "AnimNotify_Inspector_Factory.h"

static bool Contains_CaseInsensitive(const string& text, const string& pattern)
{
    if (pattern.empty())
        return true;

    auto it = std::search(
        text.begin(),
        text.end(),
        pattern.begin(),
        pattern.end(),
        [](char lhs, char rhs)
        {
            return std::tolower(static_cast<unsigned char>(lhs)) ==
                   std::tolower(static_cast<unsigned char>(rhs));
        });

    return it != text.end();
}

static void Collect_AnimationName(const FAnimationClipSetting& clip, unordered_set<string>& outNames)
{
    if (!clip.animationName.empty())
        outNames.insert(clip.animationName);
}

static bool Has_NotifyPayload(const FAnimNotifyClipData& clip)
{
    return !clip.notifies.empty() || !clip.notifyStates.empty();
}

static void Remove_NotifyTrackEntries(FAnimNotifyClipData& clip, int32 trackIndex)
{
    clip.notifies.erase(
        remove_if(
            clip.notifies.begin(),
            clip.notifies.end(),
            [trackIndex](const FAnimNotifyEventEntry& entry)
            {
                return entry.trackIndex == trackIndex;
            }),
        clip.notifies.end());

    for (auto& entry : clip.notifies)
    {
        if (entry.trackIndex > trackIndex)
            --entry.trackIndex;
    }
}

static void Remove_NotifyStateTrackEntries(FAnimNotifyClipData& clip, int32 trackIndex)
{
    clip.notifyStates.erase(
        remove_if(
            clip.notifyStates.begin(),
            clip.notifyStates.end(),
            [trackIndex](const FAnimNotifyStateEntry& entry)
            {
                return entry.trackIndex == trackIndex;
            }),
        clip.notifyStates.end());

    for (auto& entry : clip.notifyStates)
    {
        if (entry.trackIndex > trackIndex)
            --entry.trackIndex;
    }
}

Animation_View::Animation_View()
    : EditorWindow(TEXT("Animation View"))
    , _sequencerAdapter(&_sequencerState, &_sequencerContext)
{
}

void Animation_View::Initialize()
{
    EditorWindow::Initialize();

    _sequencerContext.view = this;
    _sequencerContext.clip = nullptr;
    _sequencerContext.selectedNotifyIndex = &_selectedNotifyIndex;
    _sequencerContext.selectedStateIndex = &_selectedStateIndex;
    _sequencerContext.isSelectedStateTrack = false;
    _sequencerContext.selectedTrackIndex = 0;

    _sequencerState.currentFrame = 0;
    _sequencerState.selectedEntry = -1;
    _sequencerState.firstFrame = 0;
    _sequencerState.expanded = true;

    _isActive = false;
}
void Animation_View::Update(float timeDelta)
{
    EditorWindow::Update(timeDelta);

    if (_previewCamera && _isPreviewHovered)
        _previewCamera->Priority_Update(timeDelta);

    if (!_model || !_isPlaying)
        return;

    // 프리뷰 재생 중, 노티파이 실행 금지
    // _model->Play_Animation(timeDelta, false);
    _previewPlaybackTimeSec += timeDelta;

    const int32 fps = Get_CurrentClipFps();
    const int32 frameMin = Get_FrameMin();
    const int32 frameMax = Get_FrameMax();

    _sequencerState.currentFrame = std::clamp(
        static_cast<int32>(std::round(
            _previewPlaybackTimeSec * static_cast<float>(fps))),
                frameMin,
                frameMax);

    if (_sequencerState.currentFrame >= frameMax)
    {
        _model->Set_Animation(static_cast<uint32>(_selectedClipIndex), false);
        _sequencerState.currentFrame = frameMin;
        _previewPlaybackTimeSec = 0.f;
    }

    Apply_CurrentFrame_ToPreview();
}

void Animation_View::OnGui()
{
    string title = Utils::ToString(Get_Name());

    if (!ImGui::Begin(title.c_str(), &_isActive, ImGuiWindowFlags_NoDocking))
    {
        _isFocused = ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows);
        ImGui::End();
        return;
    }

    _isFocused = ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows);

    Handle_PlaybackShortcut();
    Handle_DeleteShortcut();

    Draw_ToolBar();
    ImGui::Separator();

    Draw_TopLayout();
    ImGui::Separator();
    Draw_BottomLayout();

    Handle_CreateNotifyPopup();
    Handle_CreateStatePopup();

    ImGui::End();
}

void Animation_View::Pre_Render()
{
    EditorWindow::Pre_Render();

    if (!_model || !_previewOwner)
        return;

    const uint32 rtWidth = static_cast<uint32>(max(1.f, GAME->Get_ViewportWidth()));
    const uint32 rtHeight = static_cast<uint32>(max(1.f, GAME->Get_ViewportHeight()));

    if (!_previewRT)
        _previewRT = RenderTarget::Create(GAME->Get_Device(), rtWidth, rtHeight);
    else
        _previewRT->Resize(rtWidth, rtHeight);

    Ensure_PreviewCamera();
    if (!_previewCamera || !_previewRT)
        return;

    Matrix savedView = *GAME->Get_Transform(ETransformState::View);
    Matrix savedProj = *GAME->Get_Transform(ETransformState::Proj);

    _previewView = _previewCamera->Get_ViewMatrix();

    const float aspect = static_cast<float>(_previewRT->GetWidth()) / max(1.f, static_cast<float>(_previewRT->GetHeight()));
    _previewProj = XMMatrixPerspectiveFovLH(XM_PIDIV4, aspect, 0.1f, 100.f);

    GAME->Set_Transform(ETransformState::View, _previewView);
    GAME->Set_Transform(ETransformState::Proj, _previewProj);

    // 라이팅 세팅
    FLightDesc previewLight{};
    previewLight.direction = Vec4(0.f, -1.f, 0.f, 0.f);
    previewLight.diffuse = Vec4(1.f, 1.f, 1.f, 1.f);
    previewLight.ambient = Vec4(1.f, 1.f, 1.f, 1.f);
    previewLight.specular = Vec4(0.f, 0.f, 0.f, 1.f);

    GAME->Clear_Lights();
    GAME->Add_Light(previewLight);

    float dt = ImGui::GetIO().DeltaTime;

    bool wasEnableInput = GAME->Is_GameInputEnabled();
    GAME->Set_GameInputEnabled(false);

    bool wasEnableNotifies = true;
    if (_model)
    {
        wasEnableNotifies = _model->Is_EnableNotifies();
        _model->Set_EnableNotifies(false);
    }

    _previewOwner->Priority_Update(0.f);
    _previewOwner->Update(0.f);
    _previewOwner->Late_Update(0.f);

    if (_model)
    {
        _model->Set_EnableNotifies(wasEnableNotifies);
    }

    GAME->Set_GameInputEnabled(wasEnableInput);

    _previewRT->Clear(Color(0.12f, 0.12f, 0.12f, 1.f));
    _previewRT->BindAsTarget();

    GAME->Draw();
    Draw_PreviewGrid();

    GAME->BindBackBuffer();
    GAME->Clear_Lights();

    GAME->Set_Transform(ETransformState::View, savedView);
    GAME->Set_Transform(ETransformState::Proj, savedProj);
}

bool Animation_View::CanSave() const
{
    return !_modelGuid.empty();
}

void Animation_View::Save()
{
    Save_NotifyAsset();
}

void Animation_View::Open_Model(Shared<Model> model)
{
    _model = model;
    _previewOwner.reset();
    _modelGuid.clear();
    _clipSearchText.clear();
    _animStateClipNames.clear();
    _showAllClips = false;

    _selectedClipIndex = -1;
    Clear_SelectedEntries();

    _isPlaying = false;
    _previewPlaybackTimeSec = 0.f;

    _sequencerState.currentFrame = 0;
    _sequencerState.selectedEntry = -1;
    _sequencerState.firstFrame = 0;
    _sequencerState.expanded = true;

    if (_model)
    {
        _previewOwner = _model->Get_Owner();

        json data = _model->To_Json();
        _modelGuid = data.value("model_guid", "");
    }

    Refresh_ClipFilter();

    Ensure_PreviewCamera();
    Fit_PreviewCamera_ToOwner();

    Load_NotifyAsset();

    if (_model && _model->Get_AnimationCount() > 0)
    {
        const uint32 count = _model->Get_AnimationCount();
        for (uint32 i = 0; i < count; ++i)
        {
            const string clipName = _model->Get_AnimationName(i);
            if (!Passes_AnimStateClipFilter(clipName))
                continue;

            _selectedClipIndex = static_cast<int32>(i);
            break;
        }
    }

    Refresh_CurrentClip();

    if (_previewOwner)
    {
        _previewOwner->Priority_Update(0.f);
        _previewOwner->Late_Update(0.f);
    }

    _isActive = true;
}

void Animation_View::Focus_Clip(const string& clipName)
{
    if (!_model)
        return;

    const uint32 count = _model->Get_AnimationCount();
    for (uint32 i = 0; i < count; ++i)
    {
        if (_model->Get_AnimationName(i) == clipName)
        {
            _selectedClipIndex = static_cast<int32>(i);
            Refresh_CurrentClip();
            return;
        }
    }
}

void Animation_View::Add_NotifyTrack(const string& trackName)
{
    auto* clip = Get_CurrentClip();
    if (!clip)
        return;

    FAnimNotifyTrackDesc trackDesc;
    trackDesc.name = trackName.empty()
        ? "Notify Track " + to_string(static_cast<int32>(clip->notifyTracks.size()))
        : trackName;
    clip->notifyTracks.push_back(trackDesc);

    Select_NotifyTrack(static_cast<int32>(clip->notifyTracks.size()) - 1);
    MarkDirty();
}

void Animation_View::Add_NotifyStateTrack(const string& trackName)
{
    auto* clip = Get_CurrentClip();
    if (!clip)
        return;

    FAnimNotifyTrackDesc trackDesc;
    trackDesc.name = trackName.empty()
        ? "Notify State Track " + to_string(static_cast<int32>(clip->notifyStateTracks.size()))
        : trackName;
    clip->notifyStateTracks.push_back(trackDesc);

    Select_NotifyStateTrack(static_cast<int32>(clip->notifyStateTracks.size()) - 1);
    MarkDirty();
}

void Animation_View::Add_Notify_AtFrame(int32 frame, const string& typeName)
{
    auto* clip = Get_CurrentClip();
    if (!clip)
        return;

    if (clip->notifyTracks.empty())
        Add_NotifyTrack("Notifies");

    auto instance = AnimNotify_Factory::Create_Notify(typeName);
    if (!instance)
        return;

    int32 trackIndex = _selectedNotifyTrackIndex;
    if (!_sequencerContext.isSelectedStateTrack)
        trackIndex = _sequencerContext.selectedTrackIndex;

    trackIndex = std::clamp(trackIndex, 0, static_cast<int32>(clip->notifyTracks.size()) - 1);

    FAnimNotifyEventEntry entry;
    entry.timeSec = static_cast<float>(frame) / static_cast<float>(Get_CurrentClipFps());
    entry.trackIndex = trackIndex;
    entry.notify = instance;

    clip->notifies.push_back(entry);

    sort(clip->notifies.begin(), clip->notifies.end(),
        [](const FAnimNotifyEventEntry& lhs, const FAnimNotifyEventEntry& rhs)
        {
            if (lhs.trackIndex == rhs.trackIndex)
                return lhs.timeSec < rhs.timeSec;

            return lhs.trackIndex < rhs.trackIndex;
        });

    for (int32 i = 0; i < static_cast<int32>(clip->notifies.size()); ++i)
    {
        if (clip->notifies[i].notify == instance)
        {
            _selectedNotifyIndex = i;
            _selectedStateIndex = -1;
            Select_NotifyTrack(clip->notifies[i].trackIndex);
            break;
        }
    }

    Select_NotifyTrack(trackIndex);
    MarkDirty();
}
void Animation_View::Add_State_ByFrameRange(int32 startFrame, int32 endFrame, const string& typeName)
{
    auto* clip = Get_CurrentClip();
    if (!clip)
        return;

    if (endFrame <= startFrame)
        return;

    if (clip->notifyStateTracks.empty())
        Add_NotifyStateTrack("Notify States");

    auto instance = AnimNotify_Factory::Create_NotifyState(typeName);
    if (!instance)
        return;

    const int32 fps = Get_CurrentClipFps();

    int32 trackIndex = _selectedNotifyStateTrackIndex;
    if (_sequencerContext.isSelectedStateTrack)
        trackIndex = _sequencerContext.selectedTrackIndex;

    trackIndex = std::clamp(trackIndex, 0, static_cast<int32>(clip->notifyStateTracks.size()) - 1);

    FAnimNotifyStateEntry entry;
    entry.startSec = static_cast<float>(startFrame) / static_cast<float>(fps);
    entry.durationSec = static_cast<float>(endFrame - startFrame) / static_cast<float>(fps);
    entry.trackIndex = trackIndex;
    entry.notifyState = instance;

    clip->notifyStates.push_back(entry);

    sort(clip->notifyStates.begin(), clip->notifyStates.end(),
        [](const FAnimNotifyStateEntry& lhs, const FAnimNotifyStateEntry& rhs)
        {
            if (lhs.trackIndex != rhs.trackIndex)
                return lhs.trackIndex < rhs.trackIndex;

            if (lhs.startSec == rhs.startSec)
                return lhs.durationSec < rhs.durationSec;

            return lhs.startSec < rhs.startSec;
        });

    for (int32 i = 0; i < static_cast<int32>(clip->notifyStates.size()); ++i)
    {
        if (clip->notifyStates[i].notifyState == instance)
        {
            _selectedStateIndex = i;
            _selectedNotifyIndex = -1;
            Select_NotifyStateTrack(clip->notifyStates[i].trackIndex);
            break;
        }
    }

    Select_NotifyStateTrack(trackIndex);
    MarkDirty();
}
void Animation_View::Draw_ToolBar()
{
    if (ImGui::Button("Save"))
    {
        Save();
        NOTIFY("노티파이 저장");
    }

    ImGui::SameLine();

    if (ImGui::Button("Play"))
    {
        Start_CurrentClipPlaybackFromFrame(_sequencerState.currentFrame);
    }

    ImGui::SameLine();

    if (ImGui::Button("Pause"))
    {
        _isPlaying = false;
        Apply_CurrentFrame_ToPreview();
    }

    ImGui::SameLine();

    if (ImGui::Button("Stop"))
    {
        _isPlaying = false;
        _previewPlaybackTimeSec = 0.f;
        _sequencerState.currentFrame = Get_FrameMin();

        if (_model && _selectedClipIndex >= 0)
        {
            _model->Set_Animation(static_cast<uint32>(_selectedClipIndex), false);
            Apply_CurrentFrame_ToPreview();
        }
    }

    if (_model && _selectedClipIndex >= 0)
    {
        ImGui::SameLine(0.f, 16.f);
        ImGui::TextDisabled("Clip: %s", _model->Get_AnimationName(static_cast<uint32>(_selectedClipIndex)).c_str());
        ImGui::SameLine(0.f, 16.f);
        ImGui::TextDisabled("FPS: %d", Get_CurrentClipFps());
    }

    ImGui::SameLine(0.f, 16.f);
    ImGui::Checkbox("Preview Grid", &_showPreviewGrid);
}

void Animation_View::Draw_TopLayout()
{
    const float topHeight = Get_TopPanelHeight();

    if (ImGui::BeginTable(
        "AnimationViewTopLayout",
        3,
        ImGuiTableFlags_Resizable | ImGuiTableFlags_BordersInnerV,
        ImVec2(0.f, topHeight)))
    {
        ImGui::TableSetupColumn("ClipBrowser", ImGuiTableColumnFlags_WidthFixed, 300.f);
        ImGui::TableSetupColumn("Preview", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn("SelectedDetail", ImGuiTableColumnFlags_WidthFixed, 360.f);

        ImGui::TableNextColumn();
        Draw_ClipBrowserPanel();

        ImGui::TableNextColumn();
        Draw_PreviewPanel();

        ImGui::TableNextColumn();
        Draw_SelectedDetailPanel();

        ImGui::EndTable();
    }
}

void Animation_View::Draw_BottomLayout()
{
    if (ImGui::BeginTable(
        "AnimationViewBottomLayout",
        3,
        ImGuiTableFlags_Resizable | ImGuiTableFlags_BordersInnerV))
    {
        ImGui::TableSetupColumn("EventLists", ImGuiTableColumnFlags_WidthFixed, 300.f);
        ImGui::TableSetupColumn("Sequencer", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn("CreatePanel", ImGuiTableColumnFlags_WidthFixed, 320.f);

        ImGui::TableNextColumn();
        Draw_EventListPanel();

        ImGui::TableNextColumn();
        Draw_Sequencer();

        ImGui::TableNextColumn();
        Draw_CreatePanel();

        ImGui::EndTable();
    }
}

void Animation_View::Draw_ClipBrowserPanel()
{
    ImGui::Text("Animation Clips");
    ImGui::Separator();

    if (ImGui::BeginChild("ClipBrowserPanel", ImVec2(0.f, Get_TopChildHeight()), true))
    {
        char searchBuffer[256] = {};
        strcpy_s(searchBuffer, _clipSearchText.c_str());

        if (ImGui::InputTextWithHint("##ClipSearch", "Search clips...", searchBuffer, static_cast<size_t>(std::size(searchBuffer))))
            _clipSearchText = searchBuffer;

        ImGui::Checkbox("Show All Clips", &_showAllClips);
        ImGui::Spacing();
        Draw_ClipList();
    }
    ImGui::EndChild();
}

void Animation_View::Draw_SelectedDetailPanel()
{
    ImGui::Text("Selected Detail");
    ImGui::Separator();

    if (ImGui::BeginChild("SelectedDetailPanel", ImVec2(0.f, Get_TopChildHeight()), true))
    {
        if (Has_SelectedNotify())
        {
            Draw_SelectedNotifyInspector();
        }
        else if (Has_SelectedState())
        {
            Draw_SelectedStateInspector();
        }
        else
        {
            ImGui::TextDisabled("No notify selected");
            ImGui::Spacing();
            ImGui::TextWrapped("노티파이 또는 노티파이 스테이트를 선택 ㄱㄱ");
        }
    }
    ImGui::EndChild();
}

void Animation_View::Draw_EventListPanel()
{
    const float availableHeight = ImGui::GetContentRegionAvail().y;
    const float topHeight = max(120.f, availableHeight * 0.5f - 6.f);

    if (ImGui::BeginChild("NotifyListContainer", ImVec2(0.f, topHeight), false))
        Draw_NotifyList();
    ImGui::EndChild();

    ImGui::Spacing();

    if (ImGui::BeginChild("StateListContainer", ImVec2(0.f, 0.f), false))
        Draw_StateList();
    ImGui::EndChild();
}

void Animation_View::Draw_CreatePanel()
{
    Draw_NotifyTrackSection();
    ImGui::Separator();
    Draw_NotifyStateTrackSection();
    ImGui::Separator();
    Draw_CreateNotifySection();
    ImGui::Separator();
    Draw_CreateStateSection();
}
void Animation_View::Draw_Sequencer()
{
    auto* clip = Get_CurrentClip();
    _sequencerContext.clip = clip;

    if (!clip)
    {
        ImGui::TextDisabled("No clip selected");
        return;
    }

    ImVec2 size = ImGui::GetContentRegionAvail();

    ImGui::BeginChild("AnimationSequencerChild", size, false);
    {
        const ImVec2 sequencerCanvasPos = ImGui::GetCursorScreenPos();

        //  빈 공간 클릭 해제 판정은 프레임마다 초기화
        _sequencerContext.clickedOnNotify = false;

        _sequencerState.firstFrame = 0;

        ImSequencer::Sequencer(
            &_sequencerAdapter,
            &_sequencerState.currentFrame,
            &_sequencerState.expanded,
            &_sequencerState.selectedEntry,
            &_sequencerState.firstFrame,
            ImSequencer::SEQUENCER_EDIT_STARTEND | ImSequencer::SEQUENCER_CHANGE_FRAME
        );

        // 타임라인 빈 공간을 좌클릭하면 선택 해제
        if (!_sequencerContext.clickedOnNotify &&
            ImGui::IsWindowHovered(ImGuiHoveredFlags_ChildWindows) &&
            ImGui::IsMouseClicked(ImGuiMouseButton_Left))
        {
            Clear_SelectedEntries();
        }

        // 시퀀서 우클릭은 클릭 지점의 프레임을 기억한 뒤 컨텍스트 메뉴로 처리한다.
        if (ImGui::IsWindowHovered() &&
            !ImGui::GetIO().KeyShift &&
            ImGui::IsMouseClicked(ImGuiMouseButton_Right))
        {
            const ImVec2 mousePos = ImGui::GetMousePos();
            const ImVec2 winPos = ImGui::GetWindowPos();
            const ImVec2 contentMin = ImGui::GetWindowContentRegionMin();
            const ImVec2 contentMax = ImGui::GetWindowContentRegionMax();

            const float trackMinX = winPos.x + contentMin.x;
            const float trackMaxX = winPos.x + contentMax.x;
            bool isStateTrack = false;
            int32 trackIndex = -1;
            const bool hasTrackContext =
                Try_GetSequencerTrackContext(sequencerCanvasPos, mousePos, isStateTrack, trackIndex);

            Begin_SequencerContextMenu(
                Pixel_ToFrame_InSequencer(mousePos.x, trackMinX, trackMaxX),
                hasTrackContext,
                isStateTrack,
                trackIndex);
        }

        if (!_sequencerContext.isMarkingState &&
            _sequencerContext.pendingMarkStartFrame >= 0 &&
            _sequencerContext.pendingMarkEndFrame >= 0 &&
            _sequencerContext.pendingMarkStartFrame != _sequencerContext.pendingMarkEndFrame)
        {
            Begin_CreateStatePopup(
                min(_sequencerContext.pendingMarkStartFrame, _sequencerContext.pendingMarkEndFrame),
                max(_sequencerContext.pendingMarkStartFrame, _sequencerContext.pendingMarkEndFrame));

            _sequencerContext.pendingMarkStartFrame = -1;
            _sequencerContext.pendingMarkEndFrame = -1;
        }
    }
    ImGui::EndChild();

    Handle_SequencerContextMenu();

    if (!_isPlaying)
        Apply_CurrentFrame_ToPreview();
}
void Animation_View::Draw_PreviewPanel()
{
    ImGui::Text("Preview");

    ImVec2 previewSize(ImGui::GetContentRegionAvail().x, Get_TopChildHeight());

    ImGui::BeginChild(
        "AnimationPreviewChild",
        previewSize,
        true,
        ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

    _isPreviewHovered = ImGui::IsWindowHovered(ImGuiHoveredFlags_ChildWindows);

    {
        ImVec2 avail = ImGui::GetContentRegionAvail();

        if (_previewRT)
        {
            const float texWidth = max(1.f, GAME->Get_ViewportWidth());
            const float texHeight = max(1.f, GAME->Get_ViewportHeight());
            const float texAspect = texWidth / max(1.f, texHeight);

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
            ImGui::TextDisabled("No preview render target");
        }

        if (_model && _selectedClipIndex >= 0)
        {
            ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 8.f);
            ImGui::TextDisabled("Selected Clip: %s", _model->Get_AnimationName(static_cast<uint32>(_selectedClipIndex)).c_str());
            ImGui::TextDisabled("Current Frame: %d", _sequencerState.currentFrame);
        }
    }

    ImGui::EndChild();
}

void Animation_View::Draw_ClipList()
{
    if (!_model)
    {
        ImGui::TextDisabled("No model opened");
        return;
    }

    if (ImGui::BeginChild("AnimationClipList", ImVec2(0.f, 0.f), true))
    {
        const uint32 count = _model->Get_AnimationCount();
        bool foundAny = false;

        for (uint32 i = 0; i < count; ++i)
        {
            const string label = _model->Get_AnimationName(i);
            if (!Passes_AnimStateClipFilter(label))
                continue;

            if (!Passes_ClipSearch(label))
                continue;

            foundAny = true;

            const bool selected = (_selectedClipIndex == static_cast<int32>(i));

            if (ImGui::Selectable(label.c_str(), selected))
            {
                _selectedClipIndex = static_cast<int32>(i);
                Clear_SelectedEntries();
                Refresh_CurrentClip();
            }
        }

        if (!foundAny)
            ImGui::TextDisabled("No clips found");
    }
    ImGui::EndChild();
}

void Animation_View::Draw_NotifyList()
{
    auto* clip = Get_CurrentClip();

    const int32 notifyCount = clip ? static_cast<int32>(clip->notifies.size()) : 0;
    ImGui::Text("Notifies (%d)", notifyCount);
    ImGui::Separator();

    if (ImGui::BeginChild("NotifyList", ImVec2(0.f, 0.f), true))
    {
        if (!clip)
        {
            ImGui::TextDisabled("No clip selected");
        }
        else
        {
            const int32 fps = Get_CurrentClipFps();

            for (int32 trackIndex = 0; trackIndex < static_cast<int32>(clip->notifyTracks.size()); ++trackIndex)
            {
                const string trackLabel = clip->notifyTracks[trackIndex].name.empty()
                    ? "Notify Track " + to_string(trackIndex)
                    : clip->notifyTracks[trackIndex].name;

                if (ImGui::TreeNode((trackLabel + "##NotifyTrack" + to_string(trackIndex)).c_str()))
                {
                    if (ImGui::Selectable(("Select Track##NotifySelectTrack" + to_string(trackIndex)).c_str(), _selectedNotifyTrackIndex == trackIndex))
                    {
                        Clear_SelectedEntries();
                        Select_NotifyTrack(trackIndex);
                    }

                    for (int32 i = 0; i < static_cast<int32>(clip->notifies.size()); ++i)
                    {
                        const auto& entry = clip->notifies[i];
                        if (!entry.notify || entry.trackIndex != trackIndex)
                            continue;

                        const int32 frame = static_cast<int32>(std::round(entry.timeSec * fps));
                        const string label = entry.notify->Get_TypeName() + " (" + to_string(frame) + ")";

                        if (ImGui::Selectable(label.c_str(), _selectedNotifyIndex == i))
                        {
                            _selectedNotifyIndex = i;
                            _selectedStateIndex = -1;
                            Select_NotifyTrack(trackIndex);
                        }
                    }

                    ImGui::TreePop();
                }
            }
        }
    }
    ImGui::EndChild();
}
void Animation_View::Draw_StateList()
{
    auto* clip = Get_CurrentClip();

    const int32 stateCount = clip ? static_cast<int32>(clip->notifyStates.size()) : 0;
    ImGui::Text("Notify States (%d)", stateCount);
    ImGui::Separator();

    if (ImGui::BeginChild("StateList", ImVec2(0.f, 0.f), true))
    {
        if (!clip)
        {
            ImGui::TextDisabled("No clip selected");
        }
        else
        {
            const int32 fps = Get_CurrentClipFps();

            for (int32 trackIndex = 0; trackIndex < static_cast<int32>(clip->notifyStateTracks.size()); ++trackIndex)
            {
                const string trackLabel = clip->notifyStateTracks[trackIndex].name.empty()
                    ? "Notify State Track " + to_string(trackIndex)
                    : clip->notifyStateTracks[trackIndex].name;

                if (ImGui::TreeNode((trackLabel + "##NotifyStateTrack" + to_string(trackIndex)).c_str()))
                {
                    if (ImGui::Selectable(("Select Track##NotifyStateSelectTrack" + to_string(trackIndex)).c_str(), _selectedNotifyStateTrackIndex == trackIndex))
                    {
                        Clear_SelectedEntries();
                        Select_NotifyStateTrack(trackIndex);
                    }

                    for (int32 i = 0; i < static_cast<int32>(clip->notifyStates.size()); ++i)
                    {
                        const auto& entry = clip->notifyStates[i];
                        if (!entry.notifyState || entry.trackIndex != trackIndex)
                            continue;

                        const int32 startFrame = static_cast<int32>(std::round(entry.startSec * fps));
                        const int32 endFrame = static_cast<int32>(std::round((entry.startSec + entry.durationSec) * fps));
                        const string label = entry.notifyState->Get_TypeName() + " (" + to_string(startFrame) + "~" + to_string(endFrame) + ")";

                        if (ImGui::Selectable(label.c_str(), _selectedStateIndex == i))
                        {
                            _selectedStateIndex = i;
                            _selectedNotifyIndex = -1;
                            Select_NotifyStateTrack(trackIndex);
                        }
                    }

                    ImGui::TreePop();
                }
            }
        }
    }
    ImGui::EndChild();
}
void Animation_View::Draw_SelectedNotifyInspector()
{
    ImGui::Text("Selected Notify");
    ImGui::Separator();

    auto* clip = Get_CurrentClip();
    if (!clip || _selectedNotifyIndex < 0 || _selectedNotifyIndex >= static_cast<int32>(clip->notifies.size()))
    {
        ImGui::TextDisabled("No notify selected");
        return;
    }

    auto& entry = clip->notifies[_selectedNotifyIndex];
    if (!entry.notify)
    {
        ImGui::TextDisabled("Invalid notify");
        return;
    }

    auto customInspector =
        EDITOR->Get_NotifyInspector(entry.notify->Get_TypeName());

    if (customInspector)
    {
        customInspector->Draw_Inspector(entry.notify);
        return;
    }

    auto& reflectionInfo = entry.notify->Get_ReflectionInfo();
    if (reflectionInfo.properties.empty())
    {
        ImGui::TextDisabled("No editable properties");
        return;
    }

    Reflection_Inspector::Draw_Properties_Only(entry.notify.get(), reflectionInfo);
}


void Animation_View::Draw_SelectedStateInspector()
{
    ImGui::Text("Selected Notify State");
    ImGui::Separator();

    auto* clip = Get_CurrentClip();
    if (!clip || _selectedStateIndex < 0 || _selectedStateIndex >= static_cast<int32>(clip->notifyStates.size()))
    {
        ImGui::TextDisabled("No notify state selected");
        return;
    }

    auto& entry = clip->notifyStates[_selectedStateIndex];
    if (!entry.notifyState)
    {
        ImGui::TextDisabled("Invalid notify state");
        return;
    }

    auto& reflectionInfo = entry.notifyState->Get_ReflectionInfo();
    if (reflectionInfo.properties.empty())
    {
        ImGui::TextDisabled("리플렉션된 프로퍼티 XX");
        return;
    }

    Reflection_Inspector::Draw_Properties_Only(entry.notifyState.get(), reflectionInfo);
}

void Animation_View::Draw_NotifyTrackSection()
{
    ImGui::Text("Notify Tracks");
    ImGui::Separator();

    char buffer[128] = {};
    strcpy_s(buffer, _newNotifyTrackName.c_str());
    if (ImGui::InputText("Track Name##NotifyTrack", buffer, static_cast<size_t>(std::size(buffer))))
        _newNotifyTrackName = buffer;

    if (ImGui::Button("Add Notify Track"))
        Add_NotifyTrack(_newNotifyTrackName);

    auto* clip = Get_CurrentClip();
    const bool hasSelectedTrack =
        clip &&
        _selectedNotifyTrackIndex >= 0 &&
        _selectedNotifyTrackIndex < static_cast<int32>(clip->notifyTracks.size());

    ImGui::BeginDisabled(!hasSelectedTrack);

    if (hasSelectedTrack)
    {
        auto& trackName = clip->notifyTracks[_selectedNotifyTrackIndex].name;
        char editBuffer[128] = {};
        strcpy_s(editBuffer, trackName.c_str());

        if (ImGui::InputText("Selected Track##NotifyTrackEdit", editBuffer, static_cast<size_t>(std::size(editBuffer))))
        {
            trackName = editBuffer;
            MarkDirty();
        }
    }
    else
    {
        char disabledBuffer[128] = {};
        ImGui::InputText("Selected Track##NotifyTrackEdit", disabledBuffer, static_cast<size_t>(std::size(disabledBuffer)), ImGuiInputTextFlags_ReadOnly);
    }

    if (ImGui::Button("Delete Notify Track"))
        Delete_SelectedNotifyTrack();

    ImGui::EndDisabled();
}

void Animation_View::Draw_NotifyStateTrackSection()
{
    ImGui::Text("Notify State Tracks");
    ImGui::Separator();

    char buffer[128] = {};
    strcpy_s(buffer, _newNotifyStateTrackName.c_str());
    if (ImGui::InputText("Track Name##NotifyStateTrack", buffer, static_cast<size_t>(std::size(buffer))))
        _newNotifyStateTrackName = buffer;

    if (ImGui::Button("Add Notify State Track"))
        Add_NotifyStateTrack(_newNotifyStateTrackName);

    auto* clip = Get_CurrentClip();
    const bool hasSelectedTrack =
        clip &&
        _selectedNotifyStateTrackIndex >= 0 &&
        _selectedNotifyStateTrackIndex < static_cast<int32>(clip->notifyStateTracks.size());

    ImGui::BeginDisabled(!hasSelectedTrack);

    if (hasSelectedTrack)
    {
        auto& trackName = clip->notifyStateTracks[_selectedNotifyStateTrackIndex].name;
        char editBuffer[128] = {};
        strcpy_s(editBuffer, trackName.c_str());

        if (ImGui::InputText("Selected Track##NotifyStateTrackEdit", editBuffer, static_cast<size_t>(std::size(editBuffer))))
        {
            trackName = editBuffer;
            MarkDirty();
        }
    }
    else
    {
        char disabledBuffer[128] = {};
        ImGui::InputText("Selected Track##NotifyStateTrackEdit", disabledBuffer, static_cast<size_t>(std::size(disabledBuffer)), ImGuiInputTextFlags_ReadOnly);
    }

    if (ImGui::Button("Delete Notify State Track"))
        Delete_SelectedNotifyStateTrack();

    ImGui::EndDisabled();
}

void Animation_View::Draw_CreateNotifySection()
{
    ImGui::Text("Create Notify");
    ImGui::Separator();

    vector<string> typeNames = AnimNotify_Factory::Get_NotifyTypeNames();
    if (typeNames.empty())
    {
        ImGui::TextDisabled("No registered notify types");
        return;
    }

    _createNotifyTypeIndex = std::clamp(_createNotifyTypeIndex, 0, static_cast<int32>(typeNames.size()) - 1);

    if (ImGui::BeginCombo("Notify Type", typeNames[_createNotifyTypeIndex].c_str()))
    {
        for (int32 i = 0; i < static_cast<int32>(typeNames.size()); ++i)
        {
            const bool selected = (_createNotifyTypeIndex == i);
            if (ImGui::Selectable(typeNames[i].c_str(), selected))
                _createNotifyTypeIndex = i;

            if (selected)
                ImGui::SetItemDefaultFocus();
        }

        ImGui::EndCombo();
    }

    int32 frame = _sequencerState.currentFrame;
    if (ImGui::InputInt("Frame##CreateNotify", &frame))
        _requestedCreateNotifyFrame = std::clamp(frame, Get_FrameMin(), Get_FrameMax());
    else
        _requestedCreateNotifyFrame = _sequencerState.currentFrame;

    if (ImGui::Button("Add Notify"))
        Add_Notify_AtFrame(_requestedCreateNotifyFrame, typeNames[_createNotifyTypeIndex]);
}

void Animation_View::Draw_CreateStateSection()
{
    ImGui::Text("Create Notify State");
    ImGui::Separator();

    vector<string> typeNames = AnimNotify_Factory::Get_NotifyStateTypeNames();
    if (typeNames.empty())
    {
        ImGui::TextDisabled("No registered notify state types");
        return;
    }

    _createStateTypeIndex = std::clamp(_createStateTypeIndex, 0, static_cast<int32>(typeNames.size()) - 1);

    if (ImGui::BeginCombo("State Type", typeNames[_createStateTypeIndex].c_str()))
    {
        for (int32 i = 0; i < static_cast<int32>(typeNames.size()); ++i)
        {
            const bool selected = (_createStateTypeIndex == i);
            if (ImGui::Selectable(typeNames[i].c_str(), selected))
                _createStateTypeIndex = i;

            if (selected)
                ImGui::SetItemDefaultFocus();
        }

        ImGui::EndCombo();
    }

    const int32 frameMin = Get_FrameMin();
    const int32 frameMax = Get_FrameMax();
    const int32 safeStateStartMax = max(frameMin, frameMax - 1);

    int32 startFrame = (_sequencerContext.pendingMarkStartFrame >= 0)
        ? _sequencerContext.pendingMarkStartFrame
        : _sequencerState.currentFrame;

    int32 endFrame = (_sequencerContext.pendingMarkEndFrame >= 0)
        ? _sequencerContext.pendingMarkEndFrame
        : (_sequencerState.currentFrame + 10);

    startFrame = std::clamp(startFrame, frameMin, safeStateStartMax);
    endFrame = std::clamp(endFrame, startFrame + 1, frameMax);

    if (ImGui::InputInt("Start Frame##CreateState", &startFrame))
        _requestedCreateStateStartFrame = std::clamp(startFrame, frameMin, endFrame - 1);
    else
        _requestedCreateStateStartFrame = startFrame;

    if (ImGui::InputInt("End Frame##CreateState", &endFrame))
        _requestedCreateStateEndFrame = std::clamp(endFrame, _requestedCreateStateStartFrame + 1, frameMax);
    else
        _requestedCreateStateEndFrame = endFrame;

    if (ImGui::Button("Add Notify State"))
        Add_State_ByFrameRange(_requestedCreateStateStartFrame, _requestedCreateStateEndFrame, typeNames[_createStateTypeIndex]);
}

void Animation_View::Handle_SequencerContextMenu()
{
    if (_openSequencerContextPopup)
    {
        _openSequencerContextPopup = false;
        ImGui::OpenPopup("AnimationSequencerContextMenu");
    }

    if (!ImGui::BeginPopup("AnimationSequencerContextMenu"))
        return;

    ImGui::TextDisabled("Frame: %d", _sequencerContextFrame);
    if (_sequencerContextHasTrack)
    {
        ImGui::SameLine();
        ImGui::TextDisabled(
            "| Track: %s %d",
            _sequencerContextIsStateTrack ? "State" : "Notify",
            _sequencerContextTrackIndex);
    }
    ImGui::Separator();

    const bool canAddNotify = _sequencerContextHasTrack
        ? !_sequencerContextIsStateTrack
        : true;
    const bool canAddState = _sequencerContextHasTrack
        ? _sequencerContextIsStateTrack
        : true;

    if (ImGui::MenuItem("Notify Add", nullptr, false, canAddNotify))
    {
        Open_CreateNotifyFromContext();
        ImGui::CloseCurrentPopup();
    }

    if (ImGui::MenuItem("Notify State Add", nullptr, false, canAddState))
    {
        Open_CreateStateFromContext();
        ImGui::CloseCurrentPopup();
    }

    if (ImGui::BeginMenu("Track Add"))
    {
        if (ImGui::MenuItem("Notify"))
        {
            Add_Track_FromContext(false);
            ImGui::CloseCurrentPopup();
        }

        if (ImGui::MenuItem("Notify State"))
        {
            Add_Track_FromContext(true);
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndMenu();
    }

    ImGui::EndPopup();
}

void Animation_View::Handle_CreateNotifyPopup()
{
    if (_openCreateNotifyPopup)
    {
        _openCreateNotifyPopup = false;
        ImGui::OpenPopup("CreateNotifyPopup");
    }

    if (!ImGui::BeginPopup("CreateNotifyPopup"))
        return;

    vector<string> typeNames = AnimNotify_Factory::Get_NotifyTypeNames();
    if (typeNames.empty())
    {
        ImGui::TextDisabled("No registered notify types");
        ImGui::EndPopup();
        return;
    }

    _createNotifyTypeIndex = std::clamp(_createNotifyTypeIndex, 0, static_cast<int32>(typeNames.size()) - 1);

    if (ImGui::BeginCombo("Notify Type##Popup", typeNames[_createNotifyTypeIndex].c_str()))
    {
        for (int32 i = 0; i < static_cast<int32>(typeNames.size()); ++i)
        {
            if (ImGui::Selectable(typeNames[i].c_str(), _createNotifyTypeIndex == i))
                _createNotifyTypeIndex = i;
        }

        ImGui::EndCombo();
    }

    ImGui::Text("Frame: %d", _requestedCreateNotifyFrame);

    if (ImGui::Button("Create Notify"))
    {
        Add_Notify_AtFrame(_requestedCreateNotifyFrame, typeNames[_createNotifyTypeIndex]);
        ImGui::CloseCurrentPopup();
    }

    ImGui::SameLine();

    if (ImGui::Button("Cancel##CreateNotifyPopup"))
        ImGui::CloseCurrentPopup();

    ImGui::EndPopup();
}

void Animation_View::Handle_CreateStatePopup()
{
    if (_openCreateStatePopup)
    {
        _openCreateStatePopup = false;
        ImGui::OpenPopup("CreateStatePopup");
    }

    if (!ImGui::BeginPopup("CreateStatePopup"))
        return;

    vector<string> typeNames = AnimNotify_Factory::Get_NotifyStateTypeNames();
    if (typeNames.empty())
    {
        ImGui::TextDisabled("No registered notify state types");
        ImGui::EndPopup();
        return;
    }

    _createStateTypeIndex = std::clamp(_createStateTypeIndex, 0, static_cast<int32>(typeNames.size()) - 1);

    if (ImGui::BeginCombo("State Type##Popup", typeNames[_createStateTypeIndex].c_str()))
    {
        for (int32 i = 0; i < static_cast<int32>(typeNames.size()); ++i)
        {
            if (ImGui::Selectable(typeNames[i].c_str(), _createStateTypeIndex == i))
                _createStateTypeIndex = i;
        }

        ImGui::EndCombo();
    }

    ImGui::Text("Range: %d ~ %d", _requestedCreateStateStartFrame, _requestedCreateStateEndFrame);

    if (ImGui::Button("Create Notify State"))
    {
        Add_State_ByFrameRange(
            _requestedCreateStateStartFrame,
            _requestedCreateStateEndFrame,
            typeNames[_createStateTypeIndex]);

        ImGui::CloseCurrentPopup();
    }

    ImGui::SameLine();

    if (ImGui::Button("Cancel##CreateStatePopup"))
        ImGui::CloseCurrentPopup();

    ImGui::EndPopup();
}

void Animation_View::Begin_CreateNotifyPopup(int32 frame)
{
    _requestedCreateNotifyFrame = std::clamp(frame, Get_FrameMin(), Get_FrameMax());
    _openCreateNotifyPopup = true;
}

void Animation_View::Begin_SequencerContextMenu(int32 frame, bool hasTrackContext, bool isStateTrack, int32 trackIndex)
{
    // 시퀀서 우클릭 시 메뉴가 사용할 기준 프레임을 저장한다.
    _sequencerContextFrame = std::clamp(frame, Get_FrameMin(), Get_FrameMax());
    _sequencerContextHasTrack = hasTrackContext;
    _sequencerContextIsStateTrack = isStateTrack;
    _sequencerContextTrackIndex = hasTrackContext ? trackIndex : -1;
    _openSequencerContextPopup = true;
}

void Animation_View::Begin_CreateStatePopup(int32 startFrame, int32 endFrame)
{
    const int32 frameMin = Get_FrameMin();
    const int32 frameMax = Get_FrameMax();
    const int32 safeStateStartMax = max(frameMin, frameMax - 1);

    _requestedCreateStateStartFrame = std::clamp(startFrame, frameMin, safeStateStartMax);
    _requestedCreateStateEndFrame = std::clamp(endFrame, _requestedCreateStateStartFrame + 1, frameMax);
    _openCreateStatePopup = true;
}

void Animation_View::Open_CreateNotifyFromContext()
{
    // 우클릭한 프레임 기준으로 Notify 생성 팝업을 연다.
    if (_sequencerContextHasTrack && !_sequencerContextIsStateTrack)
        Select_NotifyTrack(_sequencerContextTrackIndex);

    Begin_CreateNotifyPopup(_sequencerContextFrame);
}

void Animation_View::Open_CreateStateFromContext()
{
    if (_sequencerContextHasTrack && _sequencerContextIsStateTrack)
        Select_NotifyStateTrack(_sequencerContextTrackIndex);

    // 범위 마킹이 남아 있으면 그 범위를 우선하고, 없으면 클릭 프레임 기준 최소 구간을 만든다.
    if (_sequencerContext.pendingMarkStartFrame >= 0 &&
        _sequencerContext.pendingMarkEndFrame >= 0 &&
        _sequencerContext.pendingMarkStartFrame != _sequencerContext.pendingMarkEndFrame)
    {
        Begin_CreateStatePopup(
            min(_sequencerContext.pendingMarkStartFrame, _sequencerContext.pendingMarkEndFrame),
            max(_sequencerContext.pendingMarkStartFrame, _sequencerContext.pendingMarkEndFrame));
        return;
    }

    const int32 frameMin = Get_FrameMin();
    const int32 frameMax = Get_FrameMax();
    const int32 safeStartFrame = std::clamp(_sequencerContextFrame, frameMin, max(frameMin, frameMax - 1));
    const int32 safeEndFrame = std::clamp(safeStartFrame + 1, safeStartFrame + 1, frameMax);

    Begin_CreateStatePopup(safeStartFrame, safeEndFrame);
}

void Animation_View::Add_Track_FromContext(bool isStateTrack)
{
    // 우클릭 메뉴에서 생성한 트랙은 현재 선택 문맥도 함께 갱신해 바로 이어서 작업할 수 있게 한다.
    if (isStateTrack)
    {
        Add_NotifyStateTrack(_newNotifyStateTrackName);
        Select_NotifyStateTrack(_selectedNotifyStateTrackIndex);
        return;
    }

    Add_NotifyTrack(_newNotifyTrackName);
    Select_NotifyTrack(_selectedNotifyTrackIndex);
}

bool Animation_View::Try_GetSequencerTrackContext(
    const ImVec2& sequencerCanvasPos,
    const ImVec2& mousePos,
    bool& outIsStateTrack,
    int32& outTrackIndex) const
{
    const auto* clip = Get_CurrentClip();
    if (!clip)
        return false;

    constexpr float SequencerHeaderHeight = 20.f;
    constexpr float SequencerTrackCustomHeight = 24.f;
    const float rowHeight = SequencerHeaderHeight + SequencerTrackCustomHeight;
    if (rowHeight <= 0.f)
        return false;

    const float localY = mousePos.y - (sequencerCanvasPos.y + SequencerHeaderHeight);
    if (localY < 0.f)
        return false;

    const int32 rowIndex = static_cast<int32>(localY / rowHeight);
    const int32 notifyTrackCount = static_cast<int32>(clip->notifyTracks.size());
    const int32 notifyStateTrackCount = static_cast<int32>(clip->notifyStateTracks.size());

    if (rowIndex < notifyTrackCount)
    {
        outIsStateTrack = false;
        outTrackIndex = rowIndex;
        return true;
    }

    const int32 stateRowIndex = rowIndex - notifyTrackCount;
    if (stateRowIndex < 0 || stateRowIndex >= notifyStateTrackCount)
        return false;

    outIsStateTrack = true;
    outTrackIndex = stateRowIndex;
    return true;
}

void Animation_View::Load_NotifyAsset()
{
    _asset = {};
    _asset.modelGuid = _modelGuid;

    if (_modelGuid.empty())
        return;

    const fs::path filePath = AnimNotify_Serializer::Get_ModelNotifyFilePath(_modelGuid);
    if (fs::exists(filePath))
        AnimNotify_Serializer::Load_FromFile(filePath.wstring(), _asset);

    ClearDirty();
}

void Animation_View::Save_NotifyAsset()
{
    if (_modelGuid.empty())
        return;

    FAnimNotifyAsset saveAsset{};
    saveAsset.modelGuid = _modelGuid;

    for (const auto& clip : _asset.clips)
    {
        if (!Has_NotifyPayload(clip))
            continue;

        saveAsset.clips.push_back(clip);
    }

    AnimNotify_Serializer::Save_ToFile(
        AnimNotify_Serializer::Get_ModelNotifyFilePath(_modelGuid).wstring(),
        saveAsset);

    _asset = saveAsset;

    GAME->Scan_Assets(TEXT("../../Client/Bin/Resources"));

    ClearDirty();
}

void Animation_View::Refresh_CurrentClip()
{
    Clear_SelectedEntries();
    Select_NotifyTrack(0);
    _sequencerState.currentFrame = Get_FrameMin();
    _previewPlaybackTimeSec = 0.f;
    _isPlaying = false;

    _sequencerContext.clip = Get_CurrentClip();

    if (_model && _selectedClipIndex >= 0)
    {
        _model->Set_Animation(static_cast<uint32>(_selectedClipIndex), false);
        Apply_CurrentFrame_ToPreview();
        Start_CurrentClipPlaybackFromFrame(Get_FrameMin());
    }
}
void Animation_View::Apply_CurrentFrame_ToPreview()
{
    if (!_model || _selectedClipIndex < 0 || !_previewOwner)
        return;

    const float timeSec = Get_CurrentFrameTimeSec();
    const float ticksPerSecond = _model->Get_AnimationTicksPerSecond(static_cast<uint32>(_selectedClipIndex));

    if (ticksPerSecond <= FLT_EPSILON)
        return;

    const float trackPosition = timeSec * ticksPerSecond;

    _model->Set_CurrentTrackPositionTicks(trackPosition);
    _model->Sample_CurrentPose();

    auto container = dynamic_pointer_cast<ContainerObject>(_previewOwner);
    if (container)
    {
        for (auto slot : magic_enum::enum_values<ContainerObject::EPartSlot>())
        {
            if (slot == ContainerObject::EPartSlot::END)
                continue;
            auto partObj = container->Get_PartObject(slot);
            if (!partObj)
                continue;

            auto partModel = partObj->Get_Component<Model>();
            if (partModel)
            {
                partModel->Set_CurrentTrackPositionTicks(trackPosition);
                partModel->Sample_CurrentPose();
            }
        }
    }
}

void Animation_View::Ensure_PreviewCamera()
{
    if (_previewCamera)
        return;

    _previewCamera = Editor_Camera_Free::Create(GAME->Get_Device(), GAME->Get_Context());

    Editor_Camera_Free::FEditorCameraDesc desc{};
    desc.eye = Vec3(0.f, 2.f, -6.f);
    desc.at = Vec3(0.f, 1.f, 0.f);
    desc.fovY = XM_PIDIV4;
    desc.nearZ = 0.1f;
    desc.farZ = 100.f;
    desc.speedPerSec = 10.f;
    desc.mouseSensor = 0.1f;

    _previewCamera->Initialize(&desc);
}

void Animation_View::Ensure_PreviewGridResources()
{
    if (_previewGridBatch && _previewGridEffect && _previewGridInputLayout && _previewGridDepthDisabledState)
        return;

    _previewGridBatch = make_shared<DirectX::PrimitiveBatch<DirectX::VertexPositionColor>>(GAME->Get_Context().Get());
    _previewGridEffect = make_shared<DirectX::BasicEffect>(GAME->Get_Device().Get());
    _previewGridEffect->SetVertexColorEnabled(true);

    const void* shaderByteCode = nullptr;
    size_t shaderByteCodeLength = 0;
    _previewGridEffect->GetVertexShaderBytecode(&shaderByteCode, &shaderByteCodeLength);

    if (FAILED(GAME->Get_Device()->CreateInputLayout(
        DirectX::VertexPositionColor::InputElements,
        DirectX::VertexPositionColor::InputElementCount,
        shaderByteCode,
        shaderByteCodeLength,
        &_previewGridInputLayout)))
    {
        _previewGridBatch.reset();
        _previewGridEffect.reset();
        _previewGridInputLayout.Reset();
    }

    D3D11_DEPTH_STENCIL_DESC depthDesc = {};
    depthDesc.DepthEnable = FALSE;
    depthDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
    depthDesc.DepthFunc = D3D11_COMPARISON_ALWAYS;
    depthDesc.StencilEnable = FALSE;

    if (FAILED(GAME->Get_Device()->CreateDepthStencilState(&depthDesc, _previewGridDepthDisabledState.GetAddressOf())))
    {
        _previewGridBatch.reset();
        _previewGridEffect.reset();
        _previewGridInputLayout.Reset();
        _previewGridDepthDisabledState.Reset();
    }
}

void Animation_View::Draw_PreviewGrid() const
{
    if (!_showPreviewGrid)
        return;

    if (!_previewGridBatch || !_previewGridEffect || !_previewGridInputLayout)
        return;

    auto context = GAME->Get_Context();
    if (!context)
        return;

    _previewGridEffect->SetWorld(Matrix::Identity);
    _previewGridEffect->SetView(_previewView);
    _previewGridEffect->SetProjection(_previewProj);

    context->IASetInputLayout(_previewGridInputLayout.Get());
    context->OMSetDepthStencilState(_previewGridDepthDisabledState.Get(), 0);
    context->OMSetBlendState(nullptr, nullptr, 0xffffffff);
    _previewGridEffect->Apply(context.Get());

    _previewGridBatch->Begin();

    Vec3 gridCenter = Vec3::Zero;
    if (_previewOwner)
    {
        auto transform = _previewOwner->Get_Transform();
        if (transform)
            gridCenter = transform->Get_WorldPosition();
    }

    const XMVECTOR origin = XMVectorSet(gridCenter.x, gridCenter.y, gridCenter.z, 1.f);
    const XMVECTOR xAxis = XMVectorSet(20.f, 0.f, 0.f, 0.f);
    const XMVECTOR zAxis = XMVectorSet(0.f, 0.f, 20.f, 0.f);

    DX::DrawGrid(
        _previewGridBatch.get(),
        xAxis,
        zAxis,
        origin,
        40,
        40,
        XMVectorSet(0.65f, 0.65f, 0.7f, 1.f));

    DX::DrawRay(
        _previewGridBatch.get(),
        origin,
        XMVectorSet(20.f, 0.f, 0.f, 0.f),
        false,
        XMVectorSet(1.f, 0.2f, 0.2f, 1.f));

    DX::DrawRay(
        _previewGridBatch.get(),
        origin,
        XMVectorSet(0.f, 0.f, 20.f, 0.f),
        false,
        XMVectorSet(0.2f, 0.7f, 1.f, 1.f));

    _previewGridBatch->End();
}

void Animation_View::Fit_PreviewCamera_ToOwner()
{
    if (!_previewCamera || !_previewOwner)
        return;

    Ensure_PreviewGridResources();

    auto transform = _previewOwner->Get_Transform();
    if (!transform)
        return;

    Vec3 forward = transform->Get_WorldForward();
    forward.y = 0.f;

    if (forward.LengthSquared() <= FLT_EPSILON)
        forward = Vec3(0.f, 0.f, 1.f);

    forward.Normalize();

    const Vec3 target = transform->Get_WorldPosition() + Vec3(0.f, 1.f, 0.f);
    const float distance = 5.f;
    const float height = 1.5f;

    const Vec3 eye = target + forward * distance + Vec3(0.f, height, 0.f);

    Editor_Camera_Free::FEditorCameraDesc desc{};
    desc.eye = eye;
    desc.at = target;
    desc.fovY = XM_PIDIV4;
    desc.nearZ = 0.1f;
    desc.farZ = 100.f;
    desc.speedPerSec = 10.f;
    desc.mouseSensor = 0.1f;

    _previewCamera->Apply_EditorDesc(desc);
}

void Animation_View::Handle_PlaybackShortcut()
{
    if (!_isFocused || !_model)
        return;

    ImGuiIO& io = ImGui::GetIO();

    if (io.WantTextInput)
        return;

    if (ImGui::IsPopupOpen(nullptr, ImGuiPopupFlags_AnyPopupId))
        return;

    if (!ImGui::IsKeyPressed(ImGuiKey_Space, false))
        return;

    if (_isPlaying)
    {
        _isPlaying = false;

        Apply_CurrentFrame_ToPreview();
        return;
    }

    const bool isAtEndFrame = (_sequencerState.currentFrame >= Get_FrameMax());
    if (isAtEndFrame)
    {
        Start_CurrentClipPlaybackFromFrame(Get_FrameMin());
        return;
    }

    Start_CurrentClipPlaybackFromFrame(_sequencerState.currentFrame);
}

void Animation_View::Handle_DeleteShortcut()
{
    if (!_isFocused || !_model)
        return;

    ImGuiIO& io = ImGui::GetIO();

    if (io.WantTextInput)
        return;

    if (ImGui::IsPopupOpen(nullptr, ImGuiPopupFlags_AnyPopupId))
        return;

    if (!ImGui::IsKeyPressed(ImGuiKey_Delete, false))
        return;

    if (Has_SelectedNotify())
    {
        Delete_SelectedNotify();
        return;
    }

    if (Has_SelectedState())
    {
        Delete_SelectedState();
        return;
    }

    if (_sequencerContext.isSelectedStateTrack)
    {
        if (Has_SelectedNotifyStateTrack())
            Delete_SelectedNotifyStateTrack();
        return;
    }

    if (Has_SelectedNotifyTrack())
        Delete_SelectedNotifyTrack();
}

bool Animation_View::Passes_ClipSearch(const string& clipName) const
{
    return Contains_CaseInsensitive(clipName, _clipSearchText);
}

void Animation_View::Refresh_ClipFilter()
{
    _animStateClipNames.clear();

    if (!_previewOwner)
        return;

    auto animState = _previewOwner->Get_Component<AnimationStateComponent>();
    if (!animState)
        return;

    const auto stateNames = animState->Get_StateNames();
    for (const auto& stateName : stateNames)
    {
        const auto* desc = animState->Find_State(stateName);
        if (!desc)
            continue;

        Collect_AnimationName(desc->single, _animStateClipNames);
        Collect_AnimationName(desc->start, _animStateClipNames);
        Collect_AnimationName(desc->loop, _animStateClipNames);
        Collect_AnimationName(desc->end, _animStateClipNames);
        Collect_AnimationName(desc->directional.forward, _animStateClipNames);
        Collect_AnimationName(desc->directional.backward, _animStateClipNames);
        Collect_AnimationName(desc->directional.left, _animStateClipNames);
        Collect_AnimationName(desc->directional.right, _animStateClipNames);
    }
}

bool Animation_View::Passes_AnimStateClipFilter(const string& clipName) const
{
    if (_showAllClips)
        return true;

    if (_animStateClipNames.empty())
        return true;

    return _animStateClipNames.contains(clipName);
}

bool Animation_View::Has_SelectedNotify() const
{
    const auto* clip = Get_CurrentClip();
    if (!clip)
        return false;

    return _selectedNotifyIndex >= 0 &&
           _selectedNotifyIndex < static_cast<int32>(clip->notifies.size());
}

bool Animation_View::Has_SelectedState() const
{
    const auto* clip = Get_CurrentClip();
    if (!clip)
        return false;

    return _selectedStateIndex >= 0 &&
           _selectedStateIndex < static_cast<int32>(clip->notifyStates.size());
}

bool Animation_View::Has_SelectedNotifyTrack() const
{
    const auto* clip = Get_CurrentClip();
    if (!clip)
        return false;

    return _selectedNotifyTrackIndex >= 0 &&
           _selectedNotifyTrackIndex < static_cast<int32>(clip->notifyTracks.size());
}

bool Animation_View::Has_SelectedNotifyStateTrack() const
{
    const auto* clip = Get_CurrentClip();
    if (!clip)
        return false;

    return _selectedNotifyStateTrackIndex >= 0 &&
           _selectedNotifyStateTrackIndex < static_cast<int32>(clip->notifyStateTracks.size());
}

void Animation_View::Clear_SelectedEntries()
{
    _selectedNotifyIndex = -1;
    _selectedStateIndex = -1;
    _sequencerState.selectedEntry = -1;
}

void Animation_View::Select_NotifyTrack(int32 trackIndex)
{
    auto* clip = Get_CurrentClip();
    if (!clip || clip->notifyTracks.empty())
    {
        _selectedNotifyTrackIndex = -1;
        _sequencerContext.isSelectedStateTrack = false;
        _sequencerContext.selectedTrackIndex = 0;
        return;
    }

    _selectedNotifyTrackIndex = std::clamp(trackIndex, 0, static_cast<int32>(clip->notifyTracks.size()) - 1);
    _sequencerContext.isSelectedStateTrack = false;
    _sequencerContext.selectedTrackIndex = _selectedNotifyTrackIndex;
}

void Animation_View::Select_NotifyStateTrack(int32 trackIndex)
{
    auto* clip = Get_CurrentClip();
    if (!clip || clip->notifyStateTracks.empty())
    {
        _selectedNotifyStateTrackIndex = -1;
        _sequencerContext.isSelectedStateTrack = true;
        _sequencerContext.selectedTrackIndex = 0;
        return;
    }

    _selectedNotifyStateTrackIndex = std::clamp(trackIndex, 0, static_cast<int32>(clip->notifyStateTracks.size()) - 1);
    _sequencerContext.isSelectedStateTrack = true;
    _sequencerContext.selectedTrackIndex = _selectedNotifyStateTrackIndex;
}

float Animation_View::Get_CurrentFrameTimeSec() const
{
    const int32 fps = Get_CurrentClipFps();
    if (fps <= 0)
        return 0.f;

    return static_cast<float>(_sequencerState.currentFrame) / static_cast<float>(fps);
}

float Animation_View::Get_CurrentClipLengthSec() const
{
    if (!_model || _selectedClipIndex < 0)
        return 0.f;

    return _model->Get_AnimationLengthSec(static_cast<uint32>(_selectedClipIndex));
}

int32 Animation_View::Pixel_ToFrame_InSequencer(float pixelX, float trackMinX, float trackMaxX) const
{
    const int32 frameMin = Get_FrameMin();
    const int32 frameMax = Get_FrameMax();

    const float width = max(1.f, trackMaxX - trackMinX);
    const float normalized = (pixelX - trackMinX) / width;
    const float clamped = std::clamp(normalized, 0.f, 1.f);

    return frameMin + static_cast<int32>(std::round(clamped * static_cast<float>(frameMax - frameMin)));
}

int32 Animation_View::Get_FrameMax() const
{
    return Get_CurrentClipFrameMax();
}

int32 Animation_View::Get_CurrentClipFrameMax() const
{
    if (!_model || _selectedClipIndex < 0)
        return 60;

    const int32 fps = Get_CurrentClipFps();
    if (fps <= 0)
        return 60;

    const float lengthSec = Get_CurrentClipLengthSec();
    if (lengthSec <= FLT_EPSILON)
        return 60;

    return max(1, static_cast<int32>(std::round(lengthSec * static_cast<float>(fps))));
}

int32 Animation_View::Get_CurrentClipFps() const
{
    const auto* clip = Get_CurrentClip();
    if (clip && clip->displayFps > 0)
        return clip->displayFps;

    if (_model && _selectedClipIndex >= 0)
    {
        const float ticksPerSecond = _model->Get_AnimationTicksPerSecond(static_cast<uint32>(_selectedClipIndex));
        if (ticksPerSecond > FLT_EPSILON)
            return max(1, static_cast<int32>(::round(ticksPerSecond)));
    }

    return 30;
}

FAnimNotifyClipData* Animation_View::Get_CurrentClip()
{
    if (!_model || _selectedClipIndex < 0)
        return nullptr;

    return &AnimNotify_Serializer::Get_OrAddClip(
        _asset,
        _model->Get_AnimationName(static_cast<uint32>(_selectedClipIndex)));
}

const FAnimNotifyClipData* Animation_View::Get_CurrentClip() const
{
    if (!_model || _selectedClipIndex < 0)
        return nullptr;

    return AnimNotify_Serializer::Find_Clip(
        _asset,
        _model->Get_AnimationName(static_cast<uint32>(_selectedClipIndex)));
}

float Animation_View::Get_TopPanelHeight() const
{
    const float textLine = ImGui::GetTextLineHeight();
    const float spacingY = ImGui::GetStyle().ItemSpacing.y;
    const float separatorY = 4.f;

    return textLine + spacingY + separatorY + spacingY + Get_TopChildHeight();
}

float Animation_View::Get_TopChildHeight() const
{
    return 430.f;
}

void Animation_View::Start_CurrentClipPlaybackFromFrame(int32 frame)
{
    if (!_model || _selectedClipIndex < 0)
        return;

    _sequencerState.currentFrame = std::clamp(frame, Get_FrameMin(), Get_FrameMax());
    _previewPlaybackTimeSec = Get_CurrentFrameTimeSec();

    _model->Set_Animation(static_cast<uint32>(_selectedClipIndex), false);
    Apply_CurrentFrame_ToPreview();

    _isPlaying = true;
}

void Animation_View::Delete_SelectedNotify()
{
    auto* clip = Get_CurrentClip();
    if (!clip)
        return;

    if (_selectedNotifyIndex < 0 || _selectedNotifyIndex >= static_cast<int32>(clip->notifies.size()))
        return;

    clip->notifies.erase(clip->notifies.begin() + _selectedNotifyIndex);
    _selectedNotifyIndex = -1;
    MarkDirty();
}

void Animation_View::Delete_SelectedState()
{
    auto* clip = Get_CurrentClip();
    if (!clip)
        return;

    if (_selectedStateIndex < 0 || _selectedStateIndex >= static_cast<int32>(clip->notifyStates.size()))
        return;

    clip->notifyStates.erase(clip->notifyStates.begin() + _selectedStateIndex);
    _selectedStateIndex = -1;
    MarkDirty();
}

void Animation_View::Delete_SelectedNotifyTrack()
{
    auto* clip = Get_CurrentClip();
    if (!clip)
        return;

    if (_selectedNotifyTrackIndex < 0 || _selectedNotifyTrackIndex >= static_cast<int32>(clip->notifyTracks.size()))
        return;

    const int32 removedTrackIndex = _selectedNotifyTrackIndex;
    Remove_NotifyTrackEntries(*clip, removedTrackIndex);
    clip->notifyTracks.erase(clip->notifyTracks.begin() + removedTrackIndex);

    Clear_SelectedEntries();

    if (clip->notifyTracks.empty())
        Select_NotifyTrack(0);
    else
        Select_NotifyTrack(min(removedTrackIndex, static_cast<int32>(clip->notifyTracks.size()) - 1));

    MarkDirty();
}

void Animation_View::Delete_SelectedNotifyStateTrack()
{
    auto* clip = Get_CurrentClip();
    if (!clip)
        return;

    if (_selectedNotifyStateTrackIndex < 0 || _selectedNotifyStateTrackIndex >= static_cast<int32>(clip->notifyStateTracks.size()))
        return;

    const int32 removedTrackIndex = _selectedNotifyStateTrackIndex;
    Remove_NotifyStateTrackEntries(*clip, removedTrackIndex);
    clip->notifyStateTracks.erase(clip->notifyStateTracks.begin() + removedTrackIndex);

    Clear_SelectedEntries();

    if (clip->notifyStateTracks.empty())
        Select_NotifyStateTrack(0);
    else
        Select_NotifyStateTrack(min(removedTrackIndex, static_cast<int32>(clip->notifyStateTracks.size()) - 1));

    MarkDirty();
}

Shared<Animation_View> Animation_View::Create()
{
    auto instance = make_shared<Animation_View>();

    instance->Initialize();

    return instance;
}

