#include "pch.h"
#include "AnimSequencerAdapter.h"

#include "Animation_View.h"
#include "AnimNotify_Types.h"
#include "AnimNotify.h"
#include "AnimNotifyState.h"

#include <algorithm>

using namespace Engine;

AnimSequencerAdapter::AnimSequencerAdapter(FSequencerUIState* state, FAnimSequencerContext* context)
    : Editor_SequencerAdapterBase(state)
    , _context(context)
{
}

FAnimNotifyClipData* AnimSequencerAdapter::Get_Clip() const
{
    if (!_context)
        return nullptr;

    return _context->clip;
}

int32 AnimSequencerAdapter::Get_ClipFps() const
{
    if (_context && _context->view)
        return max(1, _context->view->Get_CurrentClipFps());

    return 30;
}

int AnimSequencerAdapter::GetFrameMin() const
{
    if (!_context || !_context->view)
        return 0;

    return _context->view->Get_FrameMin();
}

int AnimSequencerAdapter::GetFrameMax() const
{
    if (!_context || !_context->view)
        return 0;

    return _context->view->Get_FrameMax();
}

int AnimSequencerAdapter::GetItemCount() const
{
    auto* clip = Get_Clip();
    if (!clip)
        return 0;

    return static_cast<int>(clip->notifyTracks.size() + clip->notifyStateTracks.size());
}

int AnimSequencerAdapter::GetItemTypeCount() const
{
    return 0;
}

const char* AnimSequencerAdapter::GetItemTypeName(int typeIndex) const
{
    return "";
}

bool AnimSequencerAdapter::Resolve_TrackRow(int32 rowIndex, FResolvedTrackRow& outRow) const
{
    auto* clip = Get_Clip();
    if (!clip || rowIndex < 0)
        return false;

    const int32 notifyTrackCount = static_cast<int32>(clip->notifyTracks.size());
    if (rowIndex < notifyTrackCount)
    {
        outRow.isStateTrack = false;
        outRow.trackIndex = rowIndex;
        return true;
    }

    const int32 stateRowIndex = rowIndex - notifyTrackCount;
    if (stateRowIndex < 0 || stateRowIndex >= static_cast<int32>(clip->notifyStateTracks.size()))
        return false;

    outRow.isStateTrack = true;
    outRow.trackIndex = stateRowIndex;
    return true;
}

const char* AnimSequencerAdapter::GetItemLabel(int index) const
{
    auto* clip = Get_Clip();
    if (!clip)
        return "";

    FResolvedTrackRow row;
    if (!Resolve_TrackRow(index, row))
        return "";

    if (_trackLabels.size() <= static_cast<size_t>(index))
        _trackLabels.resize(index + 1);

    if (!row.isStateTrack)
    {
        _trackLabels[index] = clip->notifyTracks[row.trackIndex].name.empty()
            ? "Notify Track " + to_string(row.trackIndex)
            : clip->notifyTracks[row.trackIndex].name;
    }
    else
    {
        _trackLabels[index] = clip->notifyStateTracks[row.trackIndex].name.empty()
            ? "Notify State Track " + to_string(row.trackIndex)
            : clip->notifyStateTracks[row.trackIndex].name;
    }

    return _trackLabels[index].c_str();
}

void AnimSequencerAdapter::Get(int index, int** start, int** end, int* type, unsigned int* color)
{
    FResolvedTrackRow row;
    if (!Resolve_TrackRow(index, row))
        return;

    if (_trackStarts.size() <= static_cast<size_t>(index))
        _trackStarts.resize(index + 1);

    if (_trackEnds.size() <= static_cast<size_t>(index))
        _trackEnds.resize(index + 1);

    _trackStarts[index] = GetFrameMin();
    _trackEnds[index] = GetFrameMax();

    if (start)
        *start = &_trackStarts[index];

    if (end)
        *end = &_trackEnds[index];

    if (type)
        *type = index;

    if (color)
    {
        if (!row.isStateTrack)
            *color = FTrackColors::NotifyFill;
        else
            *color = FTrackColors::StateFill;
    }
}

void AnimSequencerAdapter::Add(int type)
{
}

void AnimSequencerAdapter::Del(int index)
{
    if (!_context || !_context->view)
        return;

    if (!_context->isSelectedStateTrack)
        _context->view->Delete_SelectedNotify();
    else
        _context->view->Delete_SelectedState();
}

void AnimSequencerAdapter::BeginEdit(int index)
{
    if (_state)
        _state->selectedEntry = index;
}

void AnimSequencerAdapter::EndEdit()
{
    if (_context && _context->view)
        _context->view->MarkDirty();
}

void AnimSequencerAdapter::CustomDraw(
    int index,
    ImDrawList* drawList,
    const ImRect& rc,
    const ImRect& legendRect,
    const ImRect& clippingRect,
    const ImRect& legendClippingRect)
{
    auto* clip = Get_Clip();
    if (!clip || !_context)
        return;

    FResolvedTrackRow row;
    if (!Resolve_TrackRow(index, row))
        return;

    if (!row.isStateTrack)
    {
        Draw_NotifyTrack(row.trackIndex, drawList, rc);
        return;
    }

    Draw_StateTrack(row.trackIndex, drawList, rc);
    Handle_StateMarkGesture(row.trackIndex, rc);
}

int32 AnimSequencerAdapter::Clamp_Frame(int32 frame) const
{
    return std::clamp(frame, GetFrameMin(), GetFrameMax());
}

int32 AnimSequencerAdapter::TimeSec_ToFrame(float timeSec, int32 fps)
{
    return static_cast<int32>(std::round(timeSec * static_cast<float>(max(1, fps))));
}

float AnimSequencerAdapter::Frame_ToTimeSec(int32 frame, int32 fps)
{
    return static_cast<float>(frame) / static_cast<float>(max(1, fps));
}

float AnimSequencerAdapter::Get_PixelPerFrame(const ImRect& rc) const
{
    const float frameCount = static_cast<float>(max(1, GetFrameMax() - GetFrameMin()));
    return (rc.Max.x - rc.Min.x) / frameCount;
}

int32 AnimSequencerAdapter::Pixel_ToFrame(float pixelX, const ImRect& rc) const
{
    const float pixelPerFrame = Get_PixelPerFrame(rc);
    const float relativeX = pixelX - rc.Min.x;
    const int32 frame = GetFrameMin() + static_cast<int32>(std::round(relativeX / max(pixelPerFrame, 0.0001f)));
    return Clamp_Frame(frame);
}

float AnimSequencerAdapter::Frame_ToPixel(int32 frame, const ImRect& rc) const
{
    return rc.Min.x + static_cast<float>(frame - GetFrameMin()) * Get_PixelPerFrame(rc);
}

void AnimSequencerAdapter::Select_Notify(int32 notifyIndex, int32 trackIndex)
{
    if (_context && _context->selectedNotifyIndex)
        *_context->selectedNotifyIndex = notifyIndex;

    if (_context && _context->selectedStateIndex)
        *_context->selectedStateIndex = -1;

    if (_context)
    {
        _context->isSelectedStateTrack = false;
        _context->selectedTrackIndex = trackIndex;
    }
}

void AnimSequencerAdapter::Select_State(int32 stateIndex, int32 trackIndex)
{
    if (_context && _context->selectedStateIndex)
        *_context->selectedStateIndex = stateIndex;

    if (_context && _context->selectedNotifyIndex)
        *_context->selectedNotifyIndex = -1;

    if (_context)
    {
        _context->isSelectedStateTrack = true;
        _context->selectedTrackIndex = trackIndex;
    }
}

void AnimSequencerAdapter::Clear_Selection()
{
    if (_context && _context->selectedNotifyIndex)
        *_context->selectedNotifyIndex = -1;

    if (_context && _context->selectedStateIndex)
        *_context->selectedStateIndex = -1;
}

void AnimSequencerAdapter::Draw_NotifyTrack(int32 trackIndex, ImDrawList* drawList, const ImRect& rc)
{
    auto* clip = Get_Clip();
    if (!clip || !_context || !_context->view)
        return;

    ImGuiIO& io = ImGui::GetIO();
    const int32 fps = Get_ClipFps();
    const bool isSelectedTrack =
        !_context->isSelectedStateTrack &&
        _context->selectedTrackIndex == trackIndex;

    if (isSelectedTrack)
    {
        drawList->AddRectFilled(
            ImVec2(rc.Min.x, rc.Min.y + 1.f),
            ImVec2(rc.Max.x, rc.Max.y - 1.f),
            0x2223A85A,
            4.f);

        drawList->AddRect(
            ImVec2(rc.Min.x, rc.Min.y + 1.f),
            ImVec2(rc.Max.x, rc.Max.y - 1.f),
            FTrackColors::SelectedBorder,
            4.f,
            0,
            1.5f);
    }

    for (int32 i = 0; i < static_cast<int32>(clip->notifies.size()); ++i)
    {
        auto& entry = clip->notifies[i];
        if (!entry.notify || entry.trackIndex != trackIndex)
            continue;

        const int32 frame = TimeSec_ToFrame(entry.timeSec, fps);
        const float x = Frame_ToPixel(frame, rc);

        const float markerWidth = 64.f;
        const float markerHeight = (rc.Max.y - rc.Min.y) - 6.f;

        ImVec2 rectMin(x, rc.Min.y + 3.f);
        ImVec2 rectMax(x + markerWidth, rectMin.y + markerHeight);
        ImRect notifyRect(rectMin, rectMax);

        const bool isSelected =
            _context->selectedNotifyIndex &&
            (*_context->selectedNotifyIndex == i);

        const bool isHovered = notifyRect.Contains(io.MousePos);

        const unsigned int fillColor = isHovered ? FTrackColors::NotifyHover : FTrackColors::NotifyFill;
        const unsigned int borderColor = isSelected ? FTrackColors::SelectedBorder : FTrackColors::NotifyBorder;
        const float borderThickness = isSelected ? 2.f : 1.f;

        drawList->AddRectFilled(rectMin, rectMax, fillColor, 3.f);
        drawList->AddRect(rectMin, rectMax, borderColor, 3.f, 0, borderThickness);

        const string label = entry.notify->Get_TypeName();
        const ImVec2 textSize = ImGui::CalcTextSize(label.c_str());
        const float textX = rectMin.x + (markerWidth - textSize.x) * 0.5f;
        const float textY = rectMin.y + (markerHeight - textSize.y) * 0.5f;
        drawList->AddText(ImVec2(textX, textY), FTrackColors::Text, label.c_str());

        if (notifyRect.Contains(io.MousePos) && ImGui::IsMouseClicked(0))
        {
            Select_Notify(i, trackIndex);
            _context->clickedOnNotify = true;

            _draggingNotifyIndex = i;
            _notifyDragOffsetX = io.MousePos.x - rectMin.x;
        }

        if (_draggingNotifyIndex == i && ImGui::IsMouseDown(0))
        {
            const int32 newFrame = Pixel_ToFrame((io.MousePos.x - _notifyDragOffsetX), rc);
            entry.timeSec = Frame_ToTimeSec(newFrame, fps);

            if (_context->view)
                _context->view->MarkDirty();
        }
    }

    if (_draggingNotifyIndex >= 0 && !ImGui::IsMouseDown(0))
    {
        auto selectedNotify = clip->notifies[_draggingNotifyIndex].notify;
        Sort_Notifies(*clip);
        Select_Notify(Find_NotifyIndex_ByInstance(*clip, selectedNotify), trackIndex);

        _draggingNotifyIndex = -1;
        _notifyDragOffsetX = 0.f;
    }

    if (!_context->clickedOnNotify &&
        !io.KeyShift &&
        rc.Contains(io.MousePos) &&
        ImGui::IsMouseClicked(0))
    {
        Clear_Selection();
        Select_Notify(-1, trackIndex);
        _context->clickedOnNotify = true;
    }
}

void AnimSequencerAdapter::Draw_StateTrack(int32 trackIndex, ImDrawList* drawList, const ImRect& rc)
{
    auto* clip = Get_Clip();
    if (!clip)
        return;

    ImGuiIO& io = ImGui::GetIO();
    const int32 fps = Get_ClipFps();
    const float handleWidth = 10.f;
    const bool isSelectedTrack =
        _context->isSelectedStateTrack &&
        _context->selectedTrackIndex == trackIndex;

    if (isSelectedTrack)
    {
        drawList->AddRectFilled(
            ImVec2(rc.Min.x, rc.Min.y + 1.f),
            ImVec2(rc.Max.x, rc.Max.y - 1.f),
            0x221A76C4,
            4.f);

        drawList->AddRect(
            ImVec2(rc.Min.x, rc.Min.y + 1.f),
            ImVec2(rc.Max.x, rc.Max.y - 1.f),
            FTrackColors::SelectedBorder,
            4.f,
            0,
            1.5f);
    }

    for (int32 i = 0; i < static_cast<int32>(clip->notifyStates.size()); ++i)
    {
        auto& entry = clip->notifyStates[i];
        if (!entry.notifyState || entry.trackIndex != trackIndex)
            continue;

        const int32 startFrame = TimeSec_ToFrame(entry.startSec, fps);
        const int32 durationFrame = max(1, TimeSec_ToFrame(entry.durationSec, fps));
        const int32 endFrame = startFrame + durationFrame;

        const float x1 = Frame_ToPixel(startFrame, rc);
        const float x2 = Frame_ToPixel(endFrame, rc);
        const float y1 = rc.Min.y + 3.f;
        const float y2 = rc.Max.y - 3.f;

        const ImRect fullRect(ImVec2(x1, y1), ImVec2(x2, y2));
        const ImRect leftHandle(ImVec2(x1, y1), ImVec2(x1 + handleWidth, y2));
        const ImRect rightHandle(ImVec2(x2 - handleWidth, y1), ImVec2(x2, y2));
        const ImRect centerRect(ImVec2(x1 + handleWidth, y1), ImVec2(x2 - handleWidth, y2));

        const bool isSelected =
            _context->selectedStateIndex &&
            (*_context->selectedStateIndex == i);

        const bool isHovered = fullRect.Contains(io.MousePos);
        const bool isLeftHovered = leftHandle.Contains(io.MousePos);
        const bool isRightHovered = rightHandle.Contains(io.MousePos);

        const unsigned int fillColor = isHovered ? FTrackColors::StateHover : FTrackColors::StateFill;
        const unsigned int borderColor = isSelected ? FTrackColors::SelectedBorder : FTrackColors::StateBorder;
        const unsigned int leftHandleColor = isLeftHovered ? FTrackColors::HandleHover : FTrackColors::Handle;
        const unsigned int rightHandleColor = isRightHovered ? FTrackColors::HandleHover : FTrackColors::Handle;
        const float borderThickness = isSelected ? 2.f : 1.f;

        drawList->AddRectFilled(fullRect.Min, fullRect.Max, fillColor, 3.f);
        drawList->AddRectFilled(leftHandle.Min, leftHandle.Max, leftHandleColor, 3.f);
        drawList->AddRectFilled(rightHandle.Min, rightHandle.Max, rightHandleColor, 3.f);
        drawList->AddRect(fullRect.Min, fullRect.Max, borderColor, 3.f, 0, borderThickness);

        const string label = entry.notifyState->Get_TypeName();
        const ImVec2 textSize = ImGui::CalcTextSize(label.c_str());
        const float textX = fullRect.Min.x + ((fullRect.Max.x - fullRect.Min.x) - textSize.x) * 0.5f;
        const float textY = fullRect.Min.y + ((fullRect.Max.y - fullRect.Min.y) - textSize.y) * 0.5f;
        drawList->AddText(ImVec2(textX, textY), FTrackColors::Text, label.c_str());

        if (ImGui::IsMouseClicked(0))
        {
            if (leftHandle.Contains(io.MousePos))
            {
                Select_State(i, trackIndex);
                _context->clickedOnNotify = true;
                _draggingStateIndex = i;
                _stateDragMode = EStateDragMode::ResizeStart;
            }
            else if (rightHandle.Contains(io.MousePos))
            {
                Select_State(i, trackIndex);
                _context->clickedOnNotify = true;
                _draggingStateIndex = i;
                _stateDragMode = EStateDragMode::ResizeEnd;
            }
            else if (centerRect.Contains(io.MousePos))
            {
                Select_State(i, trackIndex);
                _context->clickedOnNotify = true;
                _draggingStateIndex = i;
                _stateDragMode = EStateDragMode::Move;
                _stateDragOffsetX = io.MousePos.x - x1;
            }
        }

        if (_draggingStateIndex == i && ImGui::IsMouseDown(0))
        {
            const int32 currentStartFrame = TimeSec_ToFrame(entry.startSec, fps);
            const int32 currentDurationFrame = max(1, TimeSec_ToFrame(entry.durationSec, fps));
            const int32 currentEndFrame = currentStartFrame + currentDurationFrame;

            if (_stateDragMode == EStateDragMode::Move)
            {
                const int32 newStartFrame = Pixel_ToFrame(io.MousePos.x - _stateDragOffsetX, rc);
                const int32 clampedStartFrame = std::clamp(
                    newStartFrame,
                    GetFrameMin(),
                    max(GetFrameMin(), GetFrameMax() - currentDurationFrame));

                entry.startSec = Frame_ToTimeSec(clampedStartFrame, fps);
            }
            else if (_stateDragMode == EStateDragMode::ResizeStart)
            {
                const int32 newStartFrame = std::clamp(
                    Pixel_ToFrame(io.MousePos.x, rc),
                    GetFrameMin(),
                    currentEndFrame - 1);

                entry.startSec = Frame_ToTimeSec(newStartFrame, fps);
                entry.durationSec = Frame_ToTimeSec(currentEndFrame - newStartFrame, fps);
            }
            else if (_stateDragMode == EStateDragMode::ResizeEnd)
            {
                const int32 newEndFrame = std::clamp(
                    Pixel_ToFrame(io.MousePos.x, rc),
                    currentStartFrame + 1,
                    GetFrameMax());

                entry.durationSec = Frame_ToTimeSec(newEndFrame - currentStartFrame, fps);
            }

            if (_context->view)
                _context->view->MarkDirty();
        }
    }

    if (_draggingStateIndex >= 0 && !ImGui::IsMouseDown(0))
    {
        auto selectedState = clip->notifyStates[_draggingStateIndex].notifyState;
        const int32 selectedTrackIndex = clip->notifyStates[_draggingStateIndex].trackIndex;
        Sort_States(*clip);
        Select_State(Find_StateIndex_ByInstance(*clip, selectedState), selectedTrackIndex);

        _draggingStateIndex = -1;
        _stateDragOffsetX = 0.f;
        _stateDragMode = EStateDragMode::None;
    }

    if (!_context->clickedOnNotify &&
        !io.KeyShift &&
        rc.Contains(io.MousePos) &&
        ImGui::IsMouseClicked(0))
    {
        Clear_Selection();
        Select_State(-1, trackIndex);
        _context->clickedOnNotify = true;
    }

    if (_context->isMarkingState &&
        _context->isSelectedStateTrack &&
        _context->selectedTrackIndex == trackIndex)
    {
        const int32 startFrame = min(_context->pendingMarkStartFrame, _context->pendingMarkEndFrame);
        const int32 endFrame = max(_context->pendingMarkStartFrame, _context->pendingMarkEndFrame);

        const float x1 = Frame_ToPixel(startFrame, rc);
        const float x2 = Frame_ToPixel(endFrame, rc);

        drawList->AddRectFilled(
            ImVec2(x1, rc.Min.y + 3.f),
            ImVec2(x2, rc.Max.y - 3.f),
            0x5588BBFF,
            3.f);

        drawList->AddRect(
            ImVec2(x1, rc.Min.y + 3.f),
            ImVec2(x2, rc.Max.y - 3.f),
            FTrackColors::SelectedBorder,
            3.f,
            0,
            2.f);
    }
}

void AnimSequencerAdapter::Handle_StateMarkGesture(int32 trackIndex, const ImRect& rc)
{
    if (!_context)
        return;

    ImGuiIO& io = ImGui::GetIO();

    if (!rc.Contains(io.MousePos))
        return;

    if (_draggingNotifyIndex >= 0 || _draggingStateIndex >= 0)
        return;

    if (!_context->isMarkingState &&
        io.KeyShift &&
        ImGui::IsMouseClicked(ImGuiMouseButton_Left))
    {
        _context->isSelectedStateTrack = true;
        _context->selectedTrackIndex = trackIndex;
        _context->isMarkingState = true;

        const int32 startFrame = Pixel_ToFrame(io.MousePos.x, rc);
        _context->pendingMarkStartFrame = startFrame;
        _context->pendingMarkEndFrame = startFrame;
        return;
    }

    if (_context->isMarkingState &&
        _context->isSelectedStateTrack &&
        _context->selectedTrackIndex == trackIndex &&
        ImGui::IsMouseDown(ImGuiMouseButton_Left))
    {
        _context->pendingMarkEndFrame = Pixel_ToFrame(io.MousePos.x, rc);
        return;
    }

    if (_context->isMarkingState &&
        _context->isSelectedStateTrack &&
        _context->selectedTrackIndex == trackIndex &&
        ImGui::IsMouseReleased(ImGuiMouseButton_Left))
    {
        _context->pendingMarkEndFrame = Pixel_ToFrame(io.MousePos.x, rc);
        _context->isMarkingState = false;
    }
}

void AnimSequencerAdapter::Sort_Notifies(FAnimNotifyClipData& clip)
{
    sort(clip.notifies.begin(), clip.notifies.end(),
        [](const FAnimNotifyEventEntry& lhs, const FAnimNotifyEventEntry& rhs)
        {
            if (lhs.trackIndex == rhs.trackIndex)
                return lhs.timeSec < rhs.timeSec;

            return lhs.trackIndex < rhs.trackIndex;
        });
}

void AnimSequencerAdapter::Sort_States(FAnimNotifyClipData& clip)
{
    sort(clip.notifyStates.begin(), clip.notifyStates.end(),
        [](const FAnimNotifyStateEntry& lhs, const FAnimNotifyStateEntry& rhs)
        {
            if (lhs.trackIndex != rhs.trackIndex)
                return lhs.trackIndex < rhs.trackIndex;

            if (lhs.startSec == rhs.startSec)
                return lhs.durationSec < rhs.durationSec;

            return lhs.startSec < rhs.startSec;
        });
}

int32 AnimSequencerAdapter::Find_NotifyIndex_ByInstance(
    const FAnimNotifyClipData& clip,
    const Shared<AnimNotify>& notify)
{
    for (int32 i = 0; i < static_cast<int32>(clip.notifies.size()); ++i)
    {
        if (clip.notifies[i].notify == notify)
            return i;
    }

    return -1;
}

int32 AnimSequencerAdapter::Find_StateIndex_ByInstance(
    const FAnimNotifyClipData& clip,
    const Shared<AnimNotifyState>& notifyState)
{
    for (int32 i = 0; i < static_cast<int32>(clip.notifyStates.size()); ++i)
    {
        if (clip.notifyStates[i].notifyState == notifyState)
            return i;
    }

    return -1;
}
