#pragma once

#include "Base.h"

NS_BEGIN(Engine)

class Bone;

class ENGINE_DLL Channel final : public Base
{
public:
    Channel();
    Channel(const Channel& rhs);
    virtual ~Channel() = default;

public:
    HRESULT Initialize(const FAnimationChannelRaw& src);

    void    Reset();
    void    Update_TransformationMatrix(float trackPosition, vector<Shared<Bone>>& bones);

    void    Sample_LocalPose(float trackPosition, FAnimationLocalPose& outPose) const;
    int32   Get_BoneIndex() const { return _boneIndex; }
    const string& Get_NodeName() const { return _nodeName; }

    // 애니메이션 채널을 현재 모델의 본 트리에 다시 맞출 때 호출한다.
    // loose animbin을 다른 skeleton에 붙일 때 bone index를 재설정하기 위해 사용한다.
    void    Set_BoneIndex(int32 boneIndex) { _boneIndex = boneIndex; }

private:
    void    Apply_KeyFrame(const FKeyFrame& key, const Shared<Bone>& bone);

private:
    string              _nodeName;
    int32               _boneIndex = -1;

    vector<FKeyFrame>   _keyFrames;
    uint32              _currentKeyFrameIndex = 0;

public:
    static Shared<Channel> Create(const FAnimationChannelRaw& src);
    Shared<Channel>        Clone() const;
    void    Free() override;

};

NS_END
