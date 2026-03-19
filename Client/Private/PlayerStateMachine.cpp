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
    return Component::To_Json();
}

void PlayerStateMachine::From_Json(const json& data)
{
    Component::From_Json(data);

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
