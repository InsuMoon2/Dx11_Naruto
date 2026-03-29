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

    PROPERTY_FLOAT("Dash Distance", _moveDesc.dashDistance, 0.f, 30.f);
    PROPERTY_FLOAT("Dash Duration", _moveDesc.dashDuration, 0.01f, 1.f);

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
    , _gravityEnabled(rhs._gravityEnabled)
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

    _velocity.y = _moveDesc.jumpVelocity;
    _onGround = false;
    _canDoubleJump = true;
}

void MovementComponent::Start_DoubleJump()
{
    if (_onGround || !_canDoubleJump)
        return;

    _velocity.y = _moveDesc.doubleJumpVelocity;
    _canDoubleJump = false;
}

void MovementComponent::Start_SuperJump(float velocity)
{
    if (!_onGround)
        return;

    _velocity.y = Utils::Max(velocity, _moveDesc.superJumpMinVelocity);
    _onGround = false;

    // 슈퍼점프 이후 더블점프 가능하게할지?
    _canDoubleJump = true;
}

void MovementComponent::Start_Dash(const Vec3& worldDir, float distance, float duration)
{
    Vec3 dashDir = worldDir;
    dashDir.y = 0.f;

    if (dashDir.LengthSquared() <= FLT_EPSILON)
        return;

    dashDir.Normalize();

    _isDashing = true;
    _dashWorldDirection = dashDir;
    _dashElapsed = 0.f;
    _dashDuration = Utils::Max(duration, 0.01f);

    _dashSpeed = distance / _dashDuration;
}

//void MovementComponent::Start_Dash(EMoveInputDirection inputDir, float distance, float duration)
//{
//
//    Vec3 forward    = _transform->Get_WorldForward();
//    Vec3 right      = _transform->Get_WorldRight();
//
//    // y값 제거
//    forward.y = 0;
//    right.y = 0;
//    // 정규화
//    if (forward.LengthSquared() > FLT_EPSILON)
//        forward.Normalize();
//
//    if (right.LengthSquared() > FLT_EPSILON)
//        right.Normalize();
//
//    Vec3 dashDir = forward;
//
//    switch (inputDir)
//    {
//    case EMoveInputDirection::Forward:
//        dashDir = forward;
//        break;
//    case EMoveInputDirection::Backward:
//        dashDir = -forward;
//        break;
//    case EMoveInputDirection::Left:
//        dashDir = -right;
//        break;
//    case EMoveInputDirection::Right:
//        dashDir = right;
//        break;
//    // 기본값은 forward로
//    default: dashDir = forward; break;
//    }
//
//    _isDashing = true;
//    _dashInputDirection = inputDir;
//    _dashWorldDirection = dashDir;
//    _dashElapsed = 0.f;
//    _dashDuration = Utils::Max(duration, 0.01f);
//
//    // 거리 / 시간 기반으로 속도 세팅
//    _dashSpeed = distance / _dashDuration;
//}

void MovementComponent::Stop_Dash()
{
    _isDashing = false;
    _dashInputDirection = EMoveInputDirection::Forward;
    _dashWorldDirection = Vec3::Zero;
    _dashElapsed = 0.f;
    _dashDuration = 0.f;
    _dashSpeed = 0.f;
}

void MovementComponent::Set_Velocity(Vec3 velocity)
{
    _velocity = velocity;


}

float MovementComponent::Get_DashNormalizedTime() const
{
    if (!_isDashing || _dashDuration <= FLT_EPSILON)
        return 1.f;

    return ::clamp(_dashElapsed / _dashDuration, 0.f, 1.f);
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

    // 대쉬 우선
    if (_isDashing)
    {
        _dashElapsed += timeDelta;

        _velocity.x = _dashWorldDirection.x * _dashSpeed;
        _velocity.z = _dashWorldDirection.z * _dashSpeed;

        if (_dashElapsed >= _dashDuration)
        {
            Stop_Dash();
        }

        if (_gravityEnabled && !_onGround)
        {
            _velocity.y += _moveDesc.gravity * timeDelta;
        }

        return;
    }

    float targetSpeed = _commandDesc.sprint ? _moveDesc.maxSprintSpeed : _moveDesc.maxWalkSpeed;

    Vec3 targetVelocity = desiredDir * targetSpeed;
    targetVelocity.y = _velocity.y;

    bool hasInput = (_commandDesc.moveAxis.LengthSquared() > FLT_EPSILON);
    float accel = hasInput ? _moveDesc.acceleration : _moveDesc.deceleration;

    float alpha = ::clamp(accel * timeDelta, 0.f, 1.f);

    _velocity.x = ::lerp(_velocity.x, targetVelocity.x, alpha);
    _velocity.z = ::lerp(_velocity.z, targetVelocity.z, alpha);

    if (_gravityEnabled && !_onGround)
    {
        _velocity.y += _moveDesc.gravity * timeDelta;
    }

}

void MovementComponent::Apply_Movement(float timeDelta, Shared<Transform> transform)
{
    transform->Add_WorldOffset(_velocity * timeDelta);

    // TODO : Temp 바닥 충돌처리, 나중에는 충돌체 기준으로
    Vec3 currentPos = transform->Get_WorldPosition();

    if (currentPos.y <= _moveDesc.groundY && _velocity.y <= 0.f)
    {
        currentPos.y = _moveDesc.groundY;
        transform->Set_LocalPosition(currentPos);

        _velocity.y = 0.f; // 떨어지는 속도 초기화
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
