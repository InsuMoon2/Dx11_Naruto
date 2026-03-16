#pragma once

#include "Base.h"

NS_BEGIN(Engine)

class Bone;
class Channel;

class ENGINE_DLL Animation final : public Base
{
public:
    Animation();
    virtual ~Animation() = default;

public:
    HRESULT Initialize(const FAnimationClipRaw& src);
    void    Reset();

    // true면, Loop가 아닐 때 애니메이션 끝에 도달
    bool    Update_TransformationMatrices(float timeDelta, vector<Shared<Bone>>& bones, bool isLoop);

    const string& Get_Name() const { return _name; }

private:
    string  _name;
    float   _duration = 0.f;
    float   _ticksPersecond = 25.f;
    float   _currentTrackPosition = 0.f;

    vector<Shared<Channel>> _channels;

public:
    static Shared<Animation> Create(const FAnimationClipRaw& src);
    void Free() override;
};

NS_END
