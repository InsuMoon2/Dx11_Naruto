# CPU Cross-Fade 코드 청사진

## Summary
- 이번 1차는 `DX11_Rokiss`의 `curr / next / tweenRatio` 개념만 가져오고, 구현은 현재 레포의 CPU 스키닝 경로 안에 넣는다.
- 범위는 `Set_Animation*`, `Set_AnimationSequence`, `Request_AnimEnd`, `Preview_StateAnimation` 전부 공통 cross-fade를 타게 만드는 것.
- 아래 코드는 “설계 설명”이 아니라, 실제로 각 파일에 넣을 기준안이다. 전체 파일 교체가 아니라 “변경/추가되는 선언과 함수 전체” 기준으로 적는다.

## Code

### [Engine/Public/Engine_Struct.h](c:/Users/moon/Desktop/Jusin/GitDesktop/Dx11_Naruto/Engine/Public/Engine_Struct.h)
`FAnimationClipSetting` 아래에 이 pose 타입을 추가한다.

```cpp
    struct FAnimationLocalPose
    {
        Vec3    scale = Vec3(1.f, 1.f, 1.f);
        Quat    rotation = Quat::Identity;
        Vec3    translation = Vec3::Zero;

        // 해당 bone channel이 현재 클립에 실제로 존재하는지 표시한다.
        bool    valid = false;
    };
```

### [Engine/Public/Channel.h](c:/Users/moon/Desktop/Jusin/GitDesktop/Dx11_Naruto/Engine/Public/Channel.h)
기존 선언을 아래처럼 확장한다.

```cpp
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

    // 기존 경로 호환용이다. 내부적으로는 Sample_LocalPose를 사용한다.
    void    Update_TransformationMatrix(float trackPosition, vector<Shared<Bone>>& bones);

    // 현재 트랙 위치에서 이 채널의 로컬 포즈를 샘플링한다.
    void    Sample_LocalPose(float trackPosition, FAnimationLocalPose& outPose) const;

    int32   Get_BoneIndex() const { return _boneIndex; }

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
```

### [Engine/Private/Channel.cpp](c:/Users/moon/Desktop/Jusin/GitDesktop/Dx11_Naruto/Engine/Private/Channel.cpp)
기존 `Update_TransformationMatrix`를 포함해서 아래처럼 정리한다.

```cpp
#include "pch.h"
#include "Channel.h"

#include "Bone.h"

Channel::Channel()
{
}

Channel::Channel(const Channel& rhs)
    : _nodeName(rhs._nodeName)
    , _boneIndex(rhs._boneIndex)
    , _keyFrames(rhs._keyFrames)
    , _currentKeyFrameIndex(0)
{
}

HRESULT Channel::Initialize(const FAnimationChannelRaw& src)
{
    _nodeName = src.nodeName;
    _boneIndex = src.boneIndex;
    _currentKeyFrameIndex = 0;

    _keyFrames.clear();
    _keyFrames.reserve(src.keyFrames.size());

    for (const auto& raw : src.keyFrames)
    {
        FKeyFrame key{};
        key.time = raw.time;
        key.scale = raw.scale;
        key.rotation = raw.rotation;
        key.translation = raw.translation;

        _keyFrames.push_back(key);
    }

    return S_OK;
}

void Channel::Reset()
{
    _currentKeyFrameIndex = 0;
}

void Channel::Update_TransformationMatrix(float trackPosition, vector<Shared<Bone>>& bones)
{
    if (_boneIndex < 0 || _boneIndex >= static_cast<int32>(bones.size()))
        return;

    FAnimationLocalPose sampledPose{};
    Sample_LocalPose(trackPosition, sampledPose);

    if (!sampledPose.valid)
        return;

    Matrix local = Matrix::CreateScale(sampledPose.scale)
        * Matrix::CreateFromQuaternion(sampledPose.rotation)
        * Matrix::CreateTranslation(sampledPose.translation);

    bones[_boneIndex]->Set_LocalTransform(local);
}

void Channel::Sample_LocalPose(float trackPosition, FAnimationLocalPose& outPose) const
{
    outPose = {};

    if (_keyFrames.empty())
        return;

    if (_keyFrames.size() == 1)
    {
        outPose.scale = _keyFrames[0].scale;
        outPose.rotation = _keyFrames[0].rotation;
        outPose.translation = _keyFrames[0].translation;
        outPose.valid = true;
        return;
    }

    uint32 keyIndex = 0;
    while (keyIndex + 1 < _keyFrames.size() &&
            trackPosition >= _keyFrames[keyIndex + 1].time)
    {
        ++keyIndex;
    }

    if (keyIndex + 1 >= _keyFrames.size())
    {
        const FKeyFrame& last = _keyFrames.back();

        outPose.scale = last.scale;
        outPose.rotation = last.rotation;
        outPose.translation = last.translation;
        outPose.valid = true;
        return;
    }

    const FKeyFrame& cur = _keyFrames[keyIndex];
    const FKeyFrame& next = _keyFrames[keyIndex + 1];

    const float delta = next.time - cur.time;
    const float ratio = (delta <= FLT_EPSILON) ? 0.f : (trackPosition - cur.time) / delta;

    outPose.scale = Vec3::Lerp(cur.scale, next.scale, ratio);
    outPose.rotation = Quat::Slerp(cur.rotation, next.rotation, ratio);
    outPose.translation = Vec3::Lerp(cur.translation, next.translation, ratio);
    outPose.valid = true;
}

void Channel::Apply_KeyFrame(const FKeyFrame& key, const Shared<Bone>& bone)
{
    Matrix local = Matrix::CreateScale(key.scale)
        * Matrix::CreateFromQuaternion(key.rotation)
        * Matrix::CreateTranslation(key.translation);

    bone->Set_LocalTransform(local);
}

Shared<Channel> Channel::Create(const FAnimationChannelRaw& src)
{
    Shared<Channel> instance = make_shared<Channel>();

    if (FAILED(instance->Initialize(src)))
    {
        LOG_ERROR("Failed to Create : Channel");
        return nullptr;
    }

    return instance;
}

Shared<Channel> Channel::Clone() const
{
    return make_shared<Channel>(*this);
}

void Channel::Free()
{
    Base::Free();
}
```

### [Engine/Public/Animation.h](c:/Users/moon/Desktop/Jusin/GitDesktop/Dx11_Naruto/Engine/Public/Animation.h)
기존 `Update_TransformationMatrices`는 살리고, 샘플링 기반 API를 추가한다.

```cpp
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

    // 트랙 위치만 진행하고 종료 여부를 반환한다.
    bool    Advance_TrackPosition(float timeDelta, bool isLoop, float& inOutTrackPosition) const;

    // 현재 트랙 위치의 로컬 포즈들을 샘플링한다.
    void    Sample_LocalPoses(float trackPosition, vector<FAnimationLocalPose>& inOutPoses) const;

    const string& Get_Name() const { return _name; }

private:
    string  _name;
    float   _duration = 0.f;
    float   _ticksPersecond = 25.f;
    float   _currentTrackPosition = 0.f;

    vector<Shared<Channel>> _channels;

public:
    static Shared<Animation> Create(const FAnimationClipRaw& src);
    Shared<Animation>        Clone();
    void Free() override;
};

NS_END
```

### [Engine/Private/Animation.cpp](c:/Users/moon/Desktop/Jusin/GitDesktop/Dx11_Naruto/Engine/Private/Animation.cpp)
샘플링/트랙 진행 API를 추가하고, 기존 함수는 호환 래퍼로 정리한다.

```cpp
#include "pch.h"
#include "Animation.h"

#include "Channel.h"
#include "Bone.h"

Animation::Animation()
{
}

Animation::Animation(const Animation& rhs)
    : _name(rhs._name)
    , _duration(rhs._duration)
    , _ticksPersecond(rhs._ticksPersecond)
    , _currentTrackPosition(0.f)
{
    _channels.reserve(rhs._channels.size());

    for (const auto& channel : rhs._channels)
    {
        if (channel)
            _channels.push_back(channel->Clone());
        else
            _channels.push_back(nullptr);
    }
}

HRESULT Animation::Initialize(const FAnimationClipRaw& src)
{
    _name                   = src.name;
    _duration               = src.duration;
    _ticksPersecond         = src.ticksPerSecond;
    _currentTrackPosition   = 0.f;

    _channels.clear();
    _channels.reserve(src.channels.size());

    for (const auto& channelRaw : src.channels)
    {
        Shared<Channel> channel = Channel::Create(channelRaw);
        CHECK_NULL(channel, E_FAIL);
        _channels.push_back(channel);
    }

    return S_OK;
}

void Animation::Reset()
{
    _currentTrackPosition = 0.f;

    for (auto& channel : _channels)
    {
        channel->Reset();
    }
}

bool Animation::Advance_TrackPosition(float timeDelta, bool isLoop, float& inOutTrackPosition) const
{
    if (_duration <= FLT_EPSILON)
        return true;

    inOutTrackPosition += _ticksPersecond * timeDelta;

    bool finished = false;

    if (inOutTrackPosition >= _duration)
    {
        finished = true;

        if (isLoop)
        {
            inOutTrackPosition = fmod(inOutTrackPosition, _duration);
        }
        else
        {
            inOutTrackPosition = _duration;
        }
    }

    return finished;
}

void Animation::Sample_LocalPoses(float trackPosition, vector<FAnimationLocalPose>& inOutPoses) const
{
    for (auto& pose : inOutPoses)
    {
        pose = {};
    }

    for (const auto& channel : _channels)
    {
        if (!channel)
            continue;

        const int32 boneIndex = channel->Get_BoneIndex();
        if (boneIndex < 0 || boneIndex >= static_cast<int32>(inOutPoses.size()))
            continue;

        channel->Sample_LocalPose(trackPosition, inOutPoses[boneIndex]);
    }
}

bool Animation::Update_TransformationMatrices(float timeDelta, vector<Shared<Bone>>& bones, bool isLoop)
{
    const bool finished = Advance_TrackPosition(timeDelta, isLoop, _currentTrackPosition);

    vector<FAnimationLocalPose> sampledPoses;
    sampledPoses.resize(bones.size());

    Sample_LocalPoses(_currentTrackPosition, sampledPoses);

    for (size_t i = 0; i < bones.size(); ++i)
    {
        if (!bones[i])
            continue;

        if (!sampledPoses[i].valid)
        {
            bones[i]->Reset_ToNodeTransform();
            continue;
        }

        Matrix local = Matrix::CreateScale(sampledPoses[i].scale)
            * Matrix::CreateFromQuaternion(sampledPoses[i].rotation)
            * Matrix::CreateTranslation(sampledPoses[i].translation);

        bones[i]->Set_LocalTransform(local);
    }

    return finished;
}

void Animation::Free()
{
    Base::Free();
}

Shared<Animation> Animation::Create(const FAnimationClipRaw& src)
{
    Shared<Animation> instance = make_shared<Animation>();

    if (FAILED(instance->Initialize(src)))
    {
        LOG_ERROR("Failed to Create : Animation");
        return nullptr;
    }

    return instance;
}

Shared<Animation> Animation::Clone()
{
    return make_shared<Animation>(*this);
}
```

### [Engine/Public/Model.h](c:/Users/moon/Desktop/Jusin/GitDesktop/Dx11_Naruto/Engine/Public/Model.h)
`Model` 안에 runtime clip/blend 상태를 추가한다.

```cpp
#pragma once

#include "Component.h"

NS_BEGIN(Engine)

class Mesh;
class ModelMaterial;
class Shader;
class Bone;
class Animation;

class ENGINE_DLL Model : public Component
{
    GENERATED_COMPONENT(Model, Protocol::COMPONENT_TYPE_MODEL)

public:
    explicit Model(ComPtr<Device> device, ComPtr<DeviceContext> context);
    explicit Model(const Model& rhs);
    virtual ~Model() = default;
    
public:
    virtual HRESULT Initialize_Prototype(EMeshVertexType type, const string& modelFilePath, const Matrix& preLocalTransformMatrix);
    virtual HRESULT Initialize(void* arg) override;
    HRESULT         Render(uint32 meshIndex);

    HRESULT         Bind_Material(Shared<Shader> shader, const char* constantName,
                                    uint32 meshIndex, EMaterialTextureSlot slot,
                                    uint32 textureIndex);

    size_t          Get_NumMeshes() const { return _meshes.size(); }

public:
    void    Set_Animation(uint32 animIndex, bool isLoop);
    void    Set_Animation(const string& animName, bool isLoop);
    void    Set_Animation(const FAnimationClipSetting& clip);

    void    Set_AnimationSequence(
        const FAnimationClipSetting& startClip,
        const FAnimationClipSetting& loopClip,
        const FAnimationClipSetting& endClip);

    void            Request_AnimEnd();
    EAnimPhase      Get_AnimPhase() const { return _animPhase; }

    uint32          Get_AnimationCount() const;
    const string&   Get_AnimationName(uint32 index) const;
    int32           Find_AnimationIndex_ByName(const string& animName);

    bool            Play_Animation(float timeDelta);
    HRESULT         Bind_BoneMatrices(Shared<Shader> shader, const char* constantName);

    bool            Has_Animations() const { return !_animations.empty(); }
    bool            Has_ActiveAnimation() const
    {
        return _currentAnimationIndex >= 0 &&
            _currentAnimationIndex < static_cast<int32>(_animations.size());
    }

public:
    json    To_Json() const override;
    void    From_Json(const json& data) override;

    size_t  Get_NumMaterials() const { return _materials.size(); }
    Shared<ModelMaterial> Get_Material(uint32 index) const;

    uint32  Get_MeshMaterialIndex(uint32 index) const;
    string  Get_MeshName(uint32 index);

    void    Set_AnimationPlayRate(float playRate);
    float   Get_AnimationPlayRate() const { return _animationPlayRate; }

    void    Set_AnimationBlendDuration(float seconds) { _animationBlendDuration = Utils::Max(seconds, 0.01f); }
    float   Get_AnimationBlendDuration() const { return _animationBlendDuration; }

    bool    Is_AnimationSequenceFinished() const { return _isAnimSequenceFinished; }

private:
    struct FPlayingClipState
    {
        int32   animIndex = -1;
        bool    loop = false;
        float   playRate = 1.f;
        float   trackPosition = 0.f;

        bool Is_Valid() const { return animIndex >= 0; }
    };

    struct FAnimationBlendState
    {
        bool                        active = false;
        float                       duration = 0.15f;
        float                       elapsed = 0.f;
        FPlayingClipState           next;
        vector<FAnimationLocalPose> fromPose;
        vector<FAnimationLocalPose> toPose;
    };

private:
    HRESULT Initialize_FromMeshBin(const string& modelFilePath);
    HRESULT Ready_FromBinary(const string& modelFilePath);

    HRESULT Ready_StaticMeshes(const FModelBinaryData& data);
    HRESULT Ready_SkeletalMeshes(const FModelBinaryData& data);

    HRESULT Ready_Bones(const FModelBinaryData& data);
    HRESULT Ready_Animations(const FModelBinaryData& data);

    HRESULT Ready_Materials_FromJson(const string& materialFilePath);

    void    Apply_MaterialOverrides(const json& data);
    string  Build_MaterialJsonPath(const string& modelFilePath) const;

private:
    bool    Try_BeginAnimationTransition(const FAnimationClipSetting& clip);
    void    Begin_ImmediateClip(const FPlayingClipState& clipState);
    void    Clear_BlendState();

    void    Capture_CurrentPose(vector<FAnimationLocalPose>& outPose) const;
    bool    Sample_ClipPose(const FPlayingClipState& clipState, vector<FAnimationLocalPose>& outPose) const;
    void    Apply_LocalPoses_ToBones(const vector<FAnimationLocalPose>& poses);
    void    Update_BoneMatrices_FromBones();
    void    Sync_LegacyAnimationState();

    static void Blend_LocalPoses(
        const vector<FAnimationLocalPose>& fromPose,
        const vector<FAnimationLocalPose>& toPose,
        float blendRatio,
        vector<FAnimationLocalPose>& outPose);

    void    Reset_AnimationSequenceState();
    void    Complete_AnimationSequence();

private:
    EMeshVertexType                 _modelType = { EMeshVertexType::END };
    Matrix                          _preLocalTransformMatrix = {};

private:
    uint32                          _numMeshes = {};
    vector<Shared<Mesh>>            _meshes;

    uint32                          _numMaterials = {};
    vector<Shared<ModelMaterial>>   _materials;

    vector<Shared<Bone>>            _bones;
    vector<Shared<Animation>>       _animations;
    vector<Matrix>                  _boneMatrices;

    // 기존 public/legacy 질의 호환용이다.
    int32                           _currentAnimationIndex = -1;
    bool                            _isAnimationLoop = false;

    string                          _modelGuid = "";

private: /* 애니메이션 재생 관련 */
    float                           _animationPlayRate = 1.f;
    float                           _animationBlendDuration = 0.15f;

    EAnimPhase                      _animPhase = EAnimPhase::Start;

    FAnimationClipSetting           _startClip;
    FAnimationClipSetting           _loopClip;
    FAnimationClipSetting           _endClip;

    bool                            _hasAnimSequence = false;
    bool                            _isAnimSequenceFinished = false;

    FPlayingClipState               _currentClip;
    FAnimationBlendState            _blendState;

    vector<FAnimationLocalPose>     _currentSamplePose;
    vector<FAnimationLocalPose>     _nextSamplePose;
    vector<FAnimationLocalPose>     _blendedPose;
    vector<FAnimationLocalPose>     _lastAppliedPose;

public:
    static Shared<Model> Create(ComPtr<Device> device, ComPtr<DeviceContext> context,
           EMeshVertexType type, const string& modelFilePath, const Matrix& preLocalTransformMatrix);

    virtual Shared<Component> Clone(void* arg) override;
    virtual void Free() override;
};

NS_END
```

### [Engine/Private/Model.cpp](c:/Users/moon/Desktop/Jusin/GitDesktop/Dx11_Naruto/Engine/Private/Model.cpp)
아래 함수들을 기존 구현 대신 교체/추가한다.

```cpp
Model::Model(ComPtr<Device> device, ComPtr<DeviceContext> context)
    : Component(device, context)
{
    _blendState.duration = _animationBlendDuration;
}

Model::Model(const Model& rhs)
    : Component(rhs)
    , _modelType(rhs._modelType)
    , _preLocalTransformMatrix(rhs._preLocalTransformMatrix)
    , _numMeshes(rhs._numMeshes)
    , _meshes(rhs._meshes)
    , _numMaterials(rhs._numMaterials)
    , _materials(rhs._materials)
    , _currentAnimationIndex(-1)
    , _isAnimationLoop(false)
    , _modelGuid(rhs._modelGuid)
    , _animationPlayRate(1.f)
    , _animationBlendDuration(rhs._animationBlendDuration)
    , _animPhase(EAnimPhase::Start)
    , _startClip{}
    , _loopClip{}
    , _endClip{}
    , _hasAnimSequence(false)
    , _isAnimSequenceFinished(false)
    , _currentClip{}
    , _blendState{}
{
    _blendState.duration = _animationBlendDuration;

    _bones.reserve(rhs._bones.size());
    for (const auto& bone : rhs._bones)
    {
        if (bone)
            _bones.push_back(bone->Clone());
        else
            _bones.push_back(nullptr);
    }

    _animations.reserve(rhs._animations.size());
    for (const auto& animation : rhs._animations)
    {
        if (animation)
            _animations.push_back(animation->Clone());
        else
            _animations.push_back(nullptr);
    }

    _boneMatrices.assign(_bones.size(), Matrix::Identity);
}

void Model::Set_Animation(uint32 animIndex, bool isLoop)
{
    if (animIndex >= _animations.size())
        return;

    FAnimationClipSetting clip;
    clip.animationName = Get_AnimationName(animIndex);
    clip.loop = isLoop;
    clip.playRate = 1.f;

    Set_Animation(clip);
}

void Model::Set_Animation(const string& animName, bool isLoop)
{
    FAnimationClipSetting clip;
    clip.animationName = animName;
    clip.loop = isLoop;
    clip.playRate = 1.f;

    Set_Animation(clip);
}

void Model::Set_Animation(const FAnimationClipSetting& clip)
{
    Reset_AnimationSequenceState();
    Try_BeginAnimationTransition(clip);
}

void Model::Set_AnimationSequence(
    const FAnimationClipSetting& startClip,
    const FAnimationClipSetting& loopClip,
    const FAnimationClipSetting& endClip)
{
    _startClip = startClip;
    _loopClip = loopClip;
    _endClip = endClip;

    _hasAnimSequence = true;
    _isAnimSequenceFinished = false;

    if (!_startClip.animationName.empty())
    {
        _animPhase = EAnimPhase::Start;
        Try_BeginAnimationTransition(_startClip);
        return;
    }

    _animPhase = EAnimPhase::Loop;
    Try_BeginAnimationTransition(_loopClip);
}

void Model::Request_AnimEnd()
{
    if (!_hasAnimSequence || _isAnimSequenceFinished)
        return;

    if (_animPhase == EAnimPhase::End)
        return;

    if (_endClip.animationName.empty())
    {
        Complete_AnimationSequence();
        return;
    }

    _animPhase = EAnimPhase::End;

    if (!Try_BeginAnimationTransition(_endClip))
    {
        Complete_AnimationSequence();
    }
}

bool Model::Play_Animation(float timeDelta)
{
    if (!_currentClip.Is_Valid())
    {
        // 시퀀스 종료 직후 마지막 포즈를 유지하기 위해, 마지막 적용 포즈가 있으면 그대로 다시 반영한다.
        if (!_lastAppliedPose.empty())
        {
            Apply_LocalPoses_ToBones(_lastAppliedPose);
            Update_BoneMatrices_FromBones();
        }

        return false;
    }

    for (auto& bone : _bones)
    {
        if (bone)
            bone->Reset_ToNodeTransform();
    }

    bool currentFinished = false;
    if (_currentClip.animIndex >= 0 &&
        _currentClip.animIndex < static_cast<int32>(_animations.size()) &&
        _animations[_currentClip.animIndex] != nullptr)
    {
        currentFinished = _animations[_currentClip.animIndex]->Advance_TrackPosition(
            timeDelta * _currentClip.playRate,
            _currentClip.loop,
            _currentClip.trackPosition);

        Sample_ClipPose(_currentClip, _currentSamplePose);
    }

    if (_blendState.active)
    {
        if (_blendState.next.animIndex >= 0 &&
            _blendState.next.animIndex < static_cast<int32>(_animations.size()) &&
            _animations[_blendState.next.animIndex] != nullptr)
        {
            _animations[_blendState.next.animIndex]->Advance_TrackPosition(
                timeDelta * _blendState.next.playRate,
                _blendState.next.loop,
                _blendState.next.trackPosition);

            Sample_ClipPose(_blendState.next, _nextSamplePose);
        }

        _blendState.elapsed += timeDelta;

        const float blendRatio = (_blendState.duration <= FLT_EPSILON)
            ? 1.f
            : Utils::Min(_blendState.elapsed / _blendState.duration, 1.f);

        Blend_LocalPoses(_blendState.fromPose, _nextSamplePose, blendRatio, _blendedPose);
        Apply_LocalPoses_ToBones(_blendedPose);
        Update_BoneMatrices_FromBones();

        _lastAppliedPose = _blendedPose;

        if (blendRatio >= 1.f)
        {
            _currentClip = _blendState.next;
            Clear_BlendState();
            Sync_LegacyAnimationState();
        }

        return false;
    }

    Apply_LocalPoses_ToBones(_currentSamplePose);
    Update_BoneMatrices_FromBones();

    _lastAppliedPose = _currentSamplePose;
    Sync_LegacyAnimationState();

    if (!_hasAnimSequence)
    {
        return currentFinished;
    }

    if (!currentFinished)
        return false;

    if (_animPhase == EAnimPhase::Start)
    {
        _animPhase = EAnimPhase::Loop;

        if (!Try_BeginAnimationTransition(_loopClip))
        {
            Complete_AnimationSequence();
            return true;
        }

        return false;
    }

    if (_animPhase == EAnimPhase::End)
    {
        Complete_AnimationSequence();
        return true;
    }

    return false;
}

bool Model::Try_BeginAnimationTransition(const FAnimationClipSetting& clip)
{
    if (clip.animationName.empty())
        return false;

    uint32 animIndex = 0;
    if (!Find_AnimationIndex(clip.animationName, animIndex))
        return false;

    FPlayingClipState targetClip{};
    targetClip.animIndex = static_cast<int32>(animIndex);
    targetClip.loop = clip.loop;
    targetClip.playRate = Utils::Max(clip.playRate, 0.01f);
    targetClip.trackPosition = 0.f;

    _isAnimSequenceFinished = false;

    if (!_currentClip.Is_Valid())
    {
        Begin_ImmediateClip(targetClip);
        return true;
    }

    Capture_CurrentPose(_blendState.fromPose);
    Sample_ClipPose(targetClip, _blendState.toPose);

    _blendState.active = true;
    _blendState.duration = _animationBlendDuration;
    _blendState.elapsed = 0.f;
    _blendState.next = targetClip;

    // 블렌드 중에는 target clip의 진행 결과를 실시간으로 샘플링한다.
    _nextSamplePose = _blendState.toPose;

    return true;
}

void Model::Begin_ImmediateClip(const FPlayingClipState& clipState)
{
    _currentClip = clipState;
    Clear_BlendState();
    Sync_LegacyAnimationState();
}

void Model::Clear_BlendState()
{
    _blendState.active = false;
    _blendState.elapsed = 0.f;
    _blendState.duration = _animationBlendDuration;
    _blendState.next = {};

    _blendState.fromPose.clear();
    _blendState.toPose.clear();
}

void Model::Capture_CurrentPose(vector<FAnimationLocalPose>& outPose) const
{
    outPose.clear();
    outPose.resize(_bones.size());

    if (_lastAppliedPose.size() == _bones.size())
    {
        outPose = _lastAppliedPose;
        return;
    }

    if (_currentClip.Is_Valid())
    {
        Sample_ClipPose(_currentClip, outPose);
    }
}

bool Model::Sample_ClipPose(const FPlayingClipState& clipState, vector<FAnimationLocalPose>& outPose) const
{
    outPose.clear();
    outPose.resize(_bones.size());

    if (!clipState.Is_Valid())
        return false;

    if (clipState.animIndex < 0 || clipState.animIndex >= static_cast<int32>(_animations.size()))
        return false;

    const auto& animation = _animations[clipState.animIndex];
    if (!animation)
        return false;

    animation->Sample_LocalPoses(clipState.trackPosition, outPose);
    return true;
}

void Model::Apply_LocalPoses_ToBones(const vector<FAnimationLocalPose>& poses)
{
    for (size_t i = 0; i < _bones.size(); ++i)
    {
        if (!_bones[i])
            continue;

        if (i >= poses.size() || !poses[i].valid)
        {
            _bones[i]->Reset_ToNodeTransform();
            continue;
        }

        Matrix local = Matrix::CreateScale(poses[i].scale)
            * Matrix::CreateFromQuaternion(poses[i].rotation)
            * Matrix::CreateTranslation(poses[i].translation);

        _bones[i]->Set_LocalTransform(local);
    }
}

void Model::Update_BoneMatrices_FromBones()
{
    for (size_t i = 0; i < _bones.size(); ++i)
    {
        if (!_bones[i])
            continue;

        const int32 parentIndex = _bones[i]->Get_ParentIndex();
        const Matrix* parentMatrix = (parentIndex >= 0)
            ? &_bones[parentIndex]->Get_CombinedTransform()
            : nullptr;

        _bones[i]->Update_Combined(parentMatrix, _preLocalTransformMatrix);
        _boneMatrices[i] = _bones[i]->Get_SkinningMatrix();
    }
}

void Model::Sync_LegacyAnimationState()
{
    _currentAnimationIndex = _currentClip.animIndex;
    _isAnimationLoop = _currentClip.loop;
}

void Model::Blend_LocalPoses(
    const vector<FAnimationLocalPose>& fromPose,
    const vector<FAnimationLocalPose>& toPose,
    float blendRatio,
    vector<FAnimationLocalPose>& outPose)
{
    const size_t poseCount = Utils::Max(fromPose.size(), toPose.size());
    outPose.clear();
    outPose.resize(poseCount);

    for (size_t i = 0; i < poseCount; ++i)
    {
        const bool hasFrom = i < fromPose.size() && fromPose[i].valid;
        const bool hasTo = i < toPose.size() && toPose[i].valid;

        if (hasFrom && hasTo)
        {
            outPose[i].scale = Vec3::Lerp(fromPose[i].scale, toPose[i].scale, blendRatio);
            outPose[i].rotation = Quat::Slerp(fromPose[i].rotation, toPose[i].rotation, blendRatio);
            outPose[i].translation = Vec3::Lerp(fromPose[i].translation, toPose[i].translation, blendRatio);
            outPose[i].valid = true;
            continue;
        }

        if (hasTo)
        {
            outPose[i] = toPose[i];
            outPose[i].valid = true;
            continue;
        }

        if (hasFrom)
        {
            outPose[i] = fromPose[i];
            outPose[i].valid = true;
            continue;
        }

        outPose[i] = {};
    }
}

void Model::Reset_AnimationSequenceState()
{
    _hasAnimSequence = false;
    _isAnimSequenceFinished = false;
    _animPhase = EAnimPhase::Start;
    _startClip = {};
    _loopClip = {};
    _endClip = {};
}

void Model::Complete_AnimationSequence()
{
    _hasAnimSequence = false;
    _isAnimSequenceFinished = true;

    // 마지막 End 포즈를 유지해야 다음 상태 전환 때 자연스럽게 다시 블렌드할 수 있다.
    _startClip = {};
    _loopClip = {};
    _endClip = {};
}

void Model::Set_AnimationPlayRate(float playRate)
{
    _animationPlayRate = Utils::Max(playRate, 0.01f);

    if (_currentClip.Is_Valid())
    {
        _currentClip.playRate = _animationPlayRate;
        Sync_LegacyAnimationState();
    }

    if (_blendState.active && _blendState.next.Is_Valid())
    {
        _blendState.next.playRate = _animationPlayRate;
    }
}
```

### [Client/Public/Client_Struct.h](c:/Users/moon/Desktop/Jusin/GitDesktop/Dx11_Naruto/Client/Public/Client_Struct.h)
`FStateAnimationDesc`에서 공통 `playRate`를 제거한다.

```cpp
#pragma once

#include "Client_Enum.h"

NS_BEGIN(Engine)
struct FAnimationClipSetting;
NS_END

namespace Client
{
    struct FSkillData
    {
        uint32  skill_Id = 0;
        wstring skillName = L"";
        uint32   srvIndex = 0;
        float    coolDown = 0.f;
        int      manaCost = 0;
    };

    struct FLoadJob
    {
        ELoadJobType type = ELoadJobType::TextureCreate;

        uint32 componentID = 0;
        uint32 levelIndex = 0;
        uint32 prototypeLevelIndex = 0;
        uint32 objectType = 0;

        string idStr;
        string pathStr;
        string extraStr;

        uint32 count = 1;
        bool isSkeletal = false;

        FSkillData skillData{};
    };

    struct FStateAnimationDesc
    {
        EStateAnimationMode mode = EStateAnimationMode::Single;

        // 싱글용
        FAnimationClipSetting single;

        // 시퀀스용
        FAnimationClipSetting start;
        FAnimationClipSetting loop;
        FAnimationClipSetting end;

        bool Has_Sequence() const
        {
            return !loop.animationName.empty();
        }
    };
}
```

### [Client/Private/PlayerStateMachine.cpp](c:/Users/moon/Desktop/Jusin/GitDesktop/Dx11_Naruto/Client/Private/PlayerStateMachine.cpp)
`Preview_StateAnimation()`만 아래처럼 바꾼다.

```cpp
bool PlayerStateMachine::Preview_StateAnimation(EPlayerState stateID, int32 sequenceSlot)
{
    auto model = Get_Model();
    if (!model)
        return false;

    const auto* animDesc = Find_StateAnimation(stateID);
    if (!animDesc)
        return false;

    if (animDesc->mode == EStateAnimationMode::Sequence)
    {
        const FAnimationClipSetting* clip = nullptr;

        if (sequenceSlot == 0)
            clip = &animDesc->start;
        else if (sequenceSlot == 1)
            clip = &animDesc->loop;
        else
            clip = &animDesc->end;

        if (!clip || clip->animationName.empty())
            return false;

        model->Set_Animation(*clip);
        return true;
    }

    if (animDesc->single.animationName.empty())
        return false;

    model->Set_Animation(animDesc->single);

    return true;
}
```

## Notes
- `Model::Play_Animation()`은 이제 “current 샘플링 -> blend 시 next 샘플링 -> 최종 pose 적용” 순서로 고정한다.
- 블렌드 중 새 요청이 오면 이전 blend를 완주하지 않고, `_lastAppliedPose`를 기준으로 최신 요청으로 즉시 갈아탄다.
- `Complete_AnimationSequence()`에서 현재 클립을 비우지 않는 이유는, 마지막 End 포즈를 유지한 채 다음 상태로 자연스럽게 다시 블렌드하기 위해서다.
- `Set_AnimationPlayRate()`는 공통 override API로만 남겨둔다. FSM/Preview는 더 이상 `animDesc->playRate`를 쓰지 않는다.
- `Model` clone은 runtime blend 상태를 공유하면 안 되므로, 복사 생성자에서 clip/blend state는 초기화 상태로 다시 시작한다.

## Test Plan
- `Idle -> Run`, `Run -> Idle`, `Jump -> Idle`, `Jump -> Run` 전환 시 스냅이 사라져야 한다.
- `Run` sequence의 `Start -> Loop`, `Loop -> End`가 동일한 cross-fade 규칙으로 이어져야 한다.
- `RunEnd` 도중 재입력 시 `End -> Loop`가 끊기지 않고 새 blend로 갈아타야 한다.
- `Preview_StateAnimation()`에서 `Start / Loop / End / Single`을 연속 클릭해도 동일한 느낌으로 전환돼야 한다.
- sequence finished가 올라간 뒤에도 마지막 pose가 유지되고, 다음 상태 진입 시 그 pose에서 다시 blend가 시작돼야 한다.
- 모델 clone 여러 개가 동시에 재생돼도 서로 blend 상태가 섞이면 안 된다.

## Assumptions
- 이번 패스는 CPU 보간만 구현한다.
- matrix direct lerp는 하지 않고 SRT 기준 보간만 사용한다.
- 기본 blend duration은 `0.15f`다.
- 루트모션은 이번 범위 밖이다. 대신 `trackPosition`을 clip state에 유지해 다음 패스 확장 포인트만 남긴다.
