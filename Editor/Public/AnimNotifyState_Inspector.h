#pragma once

NS_BEGIN(Engine)
class AnimNotifyState;
NS_END

NS_BEGIN(Editor)

class AnimNotifyState_Inspector
{
public:
    virtual ~AnimNotifyState_Inspector() = default;
    virtual void Draw_Inspector(Shared<AnimNotifyState> notifyState) = 0;
};

NS_END
