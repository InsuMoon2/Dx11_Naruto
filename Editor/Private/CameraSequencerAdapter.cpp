#include "pch.h"
#include "CameraSequencerAdapter.h"
#include "Camera_Types.h"
#include "Cinematic_View.h"
#include "magic_enum/magic_enum.hpp"

CameraSequencerAdapter::CameraSequencerAdapter(FSequencerUIState* state, FCameraSequencerContext* context)
    : Editor_SequencerAdapterBase(state)
    , _context(context)
{
}

FCameraTrack* CameraSequencerAdapter::Get_Track() const
{
    return _context ? _context->track : nullptr;
}

int CameraSequencerAdapter::GetFrameMin() const
{
    return 0;
}

int CameraSequencerAdapter::GetFrameMax() const
{
    auto* track = Get_Track();
    return track ? track->totalFrame : 300;
}

int CameraSequencerAdapter::GetItemCount() const
{
    return 1;
}

int CameraSequencerAdapter::GetItemTypeCount() const
{
    return 1;
}

const char* CameraSequencerAdapter::GetItemTypeName(int typeIndex) const
{
    return "Camera";
}

const char* CameraSequencerAdapter::GetItemLabel(int index) const
{
    return "Camera";
}

void CameraSequencerAdapter::Get(int index, int** start, int** end, int* type, unsigned* color)
{
    _trackStart = 0;
    _trackEnd = GetFrameMax();

    if (start) *start = &_trackStart;
    if (end)   *end = &_trackEnd;
    if (type)  *type = 0;
    if (color) *color = 0xFF444444;
}

void CameraSequencerAdapter::Add(int type)
{
    // 키 추가는 CinematicView에서
}

void CameraSequencerAdapter::Del(int index)
{
    // 키 삭제도 CinematicView에서
}

void CameraSequencerAdapter::BeginEdit(int index)
{
    
}

void CameraSequencerAdapter::EndEdit()
{
    
}

void CameraSequencerAdapter::CustomDraw(int index, ImDrawList* drawList, const ImRect& rc, const ImRect& legendRect,
    const ImRect& clippingRect, const ImRect& legendClippingRect)
{
    auto* track = Get_Track();

    if (!track || track->keys.empty())
        return;

    const int frameMin = GetFrameMin();
    const int frameMax = GetFrameMax();

    if (frameMax <= frameMin) return;

    const float trackWidth = rc.Max.x - rc.Min.x;
    const float pixelPerFrame = trackWidth / static_cast<float>(frameMax - frameMin);
    const float centerY = (rc.Min.y + rc.Max.y) * 0.5f;
    const float diamondSize = 6.f;
    // 선택 상태

    const int32 selectedKey = _context->selectedKeyIndex
        ? *_context->selectedKeyIndex : -1;

    for (int32 i = 0; i < static_cast<int32>(track->keys.size()); ++i)
    {
        const auto& key = track->keys[i];
        const float x = rc.Min.x + (key.frame - frameMin) * pixelPerFrame;

        // 클리핑
        if (x < clippingRect.Min.x || x > clippingRect.Max.x)
            continue;

        // 다이아몬드 점 4개
        ImVec2 points[4] = {
            ImVec2(x, centerY - diamondSize),   // top
            ImVec2(x + diamondSize, centerY),    // right
            ImVec2(x, centerY + diamondSize),    // bottom
            ImVec2(x - diamondSize, centerY),    // left
        };

        unsigned int fillColor = Get_ModeColor(key.cameraMode);
        unsigned int borderColor = (i == selectedKey)
            ? 0xFF00FFFF   // 선택: 시안
            : 0xFF222222;  // 기본: 어두운 테두리

        drawList->AddConvexPolyFilled(points, 4, fillColor);
        drawList->AddPolyline(points, 4, borderColor, ImDrawFlags_Closed, 2.f);

        // 모드 약자 표시 (F/T/L/R)
        const char* modeChar = "";
        switch (key.cameraMode)
        {
        case ECineCameraMode::Free:    modeChar = "F"; break;
        case ECineCameraMode::Target:  modeChar = "T"; break;
        case ECineCameraMode::LookAt:  modeChar = "L"; break;
        case ECineCameraMode::Rail:    modeChar = "R"; break;
        default: break;
        }
        drawList->AddText(ImVec2(x - 3.f, centerY - 14.f), 0xFFFFFFFF, modeChar);

        // 클릭 감지
        ImVec2 mousePos = ImGui::GetMousePos();
        float dx = mousePos.x - x;
        float dy = mousePos.y - centerY;
        bool hovered = (dx * dx + dy * dy) < (diamondSize * diamondSize * 2.f);
        if (hovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
        {
            if (_context->selectedKeyIndex)
                *_context->selectedKeyIndex = i;
        }
    }

}

uint32 CameraSequencerAdapter::Get_ModeColor(ECineCameraMode mode)
{
    switch (mode)
    {
    case ECineCameraMode::Free:    return 0xFF4DA3FF; // 파랑
    case ECineCameraMode::Target:  return 0xFF33CC55; // 초록
    case ECineCameraMode::LookAt:  return 0xFFFFCC33; // 노랑
    case ECineCameraMode::Rail:    return 0xFF4444FF; // 빨강
    default:                       return 0xFFAAAAAA;
    }
}
