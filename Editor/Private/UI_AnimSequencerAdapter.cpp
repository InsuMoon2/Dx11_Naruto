#include "pch.h"
#include "UI_AnimSequencerAdapter.h"

#include <magic_enum/magic_enum.hpp>

#include "UI_Animation_View.h"
#include "UI_AnimUtility.h"
#include "UIObject.h"

UI_AnimSequencerAdapter::UI_AnimSequencerAdapter(FSequencerUIState* state, FUIAnimSequencerContext* context)
    : Editor_SequencerAdapterBase(state)
    , _context(context)
{
}

FUIAnimAsset* UI_AnimSequencerAdapter::Get_AssetPtr() const
{
    if (!_context || !_context->asset)
        return nullptr;

    return _context->asset->get();
}

bool UI_AnimSequencerAdapter::Has_FrameConflict(const Engine::FUIAnimTrack& track, int ignoreKeyIndex, int frame) const
{
    for (int keyIndex = 0; keyIndex < static_cast<int>(track.keys.size()); ++keyIndex)
    {
        if (keyIndex == ignoreKeyIndex)
            continue;

        if (track.keys[keyIndex].frame == frame)
            return true;
    }

    return false;
}

int UI_AnimSequencerAdapter::GetFrameMin() const
{
    auto* asset = Get_AssetPtr();
    return asset ? asset->startFrame : 0;
}

int UI_AnimSequencerAdapter::GetFrameMax() const
{
    auto* asset = Get_AssetPtr();
    return asset ? asset->endFrame : 0;
}

int UI_AnimSequencerAdapter::GetItemCount() const
{
    auto* asset = Get_AssetPtr();
    return asset ? static_cast<int>(asset->tracks.size()) : 0;
}

int UI_AnimSequencerAdapter::GetItemTypeCount() const
{
    return static_cast<int>(Engine::EUIAnimProperty::END);
}

const char* UI_AnimSequencerAdapter::GetItemTypeName(int typeIndex) const
{
    const auto property = static_cast<Engine::EUIAnimProperty>(typeIndex);

    if (!magic_enum::enum_contains(property))
        return "";

    if (property == Engine::EUIAnimProperty::END)
        return "";

    return magic_enum::enum_name(property).data();
}

const char* UI_AnimSequencerAdapter::GetItemLabel(int index) const
{
    auto* asset = Get_AssetPtr();
    if (!asset)
        return "";

    if (index < 0 || index >= static_cast<int>(asset->tracks.size()))
        return "";

    if (_cachedLabels.size() != asset->tracks.size())
        RebuildCache();

    return _cachedLabels[index].c_str();
}

void UI_AnimSequencerAdapter::Get(int index, int** start, int** end, int* type, unsigned* color)
{
    auto* asset = Get_AssetPtr();
    if (!asset)
        return;

    if (index < 0 || index >= static_cast<int>(asset->tracks.size()))
        return;

    if (_cachedStarts.size() != asset->tracks.size())
        RebuildCache();

    if (start)
        *start = &_cachedStarts[index];

    if (end)
        *end = &_cachedEnds[index];

    if (type)
        *type = static_cast<int>(asset->tracks[index].property);

    if (color)
    {
        switch (asset->tracks[index].property)
        {
        case Engine::EUIAnimProperty::PositionX: *color = 0xFF4AA3FF; break;
        case Engine::EUIAnimProperty::PositionY: *color = 0xFF4AFFA3; break;
        case Engine::EUIAnimProperty::RotationZ: *color = 0xFFFFB347; break;
        case Engine::EUIAnimProperty::ScaleX:    *color = 0xFFE573FF; break;
        case Engine::EUIAnimProperty::ScaleY:    *color = 0xFFC084FC; break;
        case Engine::EUIAnimProperty::Alpha:     *color = 0xFFFF6B6B; break;
        case Engine::EUIAnimProperty::ColorRGBA: *color = 0xFFFFFF66; break;
        default:                                 *color = 0xFF888888; break;
        }
    }
}

void UI_AnimSequencerAdapter::Add(int type)
{
    auto* asset = Get_AssetPtr();
    if (!asset || !_context || !_context->view)
        return;

    const Engine::EUIAnimProperty property = static_cast<Engine::EUIAnimProperty>(type);
    if (_context->view->Has_Track(property))
        return;

    Engine::FUIAnimTrack track;
    track.property = property;
    asset->tracks.push_back(track);

    if (_context->selectedTrackIndex)
        *_context->selectedTrackIndex = static_cast<int>(asset->tracks.size()) - 1;

    if (_context->selectedKeyIndex)
        *_context->selectedKeyIndex = -1;

    if (_state)
        _state->selectedEntry = static_cast<int>(asset->tracks.size()) - 1;

    _context->view->MarkDirty();
    RebuildCache();
}

void UI_AnimSequencerAdapter::Del(int index)
{
    if (!_context || !_context->view)
        return;

    _context->view->Remove_Track(index);
    RebuildCache();
}

void UI_AnimSequencerAdapter::BeginEdit(int index)
{
    if (_context && _context->selectedTrackIndex)
        *_context->selectedTrackIndex = index;

    if (_state)
        _state->selectedEntry = index;
}

void UI_AnimSequencerAdapter::EndEdit()
{
    if (_context && _context->view)
        _context->view->MarkDirty();

    RebuildCache();
}

void UI_AnimSequencerAdapter::CustomDraw(int index, ImDrawList* draw_list, const ImRect& rc, const ImRect& legendRect,
    const ImRect& clippingRect, const ImRect& legendClippingRect)
{
    auto* asset = Get_AssetPtr();
    if (!asset || !_context)
        return;

    if (index < 0 || index >= static_cast<int>(asset->tracks.size()))
        return;

    auto& track = asset->tracks[index];

    const int frameMin = GetFrameMin();
    const int frameMax = GetFrameMax();
    const float frameCount = static_cast<float>(max(1, frameMax - frameMin));
    const float pixelPerFrame = (rc.Max.x - rc.Min.x) / frameCount;

    auto& io = ImGui::GetIO();

    for (int keyIndex = 0; keyIndex < static_cast<int>(track.keys.size()); ++keyIndex)
    {
        auto& key = track.keys[keyIndex];

        float x = rc.Min.x + (key.frame - frameMin) * pixelPerFrame;
        ImRect keyRect(ImVec2(x - 6.f, rc.Min.y + 3.f), ImVec2(x + 6.f, rc.Max.y - 3.f));

        const bool selected =
            _context->selectedTrackIndex && _context->selectedKeyIndex &&
            (*_context->selectedTrackIndex == index) &&
            (*_context->selectedKeyIndex == keyIndex);

        const unsigned int fillColor = selected ? 0xFF00FFFF : 0xFFFFFFFF;
        const unsigned int borderColor = selected ? 0xFF000000 : 0xFF333333;

        draw_list->AddRectFilled(keyRect.Min, keyRect.Max, fillColor, 2.f);
        draw_list->AddRect(keyRect.Min, keyRect.Max, borderColor, 2.f);

        if (keyRect.Contains(io.MousePos) && ImGui::IsMouseClicked(0))
        {
            if (_context->selectedTrackIndex)
                *_context->selectedTrackIndex = index;

            if (_context->selectedKeyIndex)
                *_context->selectedKeyIndex = keyIndex;

            if (_state)
                _state->selectedEntry = index;

            // 드래그 시작 기준 프레임을 기억해 두고, 마우스 이동량을 프레임 단위로 변환한다.
            _draggingTrackIndex = index;
            _draggingKeyIndex = keyIndex;
            _dragStartFrame = key.frame;
            _dragMoved = false;
        }

        if (_draggingTrackIndex == index && _draggingKeyIndex == keyIndex && ImGui::IsMouseDragging(ImGuiMouseButton_Left))
        {
            const float deltaX = io.MousePos.x - io.MouseClickedPos[ImGuiMouseButton_Left].x;
            const int frameDelta = static_cast<int>(deltaX / max(pixelPerFrame, 1.f));
            const int newFrame = clamp(_dragStartFrame + frameDelta, frameMin, frameMax);

            // 같은 frame에 다른 키가 있으면 이동을 막아서 기존 키를 보존한다.
            if (newFrame != key.frame && !Has_FrameConflict(track, keyIndex, newFrame))
            {
                key.frame = newFrame;
                _dragMoved = true;
                RebuildCache();

                if (_context->view)
                    _context->view->On_KeyFrameDragged(track, keyIndex);
            }
        }

        if (_draggingTrackIndex == index && _draggingKeyIndex == keyIndex && ImGui::IsMouseReleased(ImGuiMouseButton_Left))
        {
            const int finalFrame = key.frame;

            _draggingTrackIndex = -1;
            _draggingKeyIndex = -1;
            _dragStartFrame = -1;
            RebuildCache();

            if (_dragMoved && _context->view)
                _context->view->On_KeyFrameDragFinished(track, finalFrame);

            _dragMoved = false;
        }
    }
}

void UI_AnimSequencerAdapter::RebuildCache() const
{
    _cachedStarts.clear();
    _cachedEnds.clear();
    _cachedLabels.clear();

    auto* asset = Get_AssetPtr();
    if (!asset)
        return;

    _cachedStarts.reserve(asset->tracks.size());
    _cachedEnds.reserve(asset->tracks.size());
    _cachedLabels.reserve(asset->tracks.size());

    for (const auto& track : asset->tracks)
    {
        int startFrame = asset->startFrame;
        int endFrame = asset->endFrame;

        if (!track.keys.empty())
        {
            startFrame = track.keys.front().frame;
            endFrame = track.keys.back().frame;
        }

        _cachedStarts.push_back(startFrame);
        _cachedEnds.push_back(endFrame);

        string label = string(magic_enum::enum_name(track.property));
        label += " (";
        label += to_string(track.keys.size());
        label += ")";

        _cachedLabels.push_back(std::move(label));
    }
}
