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
    PROPERTY_FLOAT("Yaw Speed", _moveDesc.yawSpeed, 0.f, 1080.f);
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

void MovementComponent::Start_Jump()
{
    if (!_onGround)
        return;

    _verticalVelocity = _moveDesc.jumpVelocity;
    _onGround = false;
    _canDoubleJump = true;
}

void MovementComponent::Start_DoubleJump()
{
    if (_onGround || !_canDoubleJump)
        return;

    _verticalVelocity = _moveDesc.doubleJumpVelocity;
    _canDoubleJump = false;
}

void MovementComponent::Update_Rotation(float timeDelta, Shared<Transform> transform)
{
    if (!_bOrientRotationToMovement)
        return;

    Vec3 desiredDir = Build_DesiredMoveDirection();
    if (desiredDir.LengthSquared() <= FLT_EPSILON)
        return;

    const float targetYaw = XMConvertToDegrees(atan2f(desiredDir.x, desiredDir.z));

    Vec3 currentEuler = transform->Get_LocalEulerAngles();
    float currentYaw = currentEuler.y;

    float deltaYaw = targetYaw - currentYaw;

    while (deltaYaw > 180.f)
        deltaYaw -= 360.f;

    while (deltaYaw < -180.f)
        deltaYaw += 360.f;

    const float maxStep = _moveDesc.yawSpeed * timeDelta;

    deltaYaw = ::clamp(deltaYaw, -maxStep, maxStep);

    transform->Set_LocalEulerAngles(
        currentEuler.x,
        currentYaw + deltaYaw,
        currentEuler.z);
}

void MovementComponent::Update_Velocity(float timeDelta, Shared<Transform> transform)
{
    Vec3 desiredDir = Build_DesiredMoveDirection();

    float targetSpeed = _commandDesc.sprint ? _moveDesc.maxSprintSpeed : _moveDesc.maxWalkSpeed;
    Vec3 targetVelocity = desiredDir * targetSpeed;
    targetVelocity.y = _velocity.y;

    bool hasInput = (_commandDesc.moveAxis.LengthSquared() > FLT_EPSILON);
    float accel = hasInput ? _moveDesc.acceleration : _moveDesc.deceleration;

    float alpha = ::clamp(accel * timeDelta, 0.f, 1.f);

    _velocity.x = ::lerp(_velocity.x, targetVelocity.x, alpha);
    _velocity.z = ::lerp(_velocity.z, targetVelocity.z, alpha);

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

    if (currentPos.y <= _moveDesc.groundY && _verticalVelocity <= 0.f)
    {
        currentPos.y = _moveDesc.groundY;
        transform->Set_LocalPosition(currentPos);

        _verticalVelocity = 0.f; // 떨어지는 속도 초기화
        _onGround = true;
        _canDoubleJump = false;
    }
    else
    {
        _onGround = false;
    }

}

Vec3 MovementComponent::Build_DesiredMoveDirection() const
{
    Vec2 input = _commandDesc.moveAxis;
    if (input.LengthSquared() > 1.f)
        input.Normalize();

    Vec3 forward = _commandDesc.moveBasisForward;
    Vec3 right = _commandDesc.moveBasisRight;

    forward.y = 0.f;
    right.y = 0.f;

    if (forward.LengthSquared() > FLT_EPSILON)
        forward.Normalize();

    if (right.LengthSquared() > FLT_EPSILON)
        right.Normalize();

    Vec3 desiredDir = right * input.x + forward * input.y;

    if (desiredDir.LengthSquared() > FLT_EPSILON)
        desiredDir.Normalize();

    return desiredDir;
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
