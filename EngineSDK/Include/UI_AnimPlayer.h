#pragma once

#include "UI_AnimTypes.h"

NS_BEGIN(Engine)

class UIObject;

class ENGINE_DLL UI_AnimPlayer : public Base
{
public:
    explicit UI_AnimPlayer() = default;
    virtual ~UI_AnimPlayer() = default;

public:
    void    Set_Asset(Shared<FUIAnimAsset> asset);
    void    Bind_Target(Shared<UIObject> target);

    void    Play();
    void    Pause();
    void    Stop();

    void    Set_CurrentFrame(int32 frame);
    void    Set_CurrentFrameRatio(float ratio);
    void    Update(float timeDelta);

    bool    Is_Playing() const { return _isPlaying; }
    int32   Get_CurrentFrame() const { return _currentFrame; }

    void    Unbind_Target() { _target.reset(); }

private:
    int32   Adjust_Frame(int32 frame) const;
    void    Apply_Frame(int32 frame);

private:
    Shared<FUIAnimAsset> _asset;
    Shared<UIObject>     _target;

    bool    _isPlaying = false;
    float   _currentTime = 0.f;
    int32   _currentFrame = 0;

public:
    static Shared<UI_AnimPlayer> Create();

};

NS_END
