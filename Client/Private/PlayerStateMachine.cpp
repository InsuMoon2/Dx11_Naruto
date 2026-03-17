#include "pch.h"
#include "PlayerStateMachine.h"
#include "GameObject.h"
#include "InputComponent.h"
#include "MovementComponent.h"
#include "Model.h"
#include "PlayerState_DoubleJump.h"
#include "Transform.h"
#include "PlayerState_Idle.h"
#include "PlayerState_Run.h"
#include "PlayerState_Jump.h"
#include "PlayerState_SuperJump.h"
#include "Camera.h"

IMPLEMENT_REFLECTION(PlayerStateMachine)

bool PlayerStateMachine::Register_Properties()
{
    auto& info = GetStaticReflectionInfo();
    info.className = "PlayerStateMachine";

    //PROPERTY_ENUM("Current State", _currentStateID, EPlayerState);

    return true;
}

PlayerStateMachine::PlayerStateMachine(ComPtr<Device> device, ComPtr<DeviceContext> context)
    : Component(device, context)
{
}

PlayerStateMachine::PlayerStateMachine(const PlayerStateMachine& rhs)
    : Component(rhs)
{
    _states = rhs._states;
    _stateAnimations = rhs._stateAnimations;

    _currentState = nullptr;
    _currentStateID = EPlayerState::END;
    _prevStateID = EPlayerState::END;
}


HRESULT PlayerStateMachine::Initialize_Prototype()
{
    Component::Initialize_Prototype();

    Register_State(EPlayerState::Idle, PlayerState_Idle::Create());
    Register_State(EPlayerState::Run, PlayerState_Run::Create());

    Register_State(EPlayerState::Jump, PlayerState_Jump::Create());
    Register_State(EPlayerState::DoubleJump, PlayerState_DoubleJump::Create());
    Register_State(EPlayerState::SuperJump, PlayerState_SuperJump::Create());

    return S_OK;
}

HRESULT PlayerStateMachine::Initialize(void* arg)
{
    Component::Initialize(arg);

    return S_OK;
}

void PlayerStateMachine::BeginPlay()
{
    Component::BeginPlay();

    auto owner = Get_Owner();
    _input = owner->Get_Component<InputComponent>();
    _movement = owner->Get_Component<MovementComponent>();
    _model = Get_Model();

    CHECK_NULL(_input);
    CHECK_NULL(_movement);
    CHECK_NULL(_model);

    Change_State(EPlayerState::Idle);
}

void PlayerStateMachine::Update(float timeDelta)
{
    if (_currentState)
        _currentState->Update(this, timeDelta);
}

void PlayerStateMachine::Register_State(EPlayerState stateID, Shared<IPlayerState> state)
{
    _states[stateID] = state;
}

void PlayerStateMachine::Change_State(EPlayerState newState)
{
    if (newState == _currentStateID)
        return;

    auto iter = _states.find(newState);
    if (iter == _states.end())
        return;

    // 현재 상태 종료
    if (_currentState)
        _currentState->Exit(this);

    // 전환
    _prevStateID    = _currentStateID;
    _currentStateID = newState;
    _currentState   = iter->second;

    _currentState->Enter(this);
}

MovementComponent::FMoveCommand PlayerStateMachine::Init_MoveCommand() const
{
    const auto& frame = _input->Get_Frame();

    MovementComponent::FMoveCommand cmd;
    cmd.moveAxis = Vec2(frame.moveX, frame.moveY);
    cmd.sprint = frame.sprintPress;
    cmd.jump = false;

    auto activeCamera = GAME->Get_ActiveCamera();
    if (activeCamera && activeCamera->Get_ObjectType() == Protocol::OBJECT_TYPE_CAMERA_TARGET)
    {
        auto cameraTransform = activeCamera->Get_Component<Transform>();
        if (cameraTransform)
        {
            Vec3 forward = cameraTransform->Get_WorldForward();
            Vec3 right = cameraTransform->Get_WorldRight();

            forward.y = 0.f;
            right.y = 0.f;

            if (forward.LengthSquared() > FLT_EPSILON)
                forward.Normalize();

            if (right.LengthSquared() > FLT_EPSILON)
                right.Normalize();

            cmd.moveBasisForward = forward;
            cmd.moveBasisRight = right;
        }
    }

    return cmd;
}

Shared<Model> PlayerStateMachine::Get_Model()
{
    auto owner = Get_Owner();
    if (!owner)
    {
        _model.reset();
        return nullptr;
    }

    auto modelCom = owner->Get_Component(Protocol::COMPONENT_TYPE_MODEL_SASKE);
    if (!modelCom)
    {
        _model.reset();
        return nullptr;
    }

    _model = static_pointer_cast<Model>(modelCom);
    return _model;
}

vector<string> PlayerStateMachine::Get_AvaiableAnimationNames()
{
    vector<string> result;

    const auto model = Get_Model();
    if (!model)
        return result;

    const uint32 count = model->Get_AnimationCount();
    result.reserve(count);

    for (uint32 i = 0; i < count; ++i)
    {
        const string& name = model->Get_AnimationName(i);
        if (!name.empty())
        {
            result.push_back(name);
        }
    }

    return result;
}

const FStateAnimationDesc* PlayerStateMachine::Find_StateAnimation(EPlayerState stateID) const
{
    auto iter = _stateAnimations.find(stateID);

    if (iter == _stateAnimations.end())
        return nullptr;

    return &iter->second;
}

FStateAnimationDesc& PlayerStateMachine::Edit_StateAnimation(EPlayerState stateID)
{
    return _stateAnimations[stateID];
}

bool PlayerStateMachine::Apply_StateAnimation(EPlayerState stateID)
{
    auto model = Get_Model();
    if (!model)
        return false;

    const auto* animDesc = Find_StateAnimation(stateID);
    if (!animDesc)
        return false;

    if (animDesc->mode == EStateAnimationMode::Sequence)
    {
        if (animDesc->loop.animationName.empty())
            return false;

        model->Set_AnimationSequence(animDesc->start, animDesc->loop, animDesc->end);
        return true;
    }

    if (animDesc->single.animationName.empty())
        return false;

    model->Set_Animation(animDesc->single);

    return true;
}

bool PlayerStateMachine::Preview_StateAnimation(EPlayerState stateID, int32 sequenceSlot)
{
    auto model = Get_Model();
    if (!model)
        return false;

    const auto* animDesc = Find_StateAnimation(stateID);
    if (!animDesc)
        return false;

    model->Set_AnimationPlayRate(animDesc->playRate);

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

void PlayerStateMachine::Force_Enter_State(EPlayerState stateID)
{
	auto iter = _states.find(stateID);
	if (iter == _states.end())
		return;

	_prevStateID = _currentStateID;
	_currentStateID = stateID;
	_currentState = iter->second;

	if (_currentState)
		_currentState->Enter(this);
}

json PlayerStateMachine::To_Json() const
{
    json root = Component::To_Json();
    json animArray = json::array();

    for (const auto& [stateID, desc] : _stateAnimations)
    {
        json item;
        item["state"] = string(magic_enum::enum_name(stateID));
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

        animArray.push_back(item);
    }

    root["stateAnimations"] = animArray;
    return root;
}

void PlayerStateMachine::From_Json(const json& data)
{
    Component::From_Json(data);

    _stateAnimations.clear();

    if (!data.contains("stateAnimations") || !data["stateAnimations"].is_array())
        return;

    for (const auto& item : data["stateAnimations"])
    {
        const string stateName = item.value("state", "");
        auto stateOpt = magic_enum::enum_cast<EPlayerState>(stateName);
        if (!stateOpt.has_value())
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

        // 구형 일단은 호환
        if (!item.contains("single") && item.contains("animationName"))
        {
            desc.single.animationName = item.value("animationName", "");
            desc.single.loop = item.value("loop", true);
            desc.single.playRate = item.value("playRate", 1.f);

            desc.start.animationName = item.value("startAnimationName", "");
            desc.loop.animationName = item.value("loopAnimationName", "");
            desc.end.animationName = item.value("endAnimationName", "");

            desc.start.loop = false;
            desc.loop.loop = true;
            desc.end.loop = false;

            desc.start.playRate = item.value("playRate", 1.f);
            desc.loop.playRate = item.value("playRate", 1.f);
            desc.end.playRate = item.value("playRate", 1.f);
        }

        _stateAnimations[stateOpt.value()] = desc;
    }
}

Shared<PlayerStateMachine> PlayerStateMachine::Create(ComPtr<Device> device, ComPtr<DeviceContext> context)
{
    auto instance = make_shared<PlayerStateMachine>(device, context);

    if (FAILED(instance->Initialize_Prototype()))
    {
        MSG_BOX("Failed to Create: PlayerStateMachine");
        instance.reset();
    }

    return instance;
}

Shared<Component> PlayerStateMachine::Clone(void* arg)
{
    auto clone = make_shared<PlayerStateMachine>(*this);

    if (FAILED(clone->Initialize(arg)))
    {
        MSG_BOX("Failed to Clone: PlayerStateMachine");
        clone.reset();
    }

    return clone;
}

void PlayerStateMachine::Free()
{
    _states.clear();
    _currentState = nullptr;

    Component::Free();
}
