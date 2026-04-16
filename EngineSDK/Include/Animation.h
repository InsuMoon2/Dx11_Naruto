#pragma once

#include "Base.h"

NS_BEGIN(Engine)

class Bone;
class Channel;

class ENGINE_DLL Animation final : public Base
{
public:
    Animation();
    Animation(const Animation& rhs);
    virtual ~Animation() = default;

public:
    HRESULT Initialize(const FAnimationClipRaw& src);
    void    Reset();

    // true면, Loop가 아닐 때 애니메이션 끝에 도달
    bool    Update_TransformationMatrices(float timeDelta, vector<Shared<Bone>>& bones, bool isLoop);

    // 트랙 위치만 진행하고 종료 여부 반환
    bool    Advance_TrackPosition(float timeDelta, bool isLoop, float& inOutTrackPosition) const;

    // ㅅ현재 트랙 위치의 로컬 포즈 샘플링
    void    Sample_LocalPoses(float trackPosition, vector<FAnimationLocalPose>& inOutPoses) const;

    // AssimpTool이 다른 본 순서로 추출한 animbin도 런타임에서 안전하게 다시 맞추기 위해 사용
    void    Rebind_BoneIndices(const vector<Shared<Bone>>& bones);

    const string& Get_Name() const { return _name; }
                  
    float         Get_Duration() const { return _duration; }
    float         Get_TicksPerSecond() const { return _ticksPersecond; }
                  
    const string& Get_SourcePath() const { return _sourcePath; }
    void          Set_SourcePath(const string& path) { _sourcePath = path; }

private:
    string  _name;
    float   _duration = 0.f;
    float   _ticksPersecond = 25.f;
    float   _currentTrackPosition = 0.f;

    vector<Shared<Channel>> _channels;

    string  _sourcePath;

public:
    static Shared<Animation> Create(const FAnimationClipRaw& src);
    Shared<Animation>        Clone();
    void Free() override;
};

NS_END
