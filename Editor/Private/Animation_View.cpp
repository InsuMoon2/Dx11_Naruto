#include "pch.h"
#include "Animation_View.h"

#include "AnimNotify_Serializer.h"
#include "AnimNotify_Factory.h"
#include "AnimNotify_Inspector_Factory.h"
#include "AnimNotify.h"
#include "AnimNotifyState.h"
#include "Model.h"
#include "RenderTarget.h"

#include "Editor_Camera_Free.h"
#include "GameObject.h"
#include "GameInstance.h"
#include "Transform.h"

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
    {
        _previewCamera->Priority_Update(timeDelta);
    }

    if (!_model || !_isPlaying)
        return;

    // 프리뷰 재생 중, 노티파이 실행 금지
    _model->Play_Animation(timeDelta, false);

    _previewPlaybackTimeSec += timeDelta;

    const int32 fps = Get_CurrentClipFps();
    const int32 frameMax = Get_FrameMax();

    _sequencerState.currentFrame = std::clamp(
        static_cast<int32>(std::round(_previewPlaybackTimeSec * static_cast<float>(fps))),
        Get_FrameMin(),
        frameMax);

    if (_sequencerState.currentFrame >= frameMax)
    {
        _sequencerState.currentFrame = frameMax;
        _isPlaying = false;

        Apply_CurrentFrame_ToPreview();
    }
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

    Draw_ToolBar();
    ImGui::Separator();

    Draw_PreviewPanel();
    ImGui::Separator();

    if (ImGui::BeginTable("AnimationViewLayout", 3, ImGuiTableFlags_Resizable | ImGuiTableFlags_BordersInnerV))
    {
        ImGui::TableSetupColumn("Left", ImGuiTableColumnFlags_WidthFixed, 260.f);
        ImGui::TableSetupColumn("Timeline", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn("Right", ImGuiTableColumnFlags_WidthFixed, 340.f);

        ImGui::TableNextColumn();
        Draw_LeftPanel();

        ImGui::TableNextColumn();
        Draw_Sequencer();

        ImGui::TableNextColumn();
        Draw_RightPanel();

        ImGui::EndTable();
    }

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

    // 라이팅 대충 세팅
    FLightDesc previewLight{};
    previewLight.direction = Vec4(0.f, -1.f, 0.f, 0.f);
    previewLight.diffuse = Vec4(1.f, 1.f, 1.f, 1.f);
    previewLight.ambient = Vec4(1.f, 1.f, 1.f, 1.f);
    previewLight.specular = Vec4(0.f, 0.f, 0.f, 1.f);

    GAME->Clear_Lights();
    GAME->Add_Light(previewLight);

    _previewRT->Clear(Color(0.12f, 0.12f, 0.12f, 1.f));
    _previewRT->BindAsTarget();

    _previewOwner->Render();

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

    _selectedClipIndex = -1;
    _selectedNotifyIndex = -1;
    _selectedStateIndex = -1;

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

    Ensure_PreviewCamera();
    Fit_PreviewCamera_ToOwner();

    Load_NotifyAsset();

    if (_model && _model->Get_AnimationCount() > 0)
        _selectedClipIndex = 0;

    Refresh_CurrentClip();
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

void Animation_View::Add_Notify_AtFrame(int32 frame, const string& typeName)
{
    auto* clip = Get_CurrentClip();
    if (!clip)
        return;

    auto instance = AnimNotify_Factory::Create_Notify(typeName);
    if (!instance)
        return;

    FAnimNotifyEventEntry entry;
    entry.timeSec = static_cast<float>(frame) / static_cast<float>(Get_CurrentClipFps());
    entry.notify = instance;

    clip->notifies.push_back(entry);

    sort(clip->notifies.begin(), clip->notifies.end(),
        [](const FAnimNotifyEventEntry& lhs, const FAnimNotifyEventEntry& rhs)
        {
            return lhs.timeSec < rhs.timeSec;
        });

    for (int32 i = 0; i < static_cast<int32>(clip->notifies.size()); ++i)
    {
        if (clip->notifies[i].notify == instance)
        {
            _selectedNotifyIndex = i;
            _selectedStateIndex = -1;
            break;
        }
    }

    MarkDirty();
}

void Animation_View::Add_State_ByFrameRange(int32 startFrame, int32 endFrame, const string& typeName)
{
    auto* clip = Get_CurrentClip();
    if (!clip)
        return;

    if (endFrame <= startFrame)
        return;

    auto instance = AnimNotify_Factory::Create_NotifyState(typeName);
    if (!instance)
        return;

    const int32 fps = Get_CurrentClipFps();

    FAnimNotifyStateEntry entry;
    entry.startSec = static_cast<float>(startFrame) / static_cast<float>(fps);
    entry.durationSec = static_cast<float>(endFrame - startFrame) / static_cast<float>(fps);
    entry.notifyState = instance;

    clip->notifyStates.push_back(entry);

    sort(clip->notifyStates.begin(), clip->notifyStates.end(),
        [](const FAnimNotifyStateEntry& lhs, const FAnimNotifyStateEntry& rhs)
        {
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
            break;
        }
    }

    MarkDirty();
}

void Animation_View::Draw_ToolBar()
{
    if (ImGui::Button("Save"))
        Save();

    ImGui::SameLine();

    if (ImGui::Button("Play"))
    {
        if (_model && _selectedClipIndex >= 0)
        {
            _model->Set_Animation(static_cast<uint32>(_selectedClipIndex), false);
            _previewPlaybackTimeSec = static_cast<float>(_sequencerState.currentFrame) / static_cast<float>(Get_CurrentClipFps());
            _isPlaying = true;
        }
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
}

void Animation_View::Draw_LeftPanel()
{
    Draw_ClipList();
    ImGui::Separator();
    Draw_NotifyList();
    ImGui::Separator();
    Draw_StateList();
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
            _selectedNotifyIndex = -1;
            _selectedStateIndex = -1;
        }

        // Notify 생성은 adapter가 아니라 Animation_View가 우클릭 팝업으로 처리
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

            Begin_CreateNotifyPopup(Pixel_ToFrame_InSequencer(mousePos.x, trackMinX, trackMaxX));
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

    if (!_isPlaying)
        Apply_CurrentFrame_ToPreview();
}


void Animation_View::Draw_RightPanel()
{
    Draw_CreateNotifySection();
    ImGui::Separator();

    Draw_CreateStateSection();
    ImGui::Separator();

    Draw_SelectedNotifyInspector();
    ImGui::Separator();

    Draw_SelectedStateInspector();
}

void Animation_View::Draw_PreviewPanel()
{
    ImGui::Text("Preview");

    const float previewHeight = 460.f;
    ImVec2 previewSize(ImGui::GetContentRegionAvail().x, previewHeight);

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
    ImGui::Text("Animation Clips");
    ImGui::Separator();

    if (!_model)
    {
        ImGui::TextDisabled("No model opened");
        return;
    }

    if (ImGui::BeginChild("AnimationClipList", ImVec2(0.f, 220.f), true))
    {
        const uint32 count = _model->Get_AnimationCount();
        for (uint32 i = 0; i < count; ++i)
        {
            const bool selected = (_selectedClipIndex == static_cast<int32>(i));
            const string label = _model->Get_AnimationName(i);

            if (ImGui::Selectable(label.c_str(), selected))
            {
                _selectedClipIndex = static_cast<int32>(i);
                Refresh_CurrentClip();
            }
        }
    }
    ImGui::EndChild();
}

void Animation_View::Draw_NotifyList()
{
    auto* clip = Get_CurrentClip();

    ImGui::Text("Notifies");
    ImGui::Separator();

    if (ImGui::BeginChild("NotifyList", ImVec2(0.f, 170.f), true))
    {
        if (!clip)
        {
            ImGui::TextDisabled("No clip selected");
        }
        else
        {
            const int32 fps = Get_CurrentClipFps();

            for (int32 i = 0; i < static_cast<int32>(clip->notifies.size()); ++i)
            {
                const auto& entry = clip->notifies[i];
                if (!entry.notify)
                    continue;

                const int32 frame = static_cast<int32>(std::round(entry.timeSec * fps));
                string label = entry.notify->Get_TypeName() + " (" + to_string(frame) + ")";

                if (ImGui::Selectable(label.c_str(), _selectedNotifyIndex == i))
                {
                    _selectedNotifyIndex = i;
                    _selectedStateIndex = -1;
                    _sequencerState.selectedEntry = 0;
                }
            }
        }
    }
    ImGui::EndChild();
}

void Animation_View::Draw_StateList()
{
    auto* clip = Get_CurrentClip();

    ImGui::Text("Notify States");
    ImGui::Separator();

    if (ImGui::BeginChild("StateList", ImVec2(0.f, 170.f), true))
    {
        if (!clip)
        {
            ImGui::TextDisabled("No clip selected");
        }
        else
        {
            const int32 fps = Get_CurrentClipFps();

            for (int32 i = 0; i < static_cast<int32>(clip->notifyStates.size()); ++i)
            {
                const auto& entry = clip->notifyStates[i];
                if (!entry.notifyState)
                    continue;

                const int32 startFrame = static_cast<int32>(std::round(entry.startSec * fps));
                const int32 endFrame = static_cast<int32>(std::round((entry.startSec + entry.durationSec) * fps));

                string label = entry.notifyState->Get_TypeName() + " (" + to_string(startFrame) + "~" + to_string(endFrame) + ")";

                if (ImGui::Selectable(label.c_str(), _selectedStateIndex == i))
                {
                    _selectedStateIndex = i;
                    _selectedNotifyIndex = -1;
                    _sequencerState.selectedEntry = 1;
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

    const int32 fps = Get_CurrentClipFps();

    ImGui::Text("Type: %s", entry.notify->Get_TypeName().c_str());

    int32 frame = static_cast<int32>(std::round(entry.timeSec * fps));
    if (ImGui::InputInt("Frame##SelectedNotify", &frame))
    {
        frame = std::clamp(frame, Get_FrameMin(), Get_FrameMax());
        entry.timeSec = static_cast<float>(frame) / static_cast<float>(fps);
        MarkDirty();
    }

    auto inspector = AnimNotify_Inspector_Factory::GetInstance()->Get_NotifyInspector(entry.notify->Get_TypeName());
    if (inspector)
        inspector->Draw_Inspector(entry.notify);
    else
        ImGui::TextDisabled("No custom payload inspector");

    if (ImGui::Button("Delete Notify"))
        Delete_SelectedNotify();
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

    const int32 fps = Get_CurrentClipFps();

    ImGui::Text("Type: %s", entry.notifyState->Get_TypeName().c_str());

    int32 startFrame = static_cast<int32>(std::round(entry.startSec * fps));
    int32 endFrame = static_cast<int32>(std::round((entry.startSec + entry.durationSec) * fps));

    if (ImGui::InputInt("Start Frame##SelectedState", &startFrame))
    {
        startFrame = std::clamp(startFrame, Get_FrameMin(), endFrame - 1);
        entry.durationSec = static_cast<float>(endFrame - startFrame) / static_cast<float>(fps);
        entry.startSec = static_cast<float>(startFrame) / static_cast<float>(fps);
        MarkDirty();
    }

    if (ImGui::InputInt("End Frame##SelectedState", &endFrame))
    {
        endFrame = std::clamp(endFrame, startFrame + 1, Get_FrameMax());
        entry.durationSec = static_cast<float>(endFrame - startFrame) / static_cast<float>(fps);
        MarkDirty();
    }

    auto inspector = AnimNotify_Inspector_Factory::GetInstance()->Get_NotifyStateInspector(entry.notifyState->Get_TypeName());
    if (inspector)
        inspector->Draw_Inspector(entry.notifyState);
    else
        ImGui::TextDisabled("No custom payload inspector");

    if (ImGui::Button("Delete Notify State"))
        Delete_SelectedState();
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

    int32 startFrame = (_sequencerContext.pendingMarkStartFrame >= 0) ? _sequencerContext.pendingMarkStartFrame : _sequencerState.currentFrame;
    int32 endFrame = (_sequencerContext.pendingMarkEndFrame >= 0) ? _sequencerContext.pendingMarkEndFrame : (_sequencerState.currentFrame + 10);

    startFrame = std::clamp(startFrame, Get_FrameMin(), Get_FrameMax());
    endFrame = std::clamp(endFrame, startFrame + 1, Get_FrameMax());

    if (ImGui::InputInt("Start Frame##CreateState", &startFrame))
        _requestedCreateStateStartFrame = std::clamp(startFrame, Get_FrameMin(), endFrame - 1);
    else
        _requestedCreateStateStartFrame = startFrame;

    if (ImGui::InputInt("End Frame##CreateState", &endFrame))
        _requestedCreateStateEndFrame = std::clamp(endFrame, _requestedCreateStateStartFrame + 1, Get_FrameMax());
    else
        _requestedCreateStateEndFrame = endFrame;

    if (ImGui::Button("Add Notify State"))
        Add_State_ByFrameRange(_requestedCreateStateStartFrame, _requestedCreateStateEndFrame, typeNames[_createStateTypeIndex]);
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

void Animation_View::Begin_CreateStatePopup(int32 startFrame, int32 endFrame)
{
    _requestedCreateStateStartFrame = std::clamp(startFrame, Get_FrameMin(), Get_FrameMax());
    _requestedCreateStateEndFrame = std::clamp(endFrame, _requestedCreateStateStartFrame + 1, Get_FrameMax());
    _openCreateStatePopup = true;
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

    if (_model)
    {
        const uint32 count = _model->Get_AnimationCount();
        for (uint32 i = 0; i < count; ++i)
        {
            auto& clip = AnimNotify_Serializer::Get_OrAddClip(_asset, _model->Get_AnimationName(i));
            clip.displayFps = max(1, clip.displayFps);
        }
    }

    ClearDirty();
}

void Animation_View::Save_NotifyAsset()
{
    if (_modelGuid.empty())
        return;

    _asset.modelGuid = _modelGuid;

    AnimNotify_Serializer::Save_ToFile(
        AnimNotify_Serializer::Get_ModelNotifyFilePath(_modelGuid).wstring(),
        _asset);

    GAME->Scan_Assets(TEXT("../../Client/Bin/Resources"));
    ClearDirty();
}

void Animation_View::Refresh_CurrentClip()
{
    _selectedNotifyIndex = -1;
    _selectedStateIndex = -1;

    _sequencerState.currentFrame = Get_FrameMin();
    _previewPlaybackTimeSec = 0.f;
    _isPlaying = false;

    _sequencerContext.clip = Get_CurrentClip();

    if (_model && _selectedClipIndex >= 0)
    {
        _model->Set_Animation(static_cast<uint32>(_selectedClipIndex), false);
        Apply_CurrentFrame_ToPreview();
    }
        
}

void Animation_View::Apply_CurrentFrame_ToPreview()
{
    if (!_model || _selectedClipIndex < 0)
        return;

    const float timeSec = Get_CurrentFrameTimeSec();
    const float ticksPerSecond = _model->Get_AnimationTicksPerSecond(static_cast<uint32>(_selectedClipIndex));
    if (ticksPerSecond <= FLT_EPSILON)
        return;

    const float trackPosition = timeSec * ticksPerSecond;

    _model->Set_CurrentTrackPositionTicks(trackPosition);
    _model->Sample_CurrentPose();
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

void Animation_View::Fit_PreviewCamera_ToOwner()
{
    if (!_previewCamera || !_previewOwner)
        return;

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
        _sequencerState.currentFrame = Get_FrameMin();
        Apply_CurrentFrame_ToPreview();
    }

    _previewPlaybackTimeSec = Get_CurrentFrameTimeSec();
    _isPlaying = true;
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

Shared<Animation_View> Animation_View::Create()
{
    return make_shared<Animation_View>();
}
