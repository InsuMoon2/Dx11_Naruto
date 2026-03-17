#include "pch.h"
#include "Animation.h"

#include "Channel.h"

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

bool Animation::Update_TransformationMatrices(float timeDelta, vector<Shared<Bone>>& bones, bool isLoop)
{
    _currentTrackPosition += _ticksPersecond * timeDelta;

    bool finished = false;

    if (_currentTrackPosition >= _duration)
    {
        finished = true;

        if (isLoop && _duration > 0.f)
        {
            _currentTrackPosition = fmod(_currentTrackPosition, _duration);

            for (auto& channel : _channels)
                channel->Reset();
        }
        else
        {
            _currentTrackPosition = _duration; // 루프가 아닐 때 끝났으면 duration
        }
    }

    for (auto& channel : _channels)
        channel->Update_TransformationMatrix(_currentTrackPosition, bones);

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
