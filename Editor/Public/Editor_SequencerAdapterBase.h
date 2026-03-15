#pragma once

NS_BEGIN(Editor)

class Editor_SequencerAdapterBase : public ImSequencer::SequenceInterface
{
public:
    explicit Editor_SequencerAdapterBase(FSequencerUIState* state);
    virtual ~Editor_SequencerAdapterBase() = default;

public:
    FSequencerUIState* Get_State() const { return _state; }
    size_t GetCustomHeight(int) override { return 24; }

protected:
    FSequencerUIState* _state = nullptr;

};

NS_END
