#pragma once

#include "Editor_SequencerAdapterBase.h"
#include "UI_AnimTypes.h"

NS_BEGIN(Editor)
class UI_Animation_View;
NS_END

NS_BEGIN(Editor)

class UI_AnimSequencerAdapter : public Editor_SequencerAdapterBase
{
public:
    UI_AnimSequencerAdapter(FSequencerUIState* state, FUIAnimSequencerContext* context);

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
        ImDrawList* draw_list,
        const ImRect& rc,
        const ImRect& legendRect,
        const ImRect& clippingRect,
        const ImRect& legendClippingRect) override;

private:
    void RebuildCache() const;
    FUIAnimAsset* Get_AssetPtr() const;

private:
    FUIAnimSequencerContext* _context = nullptr;

    mutable vector<int> _cachedStarts;
    mutable vector<int> _cachedEnds;
    mutable vector<string> _cachedLabels;

};

NS_END
