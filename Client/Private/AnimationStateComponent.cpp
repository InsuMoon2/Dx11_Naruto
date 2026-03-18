#include "pch.h"
#include "AnimationStateComponent.h"
#include "GameObject.h"
#include "Model.h"

AnimationStateComponent::AnimationStateComponent(ComPtr<Device> device, ComPtr<DeviceContext> context)
    : Component(device, context)
{
}

AnimationStateComponent::AnimationStateComponent(const AnimationStateComponent& rhs)
    : Component(rhs)
    , _stateAnimations(rhs._stateAnimations)
    , _currentStateName("")
    , _prevStateName("")
{
}

HRESULT AnimationStateComponent::Initialize_Prototype()
{
    return Component::Initialize_Prototype();
}

HRESULT AnimationStateComponent::Initialize(void* arg)
{
    return Component::Initialize(arg);
}

void AnimationStateComponent::BeginPlay()
{
    Component::BeginPlay();

    _model = Resolve_Model();
}

Shared<Model> AnimationStateComponent::Resolve_Model()
{
    auto owner = Get_Owner();
    if (!owner)
        return nullptr;

    return owner->Get_Component<Model>();
}

bool AnimationStateComponent::Play_State(const string& stateKey)
{
    if (!_model)
        _model = Resolve_Model();

    if (!_model || stateKey.empty())
        return false;

    const auto* animDesc = Find_State(stateKey);
    if (!animDesc)
        return false;

    _prevStateName = _currentStateName;
    _currentStateName = stateKey;

    if (animDesc->mode == EStateAnimationMode::Sequence)
    {
        if (!animDesc->Has_Sequence())
            return false;

        _model->Set_AnimationSequence(animDesc->start, animDesc->loop, animDesc->end);
        return true;
    }

    if (animDesc->mode == EStateAnimationMode::DirectionalSingle)
        return false;

    if (animDesc->single.animationName.empty())
        return false;

    _model->Set_Animation(animDesc->single);
    return true;
}

bool AnimationStateComponent::Play_DirectionalState(const string& stateName, EMoveInputDirection dir)
{
    if (!_model)
        _model = Resolve_Model();

    if (!_model || stateName.empty())
        return false;

    const auto* animDesc = Find_State(stateName);
    if (!animDesc)
        return false;

    if (animDesc->mode != EStateAnimationMode::DirectionalSingle)
        return false;

    const FAnimationClipSetting* clip = animDesc->directional.Find(dir);
    if (!clip || clip->animationName.empty())
        return false;

    _prevStateName = _currentStateName;
    _currentStateName = stateName;

    _model->Set_Animation(*clip);
    return true;
}

bool AnimationStateComponent::Play_StateLoopOnly(const string& stateName)
{
    if (!_model)
        _model = Resolve_Model();

    if (!_model || stateName.empty())
        return false;

    const auto* desc = Find_State(stateName);
    if (!desc)
        return false;

    if (desc->mode != EStateAnimationMode::Sequence)
        return Play_State(stateName);

    FAnimationClipSetting emptyStart = desc->start;
    emptyStart.animationName.clear();

    _prevStateName = _currentStateName;
    _currentStateName = stateName;

    _model->Set_AnimationSequence(emptyStart, desc->loop, desc->end);
    return true;
}

void AnimationStateComponent::Request_StateEnd()
{
    if (!_model)
        _model = Resolve_Model();

    if (_model)
        _model->Request_AnimEnd();
}

bool AnimationStateComponent::Is_CurrentStateFinished() const
{
    return _model ? _model->Is_CurrentAnimationFinished() : false;
}

bool AnimationStateComponent::Is_CurrentStateSequenceFinished() const
{
    return _model ? _model->Is_AnimationSequenceFinished() : false;
}

EAnimPhase AnimationStateComponent::Get_CurrentAnimPhase() const
{
    return _model ? _model->Get_AnimPhase() : EAnimPhase::Start;
}

float AnimationStateComponent::Get_CurrentTrackPosition() const
{
    return _model ? _model->Get_CurrentTrackPosition() : 0.f;
}

float AnimationStateComponent::Get_CurrentAnimationDuration() const
{
    return _model ? _model->Get_CurrentAnimationDuration() : 0.f;
}

const FStateAnimationDesc* AnimationStateComponent::Find_State(const string& stateName) const
{
    auto iter = _stateAnimations.find(stateName);
    if (iter == _stateAnimations.end())
        return nullptr;

    return &iter->second;
}

FStateAnimationDesc& AnimationStateComponent::Edit_State(const string& stateName)
{
    return _stateAnimations[stateName];
}

vector<string> AnimationStateComponent::Get_StateNames() const
{
    vector<string> result;
    result.reserve(_stateAnimations.size());

    for (const auto& [key, value] : _stateAnimations)
    {
        result.push_back(key);
    }

    sort(result.begin(), result.end());
    return result;
}

vector<string> AnimationStateComponent::Get_ModelAnimationNames() const
{
    vector<string> result;

    if (!_model)
        return result;

    const uint32 count = _model->Get_AnimationCount();
    result.reserve(count);

    for (uint32 i = 0; i < count; ++i)
    {
        const string& name = _model->Get_AnimationName(i);
        if (!name.empty())
            result.push_back(name);
    }

    return result;
}

bool AnimationStateComponent::Preview_State(const string& stateName, int32 slotIndex, EMoveInputDirection dir)
{
    if (!_model)
        _model = Resolve_Model();

    if (!_model)
        return false;

    const auto* animDesc = Find_State(stateName);
    if (!animDesc)
        return false;

    if (animDesc->mode == EStateAnimationMode::Sequence)
    {
        const FAnimationClipSetting* clip = nullptr;

        if (slotIndex == 0)
            clip = &animDesc->start;
        else if (slotIndex == 1)
            clip = &animDesc->loop;
        else
            clip = &animDesc->end;

        if (!clip || clip->animationName.empty())
            return false;

        _model->Set_Animation(*clip);
        return true;
    }

    if (animDesc->mode == EStateAnimationMode::DirectionalSingle)
    {
        const FAnimationClipSetting* clip = nullptr;

        if (slotIndex == 0)
            clip = &animDesc->directional.forward;
        else if (slotIndex == 1)
            clip = &animDesc->directional.backward;
        else if (slotIndex == 2)
            clip = &animDesc->directional.left;
        else
            clip = &animDesc->directional.right;

        if (!clip || clip->animationName.empty())
            return false;

        _model->Set_Animation(*clip);
        return true;
    }

    if (animDesc->single.animationName.empty())
        return false;

    _model->Set_Animation(animDesc->single);
    return true;
}

json AnimationStateComponent::To_Json() const
{
    json root = Component::To_Json();
    root["current_state"] = _currentStateName;

    json stateArray = json::array();

    for (const auto& [stateKey, desc] : _stateAnimations)
    {
        json item;
        item["key"] = stateKey;
        item["mode"] = string(magic_enum::enum_name(desc.mode));

        item["single"] = {
            { "animationName", desc.single.animationName },
            { "loop", desc.single.loop },
            { "playRate", desc.single.playRate }
        };

        item["start"] = {
            { "animationName", desc.start.animationName },
            { "loop", desc.start.loop },
            { "playRate", desc.start.playRate }
        };

        item["loopClip"] = {
            { "animationName", desc.loop.animationName },
            { "loop", desc.loop.loop },
            { "playRate", desc.loop.playRate }
        };

        item["end"] = {
            { "animationName", desc.end.animationName },
            { "loop", desc.end.loop },
            { "playRate", desc.end.playRate }
        };

        item["directional"] = {
            { "forward", {
                { "animationName", desc.directional.forward.animationName },
                { "loop", desc.directional.forward.loop },
                { "playRate", desc.directional.forward.playRate }
            } },
            { "backward", {
                { "animationName", desc.directional.backward.animationName },
                { "loop", desc.directional.backward.loop },
                { "playRate", desc.directional.backward.playRate }
            } },
            { "left", {
                { "animationName", desc.directional.left.animationName },
                { "loop", desc.directional.left.loop },
                { "playRate", desc.directional.left.playRate }
            } },
            { "right", {
                { "animationName", desc.directional.right.animationName },
                { "loop", desc.directional.right.loop },
                { "playRate", desc.directional.right.playRate }
            } }
        };

        stateArray.push_back(item);
    }

    root["animation_states"] = stateArray;
    return root;
}

void AnimationStateComponent::From_Json(const json& data)
{
    Component::From_Json(data);

    _stateAnimations.clear();
    _currentStateName = data.value("current_state", "");

    if (!data.contains("animation_states") || !data["animation_states"].is_array())
        return;

    for (const auto& item : data["animation_states"])
    {
        const string stateKey = item.value("key", "");
        if (stateKey.empty())
            continue;

        FStateAnimationDesc desc;

        const string modeName = item.value("mode", "Single");
        const auto modeOpt = magic_enum::enum_cast<EStateAnimationMode>(modeName);
        desc.mode = modeOpt.value_or(EStateAnimationMode::Single);

        if (item.contains("single"))
        {
            const auto& single = item["single"];
            desc.single.animationName = single.value("animationName", "");
            desc.single.loop = single.value("loop", true);
            desc.single.playRate = single.value("playRate", 1.f);
        }

        if (item.contains("start"))
        {
            const auto& start = item["start"];
            desc.start.animationName = start.value("animationName", "");
            desc.start.loop = start.value("loop", false);
            desc.start.playRate = start.value("playRate", 1.f);
        }

        if (item.contains("loopClip"))
        {
            const auto& loopClip = item["loopClip"];
            desc.loop.animationName = loopClip.value("animationName", "");
            desc.loop.loop = loopClip.value("loop", true);
            desc.loop.playRate = loopClip.value("playRate", 1.f);
        }

        if (item.contains("end"))
        {
            const auto& end = item["end"];
            desc.end.animationName = end.value("animationName", "");
            desc.end.loop = end.value("loop", false);
            desc.end.playRate = end.value("playRate", 1.f);
        }

        if (item.contains("directional"))
        {
            const auto& directional = item["directional"];

            if (directional.contains("forward"))
            {
                const auto& forward = directional["forward"];
                desc.directional.forward.animationName = forward.value("animationName", "");
                desc.directional.forward.loop = forward.value("loop", false);
                desc.directional.forward.playRate = forward.value("playRate", 1.f);
            }

            if (directional.contains("backward"))
            {
                const auto& backward = directional["backward"];
                desc.directional.backward.animationName = backward.value("animationName", "");
                desc.directional.backward.loop = backward.value("loop", false);
                desc.directional.backward.playRate = backward.value("playRate", 1.f);
            }

            if (directional.contains("left"))
            {
                const auto& left = directional["left"];
                desc.directional.left.animationName = left.value("animationName", "");
                desc.directional.left.loop = left.value("loop", false);
                desc.directional.left.playRate = left.value("playRate", 1.f);
            }

            if (directional.contains("right"))
            {
                const auto& right = directional["right"];
                desc.directional.right.animationName = right.value("animationName", "");
                desc.directional.right.loop = right.value("loop", false);
                desc.directional.right.playRate = right.value("playRate", 1.f);
            }
        }

        _stateAnimations[stateKey] = desc;
    }
}

Shared<AnimationStateComponent> AnimationStateComponent::Create(ComPtr<Device> device, ComPtr<DeviceContext> context)
{
    auto instance = make_shared<AnimationStateComponent>(device, context);

    if (FAILED(instance->Initialize_Prototype()))
    {
        MSG_BOX("Failed to Create : AnimationStateComponent");

        return nullptr;
    }

    return instance;
}

Shared<Component> AnimationStateComponent::Clone(void* arg)
{
    auto clone = make_shared<AnimationStateComponent>(*this);

    if (FAILED(clone->Initialize(arg)))
    {
        MSG_BOX("Failed to Clone : AnimationStateComponent");

        return nullptr;
    }

    return clone;
}

void AnimationStateComponent::Free()
{
    Component::Free();
}
