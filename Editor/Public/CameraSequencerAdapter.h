#pragma once

#include "Editor_SequencerAdapterBase.h"

NS_BEGIN(Engine)
class FCameraTrack;
class FCameraKey;
NS_END

NS_BEGIN(Editor)

class CameraSequencerAdapter : public Editor_SequencerAdapterBase
{
public:
    explicit CameraSequencerAdapter(FSequencerUIState* state, FCameraSequencerContext* context);
    virtual ~CameraSequencerAdapter() = default;

public:
    int GetFrameMin() const override;
    int GetFrameMax() const override;
    int GetItemCount() const override;

    int         GetItemTypeCount() const override;
    const char* GetItemTypeName(int typeIndex) const override;
    const char* GetItemLabel(int index) const override;

    void Get(int index, int** start, int** end,
        int* type, unsigned int* color) override;

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

public:
    FCameraTrack* Get_Track() const;

    // 모드별 색상 구별
    static uint32 Get_ModeColor(ECineCameraMode mode);

private:
    FCameraSequencerContext* _context = nullptr;

    // ImSequence 인터페이스가 포인터를 필요로해서
    mutable int _trackStart = 0;
    mutable int _trackEnd = 0;

    mutable string _trackLabel = "Camera";

};

NS_END
