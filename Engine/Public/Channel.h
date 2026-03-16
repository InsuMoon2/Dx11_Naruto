#pragma once

#include "Base.h"

NS_BEGIN(Engine)

class Bone;

class ENGINE_DLL Channel final : public Base
{
public:
    Channel() = default;
    virtual ~Channel() = default;

public:
    HRESULT Initialize(const FAnimationChannelRaw& src);

    void    Reset();
    void    Update_TransformationMatrix(float trackPosition, vector<Shared<Bone>>& bones);

private:
    void    Apply_KeyFrame(const FKeyFrame& key, const Shared<Bone>& bone);

private:
    string              _nodeName;
    int32               _boneIndex = -1;

    vector<FKeyFrame>   _keyFrames;
    uint32              _currentKeyFrameIndex = 0;

public:
    static Shared<Channel> Create(const FAnimationChannelRaw& src);
    void    Free() override;

};

NS_END
