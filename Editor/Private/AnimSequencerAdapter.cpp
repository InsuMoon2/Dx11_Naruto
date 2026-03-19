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
    return 2;
}

int AnimSequencerAdapter::GetItemTypeCount() const
{
    
    return 0;
}

const char* AnimSequencerAdapter::GetItemTypeName(int typeIndex) const
{
    return "";
}

const char* AnimSequencerAdapter::GetItemLabel(int index) const
{
    if (index < 0 || index >= 2)
        return "";

    return _trackLabels[index].c_str();
}

void AnimSequencerAdapter::Get(int index, int** start, int** end, int* type, unsigned int* color)
{
    if (index < 0 || index >= 2)
        return;

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
        if (index == 0)
            *color = FTrackColors::NotifyFill;
        else
            *color = FTrackColors::StateFill;
    }
}

void AnimSequencerAdapter::Add(int type)
{
    // 생성은 Animation_View 우측 패널 / 타임라인 gesture로 처리
}

void AnimSequencerAdapter::Del(int index)
{
    if (!_context || !_context->view)
        return;

    if (index == 0)
        _context->view->Delete_SelectedNotify();
    else if (index == 1)
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

    if (index == 0)
    {
        Draw_NotifyTrack(drawList, rc);
        return;
    }

    if (index == 1)
    {
        Draw_StateTrack(drawList, rc);
        Handle_StateMarkGesture(rc);
        return;
    }
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

void AnimSequencerAdapter::Select_Notify(int32 notifyIndex)
{
    if (_context && _context->selectedNotifyIndex)
        *_context->selectedNotifyIndex = notifyIndex;

    if (_context && _context->selectedStateIndex)
        *_context->selectedStateIndex = -1;

    if (_state)
        _state->selectedEntry = 0;
}

void AnimSequencerAdapter::Select_State(int32 stateIndex)
{
    if (_context && _context->selectedStateIndex)
        *_context->selectedStateIndex = stateIndex;

    if (_context && _context->selectedNotifyIndex)
        *_context->selectedNotifyIndex = -1;

    if (_state)
        _state->selectedEntry = 1;
}

void AnimSequencerAdapter::Clear_Selection()
{
    if (_context && _context->selectedNotifyIndex)
        *_context->selectedNotifyIndex = -1;

    if (_context && _context->selectedStateIndex)
        *_context->selectedStateIndex = -1;
}

void AnimSequencerAdapter::Draw_NotifyTrack(ImDrawList* drawList, const ImRect& rc)
{
    auto* clip = Get_Clip();
    if (!clip || !_context || !_context->view)
        return;

    ImGuiIO& io = ImGui::GetIO();
    const int32 fps = Get_ClipFps();

    const int32 sourceFps = _context->view->Get_CurrentClipFps();

    for (int32 i = 0; i < static_cast<int32>(clip->notifies.size()); ++i)
    {
        auto& entry = clip->notifies[i];
        if (!entry.notify)
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
            Select_Notify(i);
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
        Select_Notify(Find_NotifyIndex_ByInstance(*clip, selectedNotify));

        _draggingNotifyIndex = -1;
        _notifyDragOffsetX = 0.f;
    }
}

void AnimSequencerAdapter::Draw_StateTrack(ImDrawList* drawList, const ImRect& rc)
{
    auto* clip = Get_Clip();
    if (!clip)
        return;

    ImGuiIO& io = ImGui::GetIO();
    const int32 fps = Get_ClipFps();
    const float handleWidth = 10.f;

    for (int32 i = 0; i < static_cast<int32>(clip->notifyStates.size()); ++i)
    {
        auto& entry = clip->notifyStates[i];
        if (!entry.notifyState)
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
                Select_State(i);
                _context->clickedOnNotify = true;
                _draggingStateIndex = i;
                _stateDragMode = EStateDragMode::ResizeStart;
            }
            else if (rightHandle.Contains(io.MousePos))
            {
                Select_State(i);
                _context->clickedOnNotify = true;
                _draggingStateIndex = i;
                _stateDragMode = EStateDragMode::ResizeEnd;
            }
            else if (centerRect.Contains(io.MousePos))
            {
                Select_State(i);
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
        Sort_States(*clip);
        Select_State(Find_StateIndex_ByInstance(*clip, selectedState));

        _draggingStateIndex = -1;
        _stateDragOffsetX = 0.f;
        _stateDragMode = EStateDragMode::None;
    }

    if (_context->isMarkingState)
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



void AnimSequencerAdapter::Handle_StateMarkGesture(const ImRect& rc)
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
        _context->isMarkingState = true;

        const int32 startFrame = Pixel_ToFrame(io.MousePos.x, rc);
        _context->pendingMarkStartFrame = startFrame;
        _context->pendingMarkEndFrame = startFrame;

        return;
    }

    if (_context->isMarkingState && ImGui::IsMouseDown(ImGuiMouseButton_Left))
    {
        _context->pendingMarkEndFrame = Pixel_ToFrame(io.MousePos.x, rc);
        return;
    }

    if (_context->isMarkingState && ImGui::IsMouseReleased(ImGuiMouseButton_Left))
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
            return lhs.timeSec < rhs.timeSec;
        });
}

void AnimSequencerAdapter::Sort_States(FAnimNotifyClipData& clip)
{
    sort(clip.notifyStates.begin(), clip.notifyStates.end(),
        [](const FAnimNotifyStateEntry& lhs, const FAnimNotifyStateEntry& rhs)
        {
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
