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

    PROPERTY_FLOAT("Ground Walkable Min Up Dot", _moveDesc.groundWalkableMinUpDot, 0.f, 1.f);

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
    //dashDir.y = 0.f;

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

bool MovementComponent::Try_WireDash_WallTrace(
    const Vec3& traceStart,
    const Vec3& traceDir,
    FSurfaceHit& outHit) const
{
    Vec3 dir = Utils::Safe_Normalize(traceDir, Vec3::Forward);
    const Vec3 traceOrigin = traceStart + Vec3(0.f, _wireDashDesc.traceStartOffsetY, 0.f);
    const Ray wallRay(traceOrigin, dir);

    return Trace_WallSurface(wallRay, _wireDashDesc.maxDistance, outHit);
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

        if (abs(_dashWorldDirection.y) > 0.001f) 
        {
             _velocity.y = _dashWorldDirection.y * _dashSpeed;
        }
        else if (_gravityEnabled && !_onGround) 
        {
            _velocity.y += _moveDesc.gravity * timeDelta;
        }
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
    const Vec3 previousPos = transform->Get_WorldPosition();

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
        const bool foundGround = Detect_GroundSurface(currentPos, groundHit);

        if (foundGround && currentPos.y <= groundHit.hitPoint.y + _moveDesc.groundSnapTolerance)
        {
            currentPos.y = groundHit.hitPoint.y;
            transform->Set_WorldPosition(currentPos);
            _velocity.y = 0.f;
            _onGround = true;
            _canDoubleJump = false;
        }
        else if (!foundGround && currentPos.y <= _moveDesc.groundY)
        {
            currentPos.y = _moveDesc.groundY;
            transform->Set_WorldPosition(currentPos);
            _velocity.y = 0.f;
            _onGround = true;
            _canDoubleJump = false;
        }
        else
        {
            _onGround = false;
        }
    }
    else
    {
        _onGround = false;
    }

    if (!_isWallRunning && _velocity.y <= 0.f)
    {
        Apply_WallBlock(previousPos, transform);
        currentPos = transform->Get_WorldPosition();
    }

    Vec3 desiredDir = Build_DesiredMoveDirection();
    Vec3 horizontalVelocity = _velocity;
    horizontalVelocity.y = 0.f;

    if (_wallJumpCooldown <= 0.f &&
        !_isWallRunning &&
        !_onGround &&
        _commandDesc.moveAxis.Length() >= 0.2f &&
        horizontalVelocity.Length() >= 2.0f)
    {
        FSurfaceHit wallHit{};
        //if (Detect_WallEntrySurface(currentPos, desiredDir, wallHit) &&
        //    Can_EnterWallRun(wallHit, desiredDir))
        //{
        //    Enter_WallRun(wallHit);
        //    Apply_WallRunPosition(transform, wallHit);
        //    return;
        //}

        if (Detect_WallEntrySurface(currentPos, desiredDir, wallHit))
        {
            Enter_WallRun(wallHit);
            Apply_WallRunPosition(transform, wallHit);
            return;
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

    for (const auto& collision : _groundCollisionModels)
    {
        if (!collision.model)
            continue;

        if (collision.hasWorldBounds)
        {
            float boundsHitDist = 0.f;
            if (!downRay.Intersects(collision.worldBounds, boundsHitDist))
                continue;
        }

        float hitDist = 0.f;
        Vec3 hitPoint = Vec3::Zero;
        Vec3 hitNormal = Vec3::Up;

        if (!collision.model)
            continue;

        if (!collision.model->Raycast(downRay, collision.worldMatrix, hitDist, hitPoint, hitNormal))
            continue;

        Vec3 surfaceNormal = Utils::Safe_Normalize(hitNormal, Vec3::Up);

        if (surfaceNormal.Dot(Vec3::Up) < 0.f)
        {
            surfaceNormal = surfaceNormal * -1.f;
        }

        const float upDot = surfaceNormal.Dot(Vec3::Up);

        if (upDot < _moveDesc.groundWalkableMinUpDot)
            continue;

        if (hitPoint.y > bestHeight)
        {
            bestHeight = hitPoint.y;
            bestHit.hitModel = collision.model;
            bestHit.hitWorldMatrix = collision.worldMatrix;
            bestHit.hitPoint = hitPoint;
            bestHit.hitNormal = surfaceNormal;
            bestHit.hitDistance = hitDist;
            bestHit.isValid = true;
        }
    }

    outHit = bestHit;
    return bestHit.isValid;
}

bool MovementComponent::Detect_WallSurface(
    const Vec3& currentPos,
    const Vec3& castDir,
    FSurfaceHit& outHit) const
{
    Vec3 dir = castDir;
    if (dir.LengthSquared() <= FLT_EPSILON)
        dir = _transform ? _transform->Get_WorldForward() : Vec3::Forward;

    dir = Utils::Safe_Normalize(dir, Vec3::Forward);

    const Vec3 rayOrigin = currentPos + Vec3(0.f, _moveDesc.wallTraceStartOffsetY, 0.f);
    const Ray wallRay(rayOrigin, dir);

    return Trace_WallSurface(wallRay, _moveDesc.wallDetectDistance, outHit);
}

bool MovementComponent::Can_EnterWallRun(const FSurfaceHit& wallHit, const Vec3& desiredMoveDir) const
{
    if (!wallHit.isValid)
        return false;

    Vec3 moveDir = desiredMoveDir;
    moveDir.y = 0.f;

    if (moveDir.LengthSquared() <= FLT_EPSILON)
        return false;

    moveDir.Normalize();
    return moveDir.Dot(-wallHit.hitNormal) > 0.15f;
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

bool MovementComponent::Detect_WallEntrySurface(
    const Vec3& currentPos,
    const Vec3& desiredDir,
    FSurfaceHit& outHit) const
{
    Vec3 baseDir = desiredDir;
    baseDir.y = 0.f;

    if (baseDir.LengthSquared() <= FLT_EPSILON)
    {
        outHit = FSurfaceHit{};
        return false;
    }

    baseDir = Utils::Safe_Normalize(baseDir, Vec3::Forward);

    const Vec3 rayOrigin = currentPos + Vec3(0.f, _moveDesc.wallTraceStartOffsetY, 0.f);
    const Ray rays[3] =
    {
        Ray(rayOrigin, baseDir),
        Ray(rayOrigin, Rotate_HorizontalDirection(baseDir, -15.f)),
        Ray(rayOrigin, Rotate_HorizontalDirection(baseDir, 15.f))
    };

    FSurfaceHit bestHit{};
    for (const Ray& ray : rays)
    {
        FSurfaceHit hit{};
        if (!Trace_WallSurface(ray, _moveDesc.wallDetectDistance, hit))
            continue;

        if (!bestHit.isValid || hit.hitDistance < bestHit.hitDistance)
            bestHit = hit;
    }

    outHit = bestHit;
    return bestHit.isValid;
}


bool MovementComponent::Trace_WallSurface(
    const Ray& wallRay,
    float maxDistance,
    FSurfaceHit& outHit) const
{
    FSurfaceHit bestHit{};

    for (const auto& collision : _wallCollisionModels)
    {
        if (!collision.model)
            continue;

        if (collision.hasWorldBounds)
        {
            float boundsHitDist = 0.f;
            if (!wallRay.Intersects(collision.worldBounds, boundsHitDist))
                continue;

            if (boundsHitDist > maxDistance)
                continue;
        }

        float hitDist = 0.f;
        Vec3 hitPoint = Vec3::Zero;
        Vec3 hitNormal = Vec3::Up;

        if (!collision.model)
            continue;

        if (!collision.model->Raycast(wallRay, collision.worldMatrix, hitDist, hitPoint, hitNormal))
            continue;

        if (hitDist > maxDistance)
            continue;

        const float upDotAbs = fabsf(hitNormal.Dot(Vec3::Up));
        if (upDotAbs > _moveDesc.wallRunnableMaxUpDot)
            continue;

        if (!bestHit.isValid || hitDist < bestHit.hitDistance)
        {
            bestHit.hitModel = collision.model;
            bestHit.hitWorldMatrix = collision.worldMatrix;
            bestHit.hitPoint = hitPoint;
            bestHit.hitNormal = hitNormal;
            bestHit.hitDistance = hitDist;
            bestHit.isValid = true;
        }
    }

    outHit = bestHit;
    return bestHit.isValid;
}

Vec3 MovementComponent::Rotate_HorizontalDirection(const Vec3& dir, float degrees)
{
    Vec3 horizontalDir = dir;
    horizontalDir.y = 0.f;
    horizontalDir = Utils::Safe_Normalize(horizontalDir, Vec3::Forward);

    const Matrix rotationMatrix = Matrix::CreateRotationY(XMConvertToRadians(degrees));
    Vec3 rotatedDir = Vec3::TransformNormal(horizontalDir, rotationMatrix);
    rotatedDir.y = 0.f;

    return Utils::Safe_Normalize(rotatedDir, horizontalDir);
}

void MovementComponent::Apply_WallBlock(const Vec3& previousPos, Shared<Transform> transform)
{
     if (!transform)
        return;

    Vec3 currentPos = transform->Get_WorldPosition();
    Vec3 moveDelta = currentPos - previousPos;
    moveDelta.y = 0.f;

    if (moveDelta.LengthSquared() <= FLT_EPSILON)
        return;

    Vec3 moveDir = Utils::Safe_Normalize(moveDelta, Vec3::Forward);

    FSurfaceHit wallHit{};
    if (!Detect_WallSurface(previousPos, moveDir, wallHit))
        return;

    // 벽 쪽으로 실제로 파고드는 이동일 때만 막는다.
    const float intoWall = moveDelta.Dot(-wallHit.hitNormal);
    if (intoWall <= 0.f)
        return;

    Vec3 correctedDelta = moveDelta - (-wallHit.hitNormal) * intoWall;
    Vec3 correctedPos = previousPos + correctedDelta;
    correctedPos.y = currentPos.y;

    transform->Set_WorldPosition(correctedPos);
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
