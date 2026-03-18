#pragma once

NS_BEGIN(Engine)
class AnimNotify;
NS_END

NS_BEGIN(Editor)

class AnimNotify_Inspector
{
public:
    virtual ~AnimNotify_Inspector() = default;
    virtual void Draw_Inspector(Shared<AnimNotify> notify) = 0;
};

NS_END
