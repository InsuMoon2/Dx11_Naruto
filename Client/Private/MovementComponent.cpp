#include "pch.h"
#include "MovementComponent.h"
#include "GameObject.h"
#include "Transform.h"

MovementComponent::MovementComponent(ComPtr<Device> device, ComPtr<DeviceContext> context)
    : Component(device, context)
{
}

MovementComponent::MovementComponent(const MovementComponent& rhs)
    : Component(rhs)
    , _moveDesc(rhs._moveDesc)
    , _commandDesc(rhs._commandDesc)
    , _velocity(rhs._velocity)
{
}

MovementComponent::~MovementComponent()
{
}

HRESULT MovementComponent::Initialize_Prototype()
{
    Component::Initialize_Prototype();

    return S_OK;
}

HRESULT MovementComponent::Initialize(void* arg)
{
    if (arg)
    {
        _moveDesc = *static_cast<FMovementDesc*>(arg);
    }

    _velocity = Vec3::Zero;

    Component::Initialize(arg);



    return S_OK;
}

void MovementComponent::BeginPlay()
{
    Component::BeginPlay();

    _owner = Get_Owner();
    _transform = _owner.lock()->Get_Component<Transform>();

    CHECK_NULL(_transform);
}

void MovementComponent::Apply_Command(const FMoveCommand& cmd)
{
    _commandDesc = cmd;
}

void MovementComponent::Update(float timeDelta)
{
    Update_Rotation(timeDelta, _transform);
    Update_Velocity(timeDelta, _transform);
    Apply_Movement(timeDelta, _transform);
}

json MovementComponent::To_Json() const
{
    json j = Component::To_Json();

    j["max_walk_speed"] = _moveDesc.maxWalkSpeed;
    j["max_sprint_speed"] = _moveDesc.maxSprintSpeed;
    j["acceleration"] = _moveDesc.acceleration;
    j["deceleration"] = _moveDesc.deceleration;
    j["yaw_speed"] = _moveDesc.yawSpeed;

    return j;
}

void MovementComponent::From_Json(const json& data)
{
    Component::From_Json(data);

    if (data.contains("max_walk_speed"))    _moveDesc.maxWalkSpeed = data["max_walk_speed"].get<float>();
    if (data.contains("max_sprint_speed"))  _moveDesc.maxSprintSpeed = data["max_sprint_speed"].get<float>();
    if (data.contains("acceleration"))      _moveDesc.acceleration = data["acceleration"].get<float>();
    if (data.contains("deceleration"))      _moveDesc.deceleration = data["deceleration"].get<float>();
    if (data.contains("yaw_speed"))         _moveDesc.yawSpeed = data["yaw_speed"].get<float>();
}

void MovementComponent::Update_Rotation(float timeDelta, Shared<Transform> transform)
{
    float yawDelta = _commandDesc.lookDelta.x * _moveDesc.yawSpeed;

    if (fabsf(yawDelta) > 0.0001f)
    {
        transform->Rotate_Axis(Vec3::Up, yawDelta);
    }
}

void MovementComponent::Update_Velocity(float timeDelta, Shared<Transform> transform)
{
    // 입력 정규화
    Vec2 input = _commandDesc.moveAxis;
    if (input.LengthSquared() > 1.f)
        input.Normalize();

    // 월드 방향 계산
    Vec3 desiredDir = transform->Get_WorldRight() * input.x + transform->Get_WorldForward() * input.y;

    if (desiredDir.LengthSquared() > FLT_EPSILON)
        desiredDir.Normalize();

    float   targetSpeed = _commandDesc.sprint ? _moveDesc.maxSprintSpeed : _moveDesc.maxWalkSpeed;
    Vec3    targetVelocity = desiredDir * targetSpeed;
    targetVelocity.y = _velocity.y;

    // 보간
    bool    hasInput = (input.LengthSquared() > FLT_EPSILON);
    float   accel = hasInput ? _moveDesc.acceleration : _moveDesc.deceleration;
    float   alpha = ::clamp(accel * timeDelta, 0.f, 1.f);
    
    _velocity = Vec3::Lerp(_velocity, targetVelocity, alpha);
}

void MovementComponent::Apply_Movement(float timeDelta, Shared<Transform> transform)
{
    transform->Add_WorldOffset(_velocity * timeDelta);
}

Shared<MovementComponent> MovementComponent::Create(ComPtr<Device> device, ComPtr<DeviceContext> context)
{
    auto instance = make_shared<MovementComponent>(device, context);

    if (FAILED(instance->Initialize_Prototype()))
    {
        MSG_BOX("Failed to Create: MovementComponent");
        instance.reset();
    }

    return instance;
}

Shared<Component> MovementComponent::Clone(void* arg)
{
    auto clone = make_shared<MovementComponent>(*this);

    if (FAILED(clone->Initialize(arg)))
    {
        MSG_BOX("Failed to Clone: MovementComponent");
        clone.reset();
    }

    return clone;
}

void MovementComponent::Free()
{
    Component::Free();
}
