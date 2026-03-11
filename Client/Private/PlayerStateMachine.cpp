#include "pch.h"
#include "PlayerStateMachine.h"
#include "GameObject.h"
#include "InputComponent.h"
#include "MovementComponent.h"
#include "PlayerState_DoubleJump.h"

#include "PlayerState_Idle.h"
#include "PlayerState_Run.h"
#include "PlayerState_Jump.h"
#include "PlayerState_SuperJump.h"

IMPLEMENT_REFLECTION(PlayerStateMachine)

bool PlayerStateMachine::Register_Properties()
{
    auto& info = GetStaticReflectionInfo();
    info.className = "PlayerStateMachine";

    PROPERTY_ENUM("Current State", _currentStateID, EPlayerState);

    return true;
}

PlayerStateMachine::PlayerStateMachine(ComPtr<Device> device, ComPtr<DeviceContext> context)
    : Component(device, context)
{
}

PlayerStateMachine::PlayerStateMachine(const PlayerStateMachine& rhs)
    : Component(rhs)
    , _currentStateID(rhs._currentStateID)
    , _prevStateID(rhs._prevStateID)
{
    _states = rhs._states;
}

PlayerStateMachine::~PlayerStateMachine()
{
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

    CHECK_NULL(_input);
    CHECK_NULL(_movement);

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

    return cmd;
}

json PlayerStateMachine::To_Json() const
{
    json root = Component::To_Json();
    
    return root;
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
