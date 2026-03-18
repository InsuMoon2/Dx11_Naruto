#include "pch.h"
#include "PlayerStateMachine.h"
#include "GameObject.h"
#include "InputComponent.h"
#include "MovementComponent.h"
#include "AnimationStateComponent.h"
#include "Model.h"
#include "PlayerState_DoubleJump.h"
#include "Transform.h"
#include "PlayerState_Idle.h"
#include "PlayerState_Run.h"
#include "PlayerState_Jump.h"
#include "PlayerState_SuperJump.h"
#include "Camera.h"
#include "PlayerState_Dash.h"
#include "PlayerState_HeightLand.h"
#include "PlayerState_SuperJumpCharge.h"

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
    Register_State(EPlayerState::SuperJumpCharge, PlayerState_SuperJumpCharge::Create());
    Register_State(EPlayerState::SuperJump, PlayerState_SuperJump::Create());

    Register_State(EPlayerState::HeightLand, PlayerState_HeightLand::Create());

    Register_State(EPlayerState::Dash, PlayerState_Dash::Create());

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
    _animationState = owner->Get_Component<AnimationStateComponent>();

    CHECK_NULL(_input);
    CHECK_NULL(_movement);
    CHECK_NULL(_animationState);

    Change_State(EPlayerState::Idle);
}

void PlayerStateMachine::Update(float timeDelta)
{
    if (_currentState)
        _currentState->Update(this, timeDelta);
}

string PlayerStateMachine::To_AnimationStateName(EPlayerState stateID)
{
    const auto name = magic_enum::enum_name(stateID);

    return name.empty() ? "" : string(name);
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
    cmd.sprint = false;
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

bool PlayerStateMachine::Play_AnimState(EPlayerState stateID)
{
    return _animationState->Play_State(To_AnimationStateName(stateID));
}

bool PlayerStateMachine::Play_DirectionalAnimState(EPlayerState stateID, EMoveInputDirection dir)
{
    return _animationState->Play_DirectionalState(To_AnimationStateName(stateID), dir);
}

bool PlayerStateMachine::Play_AnimStateLoopOnly(EPlayerState stateID)
{
    return _animationState->Play_StateLoopOnly(To_AnimationStateName(stateID));
}

void PlayerStateMachine::Request_AnimStateEnd()
{
    _animationState->Request_StateEnd();
}

bool PlayerStateMachine::Is_AnimStateFinished() const
{
    return _animationState ? _animationState->Is_CurrentStateFinished() : false;
}

bool PlayerStateMachine::Is_AnimSequenceFinished() const
{
    return _animationState ? _animationState->Is_CurrentStateSequenceFinished() : false;
}

const FStateAnimationDesc* PlayerStateMachine::Find_AnimStateDesc(EPlayerState stateID) const
{
    return _animationState ?
        _animationState->Find_State(To_AnimationStateName(stateID)) : nullptr;
}

EAnimPhase PlayerStateMachine::Get_AnimPhase() const
{
    return _animationState ? _animationState->Get_CurrentAnimPhase() : EAnimPhase::Start;
}

float PlayerStateMachine::Get_AnimTrackPosition() const
{
    return _animationState ? _animationState->Get_CurrentTrackPosition() : 0.f;
}

float PlayerStateMachine::Get_AnimDuration() const
{
    return _animationState ? _animationState->Get_CurrentAnimationDuration() : 0.f;
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
