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

    if (_keyFrames.empty())
        return;

    if (_keyFrames.size() == 1)
    {
        Apply_KeyFrame(_keyFrames[0], bones[_boneIndex]);
        return;
    }

    while (_currentKeyFrameIndex + 1 < _keyFrames.size() &&
            trackPosition >= _keyFrames[_currentKeyFrameIndex + 1].time)
    {
        ++_currentKeyFrameIndex;
    }

    if (_currentKeyFrameIndex + 1 >= _keyFrames.size())
    {
        Apply_KeyFrame(_keyFrames.back(), bones[_boneIndex]);
        return;
    }

    const FKeyFrame& cur = _keyFrames[_currentKeyFrameIndex];
    const FKeyFrame& next = _keyFrames[_currentKeyFrameIndex + 1];

    const float delta = next.time - cur.time;
    const float ratio = (delta <= FLT_EPSILON) ? 0.f : (trackPosition - cur.time) / delta;

    Vec3 scale = Vec3::Lerp(cur.scale, next.scale, ratio);
    Quat rotation = Quat::Slerp(cur.rotation, next.rotation, ratio);
    Vec3 translation = Vec3::Lerp(cur.translation, next.translation, ratio);

    Matrix local = Matrix::CreateScale(scale)
        * Matrix::CreateFromQuaternion(rotation)
        * Matrix::CreateTranslation(translation);

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
