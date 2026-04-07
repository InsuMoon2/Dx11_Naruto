#include "pch.h"
#include "MovementComponent.h"

#include "Camera.h"

#include "GameObject.h"
#include "Transform.h"
#include "Model.h"

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

    PROPERTY_FLOAT("Wall Trace Start Offset Y", _moveDesc.wallTraceStartOffsetY, 0.f, 3.f);
    PROPERTY_FLOAT("Wall Detect Distance", _moveDesc.wallDetectDistance, 0.1f, 3.f);
    PROPERTY_FLOAT("Wall Attach Offset", _moveDesc.wallAttachOffset, 0.01f, 1.f);
    PROPERTY_FLOAT("Wall Runnable Max Up Dot", _moveDesc.wallRunnableMaxUpDot, 0.f, 1.f);
    PROPERTY_FLOAT("Wall Jump Up Velocity", _moveDesc.wallJumpUpVelocity, 0.f, 30.f);
    PROPERTY_FLOAT("Wall Jump Out Velocity", _moveDesc.wallJumpOutVelocity, 0.f, 30.f);

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
    _isWallRunning = false;
    _currentWallNormal = Vec3::Up;
    _currentWallHitPoint = Vec3::Zero;

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
    if (_wallJumpCooldown > 0.f)
        _wallJumpCooldown -= timeDelta;

    if (!_isWallRunning)
    {
        Update_Rotation(timeDelta, _transform);
    }

    Update_Velocity(timeDelta, _transform);
    Apply_Movement(timeDelta, _transform);

    if (_isWallRunning)
    {
        Apply_WallRunRotation(timeDelta, _transform);
    }
}

void MovementComponent::Start_Jump()
{
    if (!_onGround)
        return;

    _velocity.y = _moveDesc.jumpVelocity;
    _onGround = false;

    _isWallRunning = false; // 일반 점프 시 wall run 해제

    _canDoubleJump = true;
}

void MovementComponent::Start_DoubleJump()
{
    if (_onGround || !_canDoubleJump)
        return;

    _isWallRunning = false;
    _velocity.y = _moveDesc.doubleJumpVelocity;
    _canDoubleJump = false;
}

void MovementComponent::Start_SuperJump(float velocity)
{
    if (!_onGround)
        return;

    _velocity.y = Utils::Max(velocity, _moveDesc.superJumpMinVelocity);
    _onGround = false;

    _isWallRunning = false;
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

void MovementComponent::Stop_Dash()
{
    _isDashing = false;
    _dashInputDirection = EMoveInputDirection::Forward;
    _dashWorldDirection = Vec3::Zero;
    _dashElapsed = 0.f;
    _dashDuration = 0.f;
    _dashSpeed = 0.f;
}

void MovementComponent::Start_WallJump()
{
    if (!_isWallRunning)
        return;

    // 벽 바깥 방향 + 위 방향 섞어서 벽차기 속도 세팅
    Vec3 launchVelocity = _currentWallNormal * _moveDesc.wallJumpOutVelocity;
    launchVelocity += Vec3::Up * _moveDesc.wallJumpUpVelocity;

    _velocity = launchVelocity;
    _onGround = false;
    _canDoubleJump = true;

    Exit_WallRun();

    _wallJumpCooldown = 0.35f;
}

void MovementComponent::Set_Velocity(Vec3 velocity)
{
    _velocity = velocity;


}

void MovementComponent::Launch(const Vec3& launchVelocity, bool xyOverride, bool zOverride)
{
    if (xyOverride)
    {
        _velocity.x = launchVelocity.x;
        _velocity.z = launchVelocity.z;
    }
    else
    {
        _velocity.x += launchVelocity.x;
        _velocity.z += launchVelocity.z;
    }
    if (zOverride)
    {
        _velocity.y = launchVelocity.y;
    }
    else
    {
        _velocity.y += launchVelocity.y;
    }

    if (launchVelocity.y > 0.f || launchVelocity.LengthSquared() > FLT_EPSILON)
    {
        _onGround = false;
        _canDoubleJump = false;
    }
}

float MovementComponent::Get_DashNormalizedTime() const
{
    if (!_isDashing || _dashDuration <= FLT_EPSILON)
        return 1.f;

    return ::clamp(_dashElapsed / _dashDuration, 0.f, 1.f);
}

bool MovementComponent::Try_WireDash_WallTrace(const Vec3& traceStart, const Vec3& traceDir, FSurfaceHit& outHit) const
{
    Vec3 dir = traceDir;
    if (dir.LengthSquared() <= FLT_EPSILON)
        dir = Vec3::Forward;

    dir = Utils::Safe_Normalize(dir, Vec3::Forward);

    Ray wallRay(traceStart, dir);

    FSurfaceHit bestHit{};

    for (const auto& colModel : _wallCollisionModels)
    {
        if (!colModel)
            continue;

        float hitDist = 0.f;
        Vec3 hitPoint = Vec3::Zero;
        Vec3 hitNormal = Vec3::Up;

        if (!colModel->Raycast(wallRay, hitDist, hitPoint, hitNormal))
            continue;

        if (hitDist > _wireDashDesc.maxDistance)
            continue;

        const float upDotAbs = fabsf(hitNormal.Dot(Vec3::Up));
        if (upDotAbs > _moveDesc.wallRunnableMaxUpDot)
            continue;

        if (!bestHit.isValid || hitDist < bestHit.hitDistance)
        {
            bestHit.hitModel = colModel;
            bestHit.hitPoint = hitPoint;
            bestHit.hitNormal = hitNormal;
            bestHit.hitDistance = hitDist;
            bestHit.isValid = true;
        }
    }

    outHit = bestHit;
    return bestHit.isValid;
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

    if (_isWallRunning)
    {
        Vec3 wallMoveDir = Build_WallRunMoveDirection();
        float targetSpeed = _commandDesc.sprint ? _moveDesc.maxSprintSpeed : _moveDesc.maxWalkSpeed;

        Vec3 targetVelocity = wallMoveDir * targetSpeed;

        if (_commandDesc.moveAxis.LengthSquared() <= FLT_EPSILON)
        {
            targetVelocity = Vec3::Zero;
        }

        float hasInput = (_commandDesc.moveAxis.LengthSquared() > FLT_EPSILON) ? 1.f : 0.f;
        float accel = hasInput > 0.f ? _moveDesc.acceleration : _moveDesc.deceleration;
        float alpha = ::clamp(accel * timeDelta, 0.f, 1.f);

        _velocity.x = ::lerp(_velocity.x, targetVelocity.x, alpha);
        _velocity.y = ::lerp(_velocity.y, targetVelocity.y, alpha);
        _velocity.z = ::lerp(_velocity.z, targetVelocity.z, alpha);

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

    Vec3 currentPos = transform->Get_WorldPosition();

    if (_isWallRunning)
    {
        FSurfaceHit wallHit{};

        if (Detect_WallSurface(currentPos, -_currentWallNormal, wallHit))
        {
            _currentWallNormal = wallHit.hitNormal;
            _currentWallHitPoint = wallHit.hitPoint;

            Apply_WallRunPosition(transform, wallHit);
            return;
        }

        Exit_WallRun();
    }

    if (_velocity.y <= 0.f)
    {
        FSurfaceHit groundHit{};
        bool foundGround = Detect_GroundSurface(currentPos, groundHit);

        if (foundGround && currentPos.y <= groundHit.hitPoint.y + _moveDesc.groundSnapTolerance)
        {
            currentPos.y = groundHit.hitPoint.y;
            transform->Set_WorldPosition(currentPos);

            _velocity.y = 0.f;
            _onGround = true;
            _canDoubleJump = false;

            return;
        }

        if (!foundGround && currentPos.y <= _moveDesc.groundY)
        {
            currentPos.y = _moveDesc.groundY;
            transform->Set_WorldPosition(currentPos);

            _velocity.y = 0.f;
            _onGround = true;
            _canDoubleJump = false;

            return;
        }
    }

    _onGround = false;

    Vec3 desiredDir = Build_DesiredMoveDirection();
    FSurfaceHit wallHit{};

    if (_wallJumpCooldown <= 0.f &&
        (Detect_WallSurface(transform->Get_WorldPosition(), desiredDir, wallHit) &&
        Can_EnterWallRun(wallHit, desiredDir)))
    {
        Enter_WallRun(wallHit);
        Apply_WallRunPosition(transform, wallHit);

        return;
    }

    // 공중이고, 벽타기 상태가 아닐 때
    if (!_onGround && !_isWallRunning)
    {
        FSurfaceHit slideHit{};

        // 현재 위치에서 이동하려는 velocity를 향해 레이를 쏴서 벽이 있는지 체크해보기
        if (Detect_WallSurface(transform->Get_WorldPosition(), _velocity, slideHit))
        {
            Vec3 moveDir = _velocity;
            moveDir.Normalize();

            // 내적했을 때 음수면 바깥방향
            if (moveDir.Dot(slideHit.hitNormal) < 0.f)
            {
                // 벽으로 파고들어가는 힘 제거
                _velocity = Utils::Project_OnPlane(_velocity, slideHit.hitNormal);

            }
        }
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

Vec3 MovementComponent::Build_WallRunMoveDirection() const
{
    Vec2 input = _commandDesc.moveAxis;
    if (input.LengthSquared() > 1.f)
        input.Normalize();

    Vec3 camForward = Vec3::Forward;
    Vec3 camRight = Vec3::Right;

    auto activeCamera = GAME->Get_ActiveCamera();
    if (activeCamera)
    {
        auto camTransform = activeCamera->Get_Component<Transform>();
        if (camTransform)
        {
            camForward = camTransform->Get_WorldForward();
            camRight = camTransform->Get_WorldRight();
        }
    }

    Vec3 desiredDir = camRight * input.x + camForward * input.y;

    desiredDir = Utils::Project_OnPlane(desiredDir, _currentWallNormal);

    if (desiredDir.LengthSquared() <= FLT_EPSILON)
    {
        desiredDir = Utils::Project_OnPlane(_transform->Get_WorldForward(), _currentWallNormal);
    }

    if (desiredDir.LengthSquared() <= FLT_EPSILON)
        desiredDir = Utils::Project_OnPlane(Vec3::Up, _currentWallNormal);

    return Utils::Safe_Normalize(desiredDir, Vec3::Forward);
}

bool MovementComponent::Detect_GroundSurface(const Vec3& currentPos, FSurfaceHit& outHit) const
{
    Vec3 rayOrigin = currentPos + Vec3(0.f, _moveDesc.groundTraceStartOffsetY, 0.f);
    Ray downRay(rayOrigin, Vec3(0.f, -1.f, 0.f));

    FSurfaceHit bestHit{};
    float bestHeight = -FLT_MAX;

    for (const auto& colModel : _groundCollisionModels)
    {
        if (!colModel)
            continue;

        float hitDist = 0.f;
        Vec3 hitPoint = Vec3::Zero;
        Vec3 hitNormal = Vec3::Up;

        if (!colModel->Raycast(downRay, hitDist, hitPoint, hitNormal))
            continue;

        if (hitPoint.y > bestHeight)
        {
            bestHeight = hitPoint.y;
            bestHit.hitModel = colModel;
            bestHit.hitPoint = hitPoint;
            bestHit.hitNormal = hitNormal;
            bestHit.hitDistance = hitDist;
            bestHit.isValid = true;
        }
    }

    outHit = bestHit;

    return bestHit.isValid;
}

bool MovementComponent::Detect_WallSurface(const Vec3& currentPos, const Vec3& castDir,
    FSurfaceHit& outHit) const
{
    Vec3 dir = castDir;
    if (dir.LengthSquared() <= FLT_EPSILON)
    {
        if (_transform)
            dir = _transform->Get_WorldForward();
        else
            dir = Vec3::Forward;
    }

    dir = Utils::Safe_Normalize(dir, Vec3::Forward);

    Vec3 rayOrigin = currentPos + Vec3(0.f, _moveDesc.wallTraceStartOffsetY, 0.f);
    Ray wallRay(rayOrigin, dir);

    FSurfaceHit bestHit{};


    for (const auto& colModel : _wallCollisionModels)
    {
        if (!colModel)
            continue;

        float hitDist = 0.f;
        Vec3 hitPoint = Vec3::Zero;
        Vec3 hitNormal = Vec3::Up;

        if (!colModel->Raycast(wallRay, hitDist, hitPoint, hitNormal))
            continue;

        if (hitDist > _moveDesc.wallDetectDistance)
            continue;

       // Up과 너무 비슷하면 바닥 / 경사로 보고 wall에서 제외
        const float upDotAbs = fabsf(hitNormal.Dot(Vec3::Up));
        if (upDotAbs > _moveDesc.wallRunnableMaxUpDot)
            continue;

        if (!bestHit.isValid || hitDist < bestHit.hitDistance)
        {
            bestHit.hitModel = colModel;
            bestHit.hitPoint = hitPoint;
            bestHit.hitNormal = hitNormal;
            bestHit.hitDistance = hitDist;
            bestHit.isValid = true;
        }
    }

    outHit = bestHit;
    return bestHit.isValid;
}

bool MovementComponent::Can_EnterWallRun(const FSurfaceHit& wallHit, const Vec3& desiredMoveDir) const
{
    if (!wallHit.isValid)
        return false;

    Vec3 moveDir = desiredMoveDir;
    if (moveDir.LengthSquared() <= FLT_EPSILON)
    {
        moveDir = _velocity;
        moveDir.y = 0.f;
    }

    if (moveDir.LengthSquared() <= FLT_EPSILON)
        return false;

    moveDir.Normalize();

    const float intoWall = moveDir.Dot(-wallHit.hitNormal);

    return intoWall > 0.15f;
}

void MovementComponent::Enter_WallRun(const FSurfaceHit& wallHit)
{
    _isWallRunning = true;
    _currentWallNormal = wallHit.hitNormal;
    _currentWallHitPoint = wallHit.hitPoint;
    _onGround = false;

    // 벽에 붙는 순간 낙하 속도를 제거
    _velocity.y = 0.f;
}

void MovementComponent::Exit_WallRun()
{
    if (!_isWallRunning)
        return;

    Restore_DefaultUpRotation(_transform);

    _isWallRunning = false;
    _currentWallNormal = Vec3::Up;
    _currentWallHitPoint = Vec3::Zero;
}

void MovementComponent::Apply_WallRunPosition(Shared<Transform> transform, const FSurfaceHit& wallHit)
{
    if (!transform || !wallHit.isValid)
        return;

    Vec3 currentPos = _transform->Get_WorldPosition();

    const float signedDistance = (currentPos - wallHit.hitPoint).Dot(wallHit.hitNormal);
    const float correction = _moveDesc.wallAttachOffset - signedDistance;

    currentPos += wallHit.hitNormal * correction;
    transform->Set_WorldPosition(currentPos);
}

void MovementComponent::Apply_WallRunRotation(float timeDelta, Shared<Transform> transform)
{
    if (!transform)
        return;

    Vec3 forwardDir = Build_WallRunMoveDirection();

    // 입력이 없을 때는 현재 바라보는 방향을 유지
    if (forwardDir.LengthSquared() <= FLT_EPSILON)
    {
        forwardDir = Utils::Project_OnPlane(transform->Get_WorldForward(), _currentWallNormal);
        forwardDir = Utils::Safe_Normalize(forwardDir, Vec3::Forward);
    }

    // 캐릭터의 Up벡터를 wall normal로 맞춰서 발이 벽에 붙는 느낌 주도록
    Matrix lookAtMatrix = XMMatrixLookAtLH(Vec3::Zero, forwardDir, _currentWallNormal);
    lookAtMatrix = lookAtMatrix.Invert();

    Quat targetRot = Quat::CreateFromRotationMatrix(lookAtMatrix);
    Quat currentRot = transform->Get_WorldRotation();

    const float alpha = ::clamp(12.f * timeDelta, 0.f, 1.f);

    transform->Set_WorldRotation(Quat::Slerp(currentRot, targetRot, alpha));
}

void MovementComponent::Restore_DefaultUpRotation(Shared<Transform> transform)
{
    if (!transform)
        return;

    Vec3 flattedForward = _transform->Get_WorldForward();
    flattedForward.y = 0;

    if (flattedForward.LengthSquared() <= FLT_EPSILON)
    {
        flattedForward = Build_DesiredMoveDirection();

        if (flattedForward.LengthSquared() <= FLT_EPSILON)
            flattedForward = Vec3::Forward;
    }

    flattedForward.Normalize();

    // 월드 Up을 기준으로 다시 회전 쿼터니언을 만들어 pitch, roll 값을 제거
    Matrix lookAtMatrix = XMMatrixLookAtLH(Vec3::Zero, flattedForward, Vec3::Up);
    lookAtMatrix = lookAtMatrix.Invert();

    Quat targetRot = Quat::CreateFromRotationMatrix(lookAtMatrix);
    transform->Set_WorldRotation(targetRot);
}

Shared<MovementComponent> MovementComponent::Create(ComPtr<Device> device, ComPtr<DeviceContext> context)
{
    auto instance = make_shared<MovementComponent>(device, context);

    if (FAILED(instance->Initialize_Prototype()))
    {
        MSG_BOX("Failed to Created : MovementComponent");
        return nullptr;
    }

    return instance;
}

Shared<Component> MovementComponent::Clone(void* arg)
{
    auto clone = make_shared<MovementComponent>(*this);

    if (FAILED(clone->Initialize(arg)))
    {
        MSG_BOX("Failed to Cloned : MovementComponent");
        return nullptr;
    }

    return clone;
}

void MovementComponent::Free()
{
    Component::Free();
}
