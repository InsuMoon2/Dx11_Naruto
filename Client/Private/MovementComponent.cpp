#include "pch.h"
#include "MovementComponent.h"
#include "GameObject.h"
#include "Transform.h"

IMPLEMENT_REFLECTION(MovementComponent)

bool MovementComponent::Register_Properties()
{
    auto& info = GetStaticReflectionInfo();
    info.className = "MovementComponent";

    PROPERTY_FLOAT("Max Walk Speed", _moveDesc.maxWalkSpeed, 0.f, 30.f);
    PROPERTY_FLOAT("Max Sprint Speed", _moveDesc.maxSprintSpeed, 0.f, 30.f);
    PROPERTY_FLOAT("Acceleration", _moveDesc.acceleration, 0.f, 50.f);
    PROPERTY_FLOAT("Deceleration", _moveDesc.deceleration, 0.f, 50.f);
    PROPERTY_FLOAT("Yaw Speed", _moveDesc.yawSpeed, 0.f, 5.f);
    PROPERTY_FLOAT("Jump Velocity", _moveDesc.jumpVelocity, 0.f, 30.f);
    PROPERTY_FLOAT("Gravity", _moveDesc.gravity, -50.f, 0.f);
    PROPERTY_FLOAT("Ground Y", _moveDesc.groundY, -100.f, 100.f);

    return true;
}

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

    // 수평 이동
    _velocity.x = ::lerp(_velocity.x, targetVelocity.x, alpha);
    _velocity.z = ::lerp(_velocity.z, targetVelocity.z, alpha);

    if (_onGround && _commandDesc.jump)
    {
        _verticalVelocity = _moveDesc.jumpVelocity;
        _onGround = false;
    }

    if (!_onGround)
    {
        _verticalVelocity += _moveDesc.gravity * timeDelta;
    }

    _velocity.y = _verticalVelocity;

}

void MovementComponent::Apply_Movement(float timeDelta, Shared<Transform> transform)
{
    transform->Add_WorldOffset(_velocity * timeDelta);

    // TODO : Temp 바닥 충돌처리, 나중에는 충돌체 기준으로
    Vec3 currentPos = transform->Get_WorldPosition();

    if (currentPos.y <= _moveDesc.groundY)
    {
        currentPos.y = _moveDesc.groundY;
        transform->Set_LocalPosition(currentPos);

        _verticalVelocity = 0.f; // 떨어지는 속도 초기화
        _onGround = true;
    }
    else
    {
        _onGround = false;
    }

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
