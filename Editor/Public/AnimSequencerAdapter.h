#pragma once

#include "Editor_SequencerAdapterBase.h"
#include "Editor_Struct.h"

NS_BEGIN(Engine)
struct FAnimNotifyClipData;
struct FAnimNotifyEventEntry;
struct FAnimNotifyStateEntry;
class  AnimNotifyState;
class  AnimNotify;
NS_END

NS_BEGIN(Editor)

class AnimSequencerAdapter : public Editor_SequencerAdapterBase
{
public:
    explicit AnimSequencerAdapter(FSequencerUIState* state, FAnimSequencerContext* context);
    virtual ~AnimSequencerAdapter() = default;

public:
    int GetFrameMin() const override;
    int GetFrameMax() const override;
    int GetItemCount() const override;

    int GetItemTypeCount() const override;
    const char* GetItemTypeName(int typeIndex) const override;
    const char* GetItemLabel(int index) const override;

    void Get(int index, int** start, int** end, int* type, unsigned int* color) override;

    void Add(int type) override;
    void Del(int index) override;

    void BeginEdit(int index) override;
    void EndEdit() override;

    void CustomDraw(
        int index,
        ImDrawList* drawList,
        const ImRect& rc,
        const ImRect& legendRect,
        const ImRect& clippingRect,
        const ImRect& legendClippingRect) override;

private:
    enum class EStateDragMode : uint8
    {
        None,
        Move,
        ResizeStart,
        ResizeEnd
    };

    struct FTrackColors
    {
        static constexpr unsigned int NotifyFill = 0xFF33CC55;
        static constexpr unsigned int NotifyHover = 0xFF55EE77;
        static constexpr unsigned int NotifyBorder = 0xFF146B2D;

        static constexpr unsigned int StateFill = 0xFF4DA3FF;
        static constexpr unsigned int StateHover = 0xFF79BEFF;
        static constexpr unsigned int StateBorder = 0xFF1B4F8A;

        static constexpr unsigned int SelectedBorder = 0xFF00FFFF;
        static constexpr unsigned int Handle = 0xFF225588;
        static constexpr unsigned int HandleHover = 0xFF3D79C5;
        static constexpr unsigned int Text = 0xFFFFFFFF;
    };

    struct FResolvedTrackRow
    {
        bool isStateTrack = false;
        int32 trackIndex = 0;
    };

private:
    FAnimNotifyClipData* Get_Clip() const;

    int32 Get_ClipFps() const;
    int32 Clamp_Frame(int32 frame) const;

    static int32 TimeSec_ToFrame(float timeSec, int32 fps);
    static float Frame_ToTimeSec(int32 frame, int32 fps);

    float Get_PixelPerFrame(const ImRect& rc) const;
    int32 Pixel_ToFrame(float pixelX, const ImRect& rc) const;
    float Frame_ToPixel(int32 frame, const ImRect& rc) const;

    bool Resolve_TrackRow(int32 rowIndex, FResolvedTrackRow& outRow) const;

    void Select_Notify(int32 notifyIndex, int32 trackIndex);
    void Select_State(int32 stateIndex, int32 trackIndex);
    void Clear_Selection();

    void Draw_NotifyTrack(int32 trackIndex, ImDrawList* drawList, const ImRect& rc);
    void Draw_StateTrack(int32 trackIndex, ImDrawList* drawList, const ImRect& rc);

    void Handle_StateMarkGesture(int32 trackIndex, const ImRect& rc);

    static void Sort_Notifies(FAnimNotifyClipData& clip);
    static void Sort_States(FAnimNotifyClipData& clip);

    static int32 Find_NotifyIndex_ByInstance(
        const FAnimNotifyClipData& clip,
        const Shared<AnimNotify>& notify);

    static int32 Find_StateIndex_ByInstance(
        const FAnimNotifyClipData& clip,
        const Shared<AnimNotifyState>& notifyState);

private:
    FAnimSequencerContext* _context = nullptr;

    mutable vector<int> _trackStarts;
    mutable vector<int> _trackEnds;
    mutable vector<string> _trackLabels;

private:
    int32 _draggingNotifyIndex = -1;
    int32 _draggingStateIndex = -1;
    float _notifyDragOffsetX = 0.f;
    float _stateDragOffsetX = 0.f;

    EStateDragMode _stateDragMode = EStateDragMode::None;
};

NS_END
