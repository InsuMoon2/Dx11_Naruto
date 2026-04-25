#include "pch.h"
#include "MovementComponent.h"

#include "Camera.h"
#include "Debug_Manager.h"
#include "GameObject.h"
#include "Transform.h"
#include "Model.h"
#include "Collider.h"
#include "Bounding_AABB.h"
#include "Bounding_Capsule.h"
#include "Bounding_OBB.h"
#include "Bounding_Sphere.h"
#include "Collision_Define.h"
#include "GameInstance.h"

IMPLEMENT_REFLECTION(MovementComponent)

// 현재 캐릭터 몸통 충돌체 기준으로 벽에서 유지해야 할 최소 수평 여유 거리를 계산한다.
static float Calculate_BodyWallClearance(const Shared<Collider>& collider, float fallbackClearance)
{
    if (!collider)
        return fallbackClearance;

    const Shared<Bounding> bounding = collider->Get_Bounding();
    if (!bounding)
        return fallbackClearance;

    switch (collider->Get_Shape())
    {
    case EShape::OBB:
        {
            auto obbBounding = static_pointer_cast<Bounding_OBB>(bounding);
            const Vec3 extents = obbBounding->Get_OBB().Extents;
            const float horizontalExtent = max(extents.x, extents.z);
            return max(horizontalExtent + 0.03f, fallbackClearance);
        }

    case EShape::Capsule:
        {
            auto capsuleBounding = static_pointer_cast<Bounding_Capsule>(bounding);
            return max(capsuleBounding->Get_OriginRadius() + 0.03f, fallbackClearance);
        }

    default:
        break;
    }

    return fallbackClearance;
}

bool MovementComponent::Register_Properties()
{
    auto& info = GetStaticReflectionInfo();
    info.className = "MovementComponent";

    PROPERTY_FLOAT("Max Walk Speed", _moveDesc.maxWalkSpeed, 0.f, 30.f);
    PROPERTY_FLOAT("Max Sprint Speed", _moveDesc.maxSprintSpeed, 0.f, 30.f);
    PROPERTY_FLOAT("Acceleration", _moveDesc.acceleration, 0.f, 50.f);
    PROPERTY_FLOAT("Deceleration", _moveDesc.deceleration, 0.f, 50.f);
    PROPERTY_FLOAT("Yaw Speed", _moveDesc.yawSpeed, 0.f, 1080.f);
    PROPERTY_FLOAT("Launch Control Lock Duration", _moveDesc.launchControlLockDuration, 0.f, 1.f);
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
    PROPERTY_FLOAT("Wall Top Landing Max Height Delta", _moveDesc.wallTopLandingMaxHeightDelta, 0.05f, 2.f);
    PROPERTY_FLOAT("Wall Top Landing Forward Offset", _moveDesc.wallTopLandingForwardOffset, 0.f, 1.f);

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
    , _wallRunLostContactElapsed(rhs._wallRunLostContactElapsed)
    , _launchControlLockRemaining(rhs._launchControlLockRemaining)
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
    _wallRunLostContactElapsed = 0.f;
    _launchControlLockRemaining = 0.f;

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
    Resolve_CharacterBodyPenetration();

    if (_wallJumpCooldown > 0.f)
        _wallJumpCooldown -= timeDelta;

    if (_launchControlLockRemaining > 0.f)
        _launchControlLockRemaining -= timeDelta;

    if (!_isWallRunning)
    {
        Update_Rotation(timeDelta, _transform);
    }

    Update_Velocity(timeDelta, _transform);
    Apply_Movement(timeDelta, _transform);
    Resolve_CharacterBodyPenetration();

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

bool MovementComponent::Try_RecoverWallRunHold()
{
    if (!_transform || _onGround)
        return false;

    Vec3 wallNormal = Utils::Safe_Normalize(_currentWallNormal, Vec3::Zero);
    if (wallNormal.LengthSquared() <= FLT_EPSILON)
        return false;

    const Vec3 currentPos = _transform->Get_WorldPosition();

    FSurfaceHit wallHit{};
    if (!Detect_WallAhead(currentPos, -wallNormal, wallHit))
        return false;

    if (!wallHit.isValid || !Is_WallLikeNormal(wallHit.hitNormal))
        return false;

    // 벽 이동 복구는 이전에 붙어 있던 벽의 앞면과 비슷한 경우만 허용해서
    // 코너를 타고 반대편 벽 뒤에서 다시 붙는 상황을 막는다.
    if (wallHit.hitNormal.Dot(wallNormal) < 0.55f)
        return false;

    Enter_WallRun(wallHit);
    if (!Resolve_WallRunContact(_transform, currentPos, wallHit))
    {
        Exit_WallRun();
        return false;
    }

    // 막힌 구간에 다시 붙는 목적이므로 속도는 비워서 즉시 정지한 wall idle 상태를 만들게 한다.
    _velocity = Vec3::Zero;
    _wallRunLostContactElapsed = 0.f;

    return true;
}

void MovementComponent::Set_Velocity(Vec3 velocity)
{
    _velocity = velocity;


}

void MovementComponent::Launch(const Vec3& launchVelocity, bool xyOverride, bool zOverride)
{
    Resolve_CharacterBodyPenetration();

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

    Vec3 horizontalLaunch = launchVelocity;
    horizontalLaunch.y = 0.f;
    if (horizontalLaunch.LengthSquared() > FLT_EPSILON)
    {
        // 피격 launch를 넣은 바로 다음 프레임에 일반 이동 가속/감속이 덮어써 버리면
        // 뒤로 밀리는 체감이 거의 사라지므로, 짧은 시간 동안 입력 보정을 잠깐 멈춘다.
        _launchControlLockRemaining = _moveDesc.launchControlLockDuration;
    }
}

// 이미 몸통끼리 겹친 상태에서 previous position 기반 Block만으로는 빠져나오지 못하므로,
// 현재 충돌체 위치를 기준으로 수평 분리 벡터를 직접 계산해 밀어낸다.
void MovementComponent::Resolve_CharacterBodyPenetration()
{
    if (!_transform)
        return;

    auto owner = _owner.lock();
    if (!owner)
        return;

    auto selfCollider = owner->Get_Component<Collider>();
    if (!selfCollider || !selfCollider->Get_IsActive())
        return;

    if (!Is_CharacterBodyChannel(selfCollider->Get_Channel()))
        return;

    selfCollider->Update_Collider(_transform->Get_WorldMatrix());

    FCharacterBodySeparationInfo selfInfo{};
    if (!Try_BuildCharacterBodySeparationInfo(selfCollider, selfInfo))
        return;

    const vector<Shared<GameObject>> gameObjects = GAME->Get_GameObjects(owner->Get_LevelIndex());
    Vec3 totalPush = Vec3::Zero;

    for (const auto& otherObject : gameObjects)
    {
        if (!otherObject || otherObject.get() == owner.get())
            continue;

        auto otherCollider = otherObject->Get_Component<Collider>();
        if (!otherCollider || !otherCollider->Get_IsActive())
            continue;

        if (!Is_CharacterBodyChannel(otherCollider->Get_Channel()))
            continue;

        const ECollisionResponse response = Calculate_ResponseResult(
            selfCollider->Get_Channel(),
            selfCollider->Get_OverlapMask(),
            selfCollider->Get_BlockMask(),
            otherCollider->Get_Channel(),
            otherCollider->Get_OverlapMask(),
            otherCollider->Get_BlockMask());

        if (response != ECollisionResponse::Block)
            continue;

        if (auto otherTransform = otherObject->Get_Transform())
            otherCollider->Update_Collider(otherTransform->Get_WorldMatrix());

        if (!selfCollider->Intersect(otherCollider))
            continue;

        FCharacterBodySeparationInfo otherInfo{};
        if (!Try_BuildCharacterBodySeparationInfo(otherCollider, otherInfo))
            continue;

        Vec3 toSelf = selfInfo.center - otherInfo.center;
        toSelf.y = 0.f;

        const float distance = toSelf.Length();
        const float minimumDistance = selfInfo.horizontalRadius + otherInfo.horizontalRadius + 0.02f;

        if (distance >= minimumDistance)
            continue;

        Vec3 pushDir = Vec3::Zero;
        if (distance > 0.0001f)
        {
            pushDir = toSelf / distance;
        }
        else
        {
            pushDir = Build_CharacterBodyFallbackPushDirection(otherObject);
        }

        const float penetrationDepth = minimumDistance - distance;
        totalPush += pushDir * penetrationDepth;
    }

    totalPush.y = 0.f;
    if (totalPush.LengthSquared() <= FLT_EPSILON)
        return;

    const float maxPushStep = 0.75f;
    if (totalPush.Length() > maxPushStep)
    {
        totalPush.Normalize();
        totalPush *= maxPushStep;
    }

    _transform->Add_WorldOffset(totalPush);
    selfCollider->Update_Collider(_transform->Get_WorldMatrix());
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

    FPhysXRaycastHit physXHit{};
    const bool isHit = GAME->Raycast_PhysX(
        traceStart,
        dir,
        _wireDashDesc.maxDistance,
        physXHit,
        ECollisionProxyType::WallRun);

    FSurfaceHit hit{};
    if (isHit)
    {
        Vec3 hitNormal = Utils::Safe_Normalize(physXHit.normal, Vec3::Up);
        if (dir.Dot(hitNormal) > 0.f)
            hitNormal = -hitNormal;

        hit.hitPoint = physXHit.position;
        hit.hitNormal = hitNormal;
        hit.hitDistance = physXHit.distance;
        hit.isValid = true;
    }

    if (!isHit)
    {
        outHit = FSurfaceHit{};
        return false;
    }

    if (!Is_WallLikeNormal(hit.hitNormal))
    {
        outHit = FSurfaceHit{};
        return false;
    }

    outHit = hit;
    return true;
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

    if (_launchControlLockRemaining > 0.f)
    {
        if (_gravityEnabled && !_onGround)
            _velocity.y += _moveDesc.gravity * timeDelta;

        return;
    }

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
    if (!transform)
        return;

    const Vec3 totalDelta = _velocity * timeDelta;
    const float totalDistance = totalDelta.Length();
    const float maxStepDistance = _isWallRunning ? 0.08f : 0.18f;
    const int32 subStepCount = max(1, static_cast<int32>(ceilf(totalDistance / maxStepDistance)));
    const Vec3 stepDelta = totalDelta / static_cast<float>(subStepCount);

    Vec3 currentPos = transform->Get_WorldPosition();

    for (int32 stepIndex = 0; stepIndex < subStepCount; ++stepIndex)
    {
        const Vec3 previousPos = transform->Get_WorldPosition();

        transform->Add_WorldOffset(stepDelta);
        Apply_CharacterBodyBlock(previousPos, transform);

        currentPos = transform->Get_WorldPosition();

        if (_isWallRunning)
        {
            const float wallRunLostContactGrace = 0.25f;
            const float wallRunStepDelta = timeDelta / static_cast<float>(subStepCount);

            // 벽타기 중 먼저 상단 착지 가능 여부를 본다.
            FSurfaceHit topHit{};
            if (Detect_TopLanding(currentPos, _currentWallNormal, topHit))
            {
                Apply_WallTopLanding(transform, topHit);
                return;
            }

            // 상단이 아니면 현재 붙어 있는 벽을 계속 유지할 수 있는지 본다.
            FSurfaceHit wallHit{};
            if (Detect_WallAhead(currentPos, -_currentWallNormal, wallHit))
            {
                _currentWallNormal = wallHit.hitNormal;
                _currentWallHitPoint = wallHit.hitPoint;
                _wallRunLostContactElapsed = 0.f;
                if (Resolve_WallRunContact(transform, previousPos, wallHit))
                {
                    currentPos = transform->Get_WorldPosition();
                    continue;
                }

                transform->Set_WorldPosition(previousPos);
                Exit_WallRun();
                return;
            }

            _wallRunLostContactElapsed += wallRunStepDelta;
            if (_wallRunLostContactElapsed < wallRunLostContactGrace)
            {
                FSurfaceHit fallbackHit{};
                fallbackHit.hitNormal = _currentWallNormal;
                fallbackHit.hitPoint = _currentWallHitPoint;
                fallbackHit.isValid = true;

                if (Resolve_WallRunContact(transform, previousPos, fallbackHit))
                {
                    currentPos = transform->Get_WorldPosition();
                    continue;
                }
            }

            transform->Set_WorldPosition(previousPos);
            Exit_WallRun();
            return;
        }

        if (_velocity.y <= 0.f)
        {
            FSurfaceHit groundHit{};
            const bool foundGround = Detect_FloorBelow(currentPos, groundHit);

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

        if (!_isWallRunning)
        {
            Apply_CeilingBlock(previousPos, transform);
            Apply_WallBlock(previousPos, transform);
            currentPos = transform->Get_WorldPosition();
        }
    }

    Vec3 desiredDir = Build_DesiredMoveDirection();
    Vec3 horizontalVelocity = _velocity;
    horizontalVelocity.y = 0.f;

    Vec3 wallProbeDir = desiredDir;
    if (wallProbeDir.LengthSquared() <= FLT_EPSILON && horizontalVelocity.LengthSquared() > FLT_EPSILON)
        wallProbeDir = horizontalVelocity;

    const bool hasWallProbeInput = _commandDesc.moveAxis.Length() >= 0.2f;
    const bool hasWallProbeVelocity = horizontalVelocity.Length() >= 1.0f;
    Vec3 facingWallProbeDir = transform->Get_WorldForward(); // 이동 입력 없이 벽을 바라보고 점프한 경우 정면 벽을 탐지하기 위한 후보 방향이다.
    facingWallProbeDir.y = 0.f;
    const bool hasWallProbeFacing = _velocity.y > 0.f && facingWallProbeDir.LengthSquared() > FLT_EPSILON; // 상승 점프 중일 때만 무입력 정면 벽 붙기를 허용한다.

    if (wallProbeDir.LengthSquared() <= FLT_EPSILON && hasWallProbeFacing)
        wallProbeDir = facingWallProbeDir;

    if (_wallJumpCooldown <= 0.f &&
        !_isWallRunning &&
        !_onGround &&
        (hasWallProbeInput || hasWallProbeVelocity || hasWallProbeFacing) &&
        wallProbeDir.LengthSquared() > FLT_EPSILON)
    {
        wallProbeDir = Utils::Safe_Normalize(wallProbeDir, Vec3::Forward);

        FSurfaceHit wallHit{};
        if (Detect_WallAhead(currentPos, wallProbeDir, wallHit) &&
            Can_EnterWallRun(wallHit, wallProbeDir))
        {
            const Vec3 beforeAttachPos = transform->Get_WorldPosition(); // 벽 부착 검증에 실패하면 되돌릴 진입 직전 위치다.

            Enter_WallRun(wallHit);

            if (!Resolve_WallRunContact(transform, beforeAttachPos, wallHit))
            {
                // 최초 진입은 Detect_WallAhead가 잡은 표면을 신뢰해서 벽타기 상태를 살리고, 다음 tick부터 유지 검증을 맡긴다.
                Apply_WallRunPosition(transform, wallHit);
                _currentWallNormal = wallHit.hitNormal;
                _currentWallHitPoint = wallHit.hitPoint;
            }

            return;
        }
    }
}

bool MovementComponent::Is_CharacterBodyChannel(Collision_Channel channel)
{
    return channel == Collision_Channel::Player_Body
        || channel == Collision_Channel::Monster_Body;
}

bool MovementComponent::Is_BlockedByCharacterBody(const Vec3& testPosition, Shared<Transform> transform) const
{
    if (!transform)
        return false;

    auto owner = _owner.lock();
    if (!owner)
        return false;

    auto selfCollider = owner->Get_Component<Collider>();
    if (!selfCollider || !selfCollider->Get_IsActive())
        return false;

    if (!Is_CharacterBodyChannel(selfCollider->Get_Channel()))
        return false;

    const Vec3 originalPos = transform->Get_WorldPosition();

    transform->Set_WorldPosition(testPosition);
    selfCollider->Update_Collider(transform->Get_WorldMatrix());

    bool isBlocked = false;
    const vector<Shared<GameObject>> gameObjects = GAME->Get_GameObjects(owner->Get_LevelIndex());

    for (const auto& otherObject : gameObjects)
    {
        if (!otherObject || otherObject.get() == owner.get())
            continue;

        auto otherCollider = otherObject->Get_Component<Collider>();
        if (!otherCollider || !otherCollider->Get_IsActive())
            continue;

        if (!Is_CharacterBodyChannel(otherCollider->Get_Channel()))
            continue;

        const ECollisionResponse response = Calculate_ResponseResult(
            selfCollider->Get_Channel(),
            selfCollider->Get_OverlapMask(),
            selfCollider->Get_BlockMask(),
            otherCollider->Get_Channel(),
            otherCollider->Get_OverlapMask(),
            otherCollider->Get_BlockMask());

        if (response != ECollisionResponse::Block)
            continue;

        if (auto otherTransform = otherObject->Get_Transform())
            otherCollider->Update_Collider(otherTransform->Get_WorldMatrix());

        if (selfCollider->Intersect(otherCollider))
        {
            isBlocked = true;
            break;
        }
    }

    transform->Set_WorldPosition(originalPos);
    selfCollider->Update_Collider(transform->Get_WorldMatrix());

    return isBlocked;
}

void MovementComponent::Apply_CharacterBodyBlock(const Vec3& previousPos, Shared<Transform> transform)
{
    if (!transform)
        return;

    const Vec3 currentPos = transform->Get_WorldPosition();

    Vec3 moveDelta = currentPos - previousPos;
    moveDelta.y = 0.f;

    if (moveDelta.LengthSquared() <= FLT_EPSILON)
        return;

    if (!Is_BlockedByCharacterBody(currentPos, transform))
        return;

    Vec3 resolvedPos = previousPos;

    const Vec3 tryPosX = resolvedPos + Vec3(moveDelta.x, 0.f, 0.f);
    if (!Is_BlockedByCharacterBody(tryPosX, transform))
        resolvedPos.x = tryPosX.x;

    const Vec3 tryPosZ = resolvedPos + Vec3(0.f, 0.f, moveDelta.z);
    if (!Is_BlockedByCharacterBody(tryPosZ, transform))
        resolvedPos.z = tryPosZ.z;

    resolvedPos.y = currentPos.y;
    transform->Set_WorldPosition(resolvedPos);

    auto owner = _owner.lock();
    if (!owner)
        return;

    auto selfCollider = owner->Get_Component<Collider>();
    if (selfCollider)
        selfCollider->Update_Collider(transform->Get_WorldMatrix());
}

// 다양한 몸통 collider 모양을 수평 분리 계산용 원형 프록시로 변환한다.
bool MovementComponent::Try_BuildCharacterBodySeparationInfo(
    const Shared<Collider>& collider,
    FCharacterBodySeparationInfo& outInfo) const
{
    if (!collider)
        return false;

    const Shared<Bounding> bounding = collider->Get_Bounding();
    if (!bounding)
        return false;

    switch (collider->Get_Shape())
    {
    case EShape::AABB:
        {
            auto aabbBounding = static_pointer_cast<Bounding_AABB>(bounding);
            const BoundingBox& aabb = aabbBounding->Get_AABB();

            outInfo.center = aabb.Center;
            outInfo.horizontalRadius = max(aabb.Extents.x, aabb.Extents.z);
            return outInfo.horizontalRadius > FLT_EPSILON;
        }

    case EShape::OBB:
        {
            auto obbBounding = static_pointer_cast<Bounding_OBB>(bounding);
            const BoundingOrientedBox& obb = obbBounding->Get_OBB();

            outInfo.center = obb.Center;
            outInfo.horizontalRadius = max(obb.Extents.x, obb.Extents.z);
            return outInfo.horizontalRadius > FLT_EPSILON;
        }

    case EShape::Sphere:
        {
            auto sphereBounding = static_pointer_cast<Bounding_Sphere>(bounding);
            const BoundingSphere& sphere = sphereBounding->Get_Sphere();

            outInfo.center = sphere.Center;
            outInfo.horizontalRadius = sphere.Radius;
            return outInfo.horizontalRadius > FLT_EPSILON;
        }

    case EShape::Capsule:
        {
            auto capsuleBounding = static_pointer_cast<Bounding_Capsule>(bounding);
            const BoundingOrientedBox proxy = capsuleBounding->Get_ProxyOBB();

            outInfo.center = proxy.Center;
            outInfo.horizontalRadius = max(proxy.Extents.x, proxy.Extents.z);
            return outInfo.horizontalRadius > FLT_EPSILON;
        }

    default:
        break;
    }

    return false;
}

// 두 몸통 중심이 거의 같아 normal을 만들 수 없을 때 안정적인 fallback 밀림 방향을 만든다.
Vec3 MovementComponent::Build_CharacterBodyFallbackPushDirection(const Shared<GameObject>& otherObject) const
{
    Vec3 fallbackDir = Vec3::Zero;

    if (_transform && otherObject)
    {
        if (auto otherTransform = otherObject->Get_Transform())
        {
            fallbackDir = _transform->Get_WorldPosition() - otherTransform->Get_WorldPosition();
            fallbackDir.y = 0.f;
        }
    }

    if (fallbackDir.LengthSquared() <= FLT_EPSILON && _transform)
    {
        fallbackDir = -_transform->Get_WorldForward();
        fallbackDir.y = 0.f;
    }

    return Utils::Safe_Normalize(fallbackDir, Vec3::Forward);
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

bool MovementComponent::Detect_FloorBelow(const Vec3& currentPos, FSurfaceHit& outHit) const
{
    const Vec3 rayStart = currentPos + Vec3(0.f, _moveDesc.groundTraceStartOffsetY, 0.f);
    const float traceDistance = _moveDesc.groundTraceStartOffsetY + 2.0f;
    const Vec3 rayEnd = rayStart + Vec3(0.f, -1.f, 0.f) * traceDistance;

    FPhysXRaycastHit physXHit{};
    const bool isHit = GAME->Raycast_PhysX(
        rayStart,
        Vec3(0.f, -1.f, 0.f),
        traceDistance,
        physXHit);

    FSurfaceHit hit{};
    if (isHit)
    {
        hit.hitPoint = physXHit.position;
        hit.hitNormal = Utils::Safe_Normalize(physXHit.normal, Vec3::Up);
        hit.hitDistance = physXHit.distance;
        hit.isValid = true;
    }

    Draw_TraceDebug(rayStart, rayEnd, hit);

    if (!isHit)
    {
        outHit = FSurfaceHit{};
        return false;
    }

    if (!Is_GroundLikeNormal(hit.hitNormal))
    {
        outHit = FSurfaceHit{};
        return false;
    }

    outHit = hit;
    return true;
}

bool MovementComponent::Detect_WallAhead(const Vec3& currentPos, const Vec3& desiredDir, FSurfaceHit& outHit) const
{
    Vec3 baseDir = desiredDir;
    baseDir.y = 0.f;

    if (baseDir.LengthSquared() <= FLT_EPSILON)
    {
        outHit = FSurfaceHit{};
        return false;
    }

    baseDir = Utils::Safe_Normalize(baseDir, Vec3::Forward);

    const float traceDistance = _moveDesc.wallDetectDistance + 0.15f;
    const float verticalOffsets[3] = { 0.45f, 0.f, -0.45f };
    const float backStartOffsets[2] = { 0.f, 0.35f }; // 벽에 너무 붙은 상태에서 ray 시작점이 면을 지나친 경우를 보정하기 위해 살짝 뒤에서도 쏜다.

    FSurfaceHit bestHit{};

    for (int32 verticalIndex = 0; verticalIndex < 3; ++verticalIndex)
    {
        const float verticalOffset = verticalOffsets[verticalIndex];
        for (float backStartOffset : backStartOffsets)
        {
            const Vec3 rayOrigin =
                currentPos +
                Vec3(0.f, _moveDesc.wallTraceStartOffsetY + verticalOffset, 0.f) -
                baseDir * backStartOffset;
            const float currentTraceDistance = traceDistance + backStartOffset;
            const Vec3 rayEnd = rayOrigin + baseDir * currentTraceDistance;

            FPhysXRaycastHit physXHit{};
            bool isHit = GAME->Raycast_PhysX(
                rayOrigin,
                baseDir,
                currentTraceDistance,
                physXHit,
                ECollisionProxyType::WallRun);

            if (!isHit)
            {
                isHit = GAME->Raycast_PhysX(
                    rayOrigin,
                    baseDir,
                    currentTraceDistance,
                    physXHit,
                    ECollisionProxyType::WorldBlock);
            }

            FSurfaceHit hit{};
            if (isHit)
            {
                Vec3 hitNormal = Utils::Safe_Normalize(physXHit.normal, Vec3::Up);

                // 일부 proxy normal/backface가 반대로 들어와도, 벽 진입 판정에서는 캐릭터를 향하는 normal로 정리한다.
                if (baseDir.Dot(hitNormal) > -0.1f)
                    hitNormal = -hitNormal;

                if (baseDir.Dot(hitNormal) > -0.1f)
                {
                    Draw_TraceDebug(rayOrigin, rayEnd, hit);
                    continue;
                }

                hit.hitPoint = physXHit.position;
                hit.hitNormal = hitNormal;
                hit.hitDistance = physXHit.distance;
                hit.isValid = true;
            }

            Draw_TraceDebug(rayOrigin, rayEnd, hit);

            if (!isHit)
                continue;

            if (!Is_WallLikeNormal(hit.hitNormal))
                continue;

            if (!bestHit.isValid || hit.hitDistance < bestHit.hitDistance)
                bestHit = hit;
        }
    }

    outHit = bestHit;
    return bestHit.isValid;
}

bool MovementComponent::Detect_TopLanding(const Vec3& currentPos, const Vec3& wallNormal, FSurfaceHit& outHit) const
{
    const float forwardSample = 0.50f;
    const float sideSamples[3] = { 0.f, -0.18f, 0.18f };
    const float traceDistance = 2.2f;

    Vec3 wallForward = -Utils::Safe_Normalize(wallNormal, Vec3::Forward);
    Vec3 wallSide = wallForward.Cross(Vec3::Up);
    if (wallSide.LengthSquared() <= FLT_EPSILON)
        wallSide = Vec3::Right;
    else
        wallSide.Normalize();

    FSurfaceHit bestHit{};

    for (float sideSample : sideSamples)
    {
        const Vec3 rayStart =
            currentPos +
            wallForward * forwardSample +
            wallSide * sideSample +
            Vec3(0.f, 1.4f, 0.f);
        const Vec3 rayEnd = rayStart + Vec3(0.f, -1.f, 0.f) * traceDistance;

        FPhysXRaycastHit physXHit{};
        const bool isHit = GAME->Raycast_PhysX(
            rayStart,
            Vec3(0.f, -1.f, 0.f),
            traceDistance,
            physXHit);

        FSurfaceHit hit{};
        if (isHit)
        {
            hit.hitPoint = physXHit.position;
            hit.hitNormal = Utils::Safe_Normalize(physXHit.normal, Vec3::Up);
            hit.hitDistance = physXHit.distance;
            hit.isValid = true;
        }

        Draw_TraceDebug(rayStart, rayEnd, hit);

        if (!isHit)
            continue;

        if (!Is_GroundLikeNormal(hit.hitNormal))
            continue;

        const float topHeightDelta = hit.hitPoint.y - currentPos.y;
        if (topHeightDelta < -_moveDesc.groundSnapTolerance)
            continue;

        if (topHeightDelta > _moveDesc.wallTopLandingMaxHeightDelta)
            continue;

        if (!bestHit.isValid || hit.hitDistance < bestHit.hitDistance)
            bestHit = hit;
    }

    outHit = bestHit;
    return bestHit.isValid;
}

bool MovementComponent::Can_EnterWallRun(const FSurfaceHit& wallHit, const Vec3& desiredMoveDir) const
{
    if (!wallHit.isValid)
        return false;

    if (!Is_WallLikeNormal(wallHit.hitNormal))
        return false;

    Vec3 moveDir = desiredMoveDir;
    moveDir.y = 0.f;

    if (moveDir.LengthSquared() <= FLT_EPSILON)
        return false;

    moveDir.Normalize();
    return moveDir.Dot(-wallHit.hitNormal) > 0.08f;
}

void MovementComponent::Enter_WallRun(const FSurfaceHit& wallHit)
{
    _isWallRunning = true;
    _currentWallNormal = Utils::Safe_Normalize(wallHit.hitNormal, Vec3::Up);
    _currentWallHitPoint = wallHit.hitPoint;
    _wallRunLostContactElapsed = 0.f;
    _onGround = false;

    // 벽에 붙는 순간 벽 안쪽으로 밀고 들어가는 속도와 낙하 속도를 제거한다.
    _velocity = Utils::Project_OnPlane(_velocity, _currentWallNormal);
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
    _wallRunLostContactElapsed = 0.f;
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

bool MovementComponent::Resolve_WallRunContact(Shared<Transform> transform, const Vec3& previousPos, const FSurfaceHit& wallHit)
{
    if (!transform || !wallHit.isValid)
        return false;

    // 첫 번째 보정은 현재 감지된 벽 기준 attach 거리로 맞춘다.
    Apply_WallRunPosition(transform, wallHit);
    Apply_WallBlock(previousPos, transform);

    FSurfaceHit verifiedHit{};
    if (!Trace_WallRunSurface(transform->Get_WorldPosition(), wallHit.hitNormal, verifiedHit))
        return false;

    // 코너나 경사면에서 다른 표면을 집는 경우를 줄이기 위해 법선 방향이 너무 다르면 실패로 본다.
    const Vec3 requestedNormal = Utils::Safe_Normalize(wallHit.hitNormal, Vec3::Up);
    const Vec3 verifiedNormal = Utils::Safe_Normalize(verifiedHit.hitNormal, Vec3::Up);
    if (requestedNormal.Dot(verifiedNormal) < 0.15f)
        return false;

    Apply_WallRunPosition(transform, verifiedHit);
    _currentWallNormal = verifiedHit.hitNormal;
    _currentWallHitPoint = verifiedHit.hitPoint;

    return true;
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

void MovementComponent::Apply_WallTopLanding(Shared<Transform> transform, const FSurfaceHit& topHit)
{
    if (!transform || !topHit.isValid)
        return;

    Vec3 landingPos = transform->Get_WorldPosition();
    const Vec3 landingForward = -Utils::Safe_Normalize(_currentWallNormal, Vec3::Forward);

    landingPos += landingForward * _moveDesc.wallTopLandingForwardOffset;
    landingPos.y = topHit.hitPoint.y;
    transform->Set_WorldPosition(landingPos);

    _velocity.y = 0.f;
    _onGround = true;
    _canDoubleJump = false;
    Exit_WallRun();
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

void MovementComponent::Apply_NotifyMotionDelta(const Vec3& worldDelta, bool constrainToGround)
{
    if (!_transform)
        return;

    if (worldDelta.LengthSquared() <= FLT_EPSILON)
        return;

    const float totalDistance = worldDelta.Length();
    const float maxStepDistance = constrainToGround ? 0.08f : 0.12f;
    const int32 subStepCount = max(1, static_cast<int32>(ceilf(totalDistance / maxStepDistance)));
    const Vec3 stepDelta = worldDelta / static_cast<float>(subStepCount);

    for (int32 i = 0; i < subStepCount; ++i)
    {
        if (!Apply_NotifyMotionStep(stepDelta, constrainToGround))
            break;
    }
}

bool MovementComponent::Apply_NotifyMotionStep(const Vec3& stepDelta, bool constrainToGround)
{
    if (!_transform)
        return false;

    const Vec3 previousPos = _transform->Get_WorldPosition();
    const Vec3 resolvedStepDelta = Build_NotifyMotionStepDelta(stepDelta, constrainToGround);
    Vec3 requestedHorizontalDelta = resolvedStepDelta;
    requestedHorizontalDelta.y = 0.f;

    if (resolvedStepDelta.LengthSquared() <= FLT_EPSILON)
        return true;

    _transform->Add_WorldOffset(resolvedStepDelta);

    Apply_CharacterBodyBlock(previousPos, _transform);

    if (!_isWallRunning)
    {
        Apply_CeilingBlock(previousPos, _transform);
        Apply_WallBlock(previousPos, _transform);
    }

    const Vec3 currentPos = _transform->Get_WorldPosition();
    Vec3 actualHorizontalDelta = currentPos - previousPos;
    actualHorizontalDelta.y = 0.f;

    if (requestedHorizontalDelta.LengthSquared() > FLT_EPSILON)
    {
        const Vec3 requestDir = Utils::Safe_Normalize(requestedHorizontalDelta, Vec3::Forward);
        const float requestedDistance = requestedHorizontalDelta.Length();
        const float progressedDistance = max(0.f, actualHorizontalDelta.Dot(requestDir));

        // ANS_Move가 벽에 막혔는데도 남은 root motion을 계속 밀어 넣지 않도록, 전진량이 거의 없으면 이번 tick 이동을 중단한다.
        if (progressedDistance <= requestedDistance * 0.1f)
            return false;
    }

    if (!constrainToGround)
        return true;

    return Resolve_NotifyGroundSnap(previousPos, _transform);
}

bool MovementComponent::Resolve_NotifyGroundSnap(const Vec3& previousPos, Shared<Transform> transform)
{
    if (!transform)
        return false;

    Vec3 currentPos = transform->Get_WorldPosition();

    FSurfaceHit groundHit{};
    const bool foundGround = Detect_FloorBelow(currentPos, groundHit);

    const float maxClimbHeight = _moveDesc.groundTraceStartOffsetY + _moveDesc.groundSnapTolerance;

    if (foundGround)
    {
        const float heightDelta = groundHit.hitPoint.y - previousPos.y;

        if (heightDelta > maxClimbHeight)
        {
            transform->Set_WorldPosition(previousPos);
            return false;
        }

        currentPos.y = groundHit.hitPoint.y;
        transform->Set_WorldPosition(currentPos);

        _velocity.y = 0.f;
        _onGround = true;

        return true;
    }

    if (currentPos.y <= _moveDesc.groundY + _moveDesc.groundSnapTolerance)
    {
        currentPos.y = _moveDesc.groundY;
        transform->Set_WorldPosition(currentPos);

        _velocity.y = 0.f;
        _onGround = true;

        return true;
    }

    transform->Set_WorldPosition(previousPos);
    return false;
}

Vec3 MovementComponent::Build_NotifyMotionStepDelta(const Vec3& stepDelta, bool constrainToGround) const
{
    Vec3 resolvedDelta = stepDelta;

    if (constrainToGround)
        resolvedDelta.y = 0.f;

    return resolvedDelta;
}

bool MovementComponent::Is_GroundLikeNormal(const Vec3& hitNormal) const
{
    const Vec3 normal = Utils::Safe_Normalize(hitNormal, Vec3::Up);
    return normal.Dot(Vec3::Up) >= _moveDesc.groundWalkableMinUpDot;
}

bool MovementComponent::Is_WallLikeNormal(const Vec3& hitNormal) const
{
    const Vec3 normal = Utils::Safe_Normalize(hitNormal, Vec3::Up);
    const float upDot = fabsf(normal.Dot(Vec3::Up));

    return upDot <= _moveDesc.wallRunnableMaxUpDot;
}

bool MovementComponent::Trace_WallRunSurface(const Vec3& currentPos, const Vec3& wallNormal, FSurfaceHit& outHit) const
{
    const Vec3 outwardNormal = Utils::Safe_Normalize(wallNormal, Vec3::Up);
    const Vec3 inwardDir = -outwardNormal;
    const float probeStartOffset = _moveDesc.wallAttachOffset + 0.75f; // 벽 바깥쪽에서 다시 안쪽으로 쏘기 위한 재검증 시작 거리다.
    const float traceDistance = probeStartOffset + 0.5f; // 시작점에서 벽면까지 충분히 닿도록 보장하는 재검증 ray 길이다.
    const float verticalOffsets[3] = { 0.45f, 0.f, -0.45f };

    FSurfaceHit bestHit{};

    const ECollisionProxyType proxyTypes[3] =
    {
        ECollisionProxyType::WallRun,
        ECollisionProxyType::WorldBlock,
        ECollisionProxyType::Walkable
    };

    for (float verticalOffset : verticalOffsets)
    {
        const Vec3 rayStart =
            currentPos +
            Vec3(0.f, _moveDesc.wallTraceStartOffsetY + verticalOffset, 0.f) +
            outwardNormal * probeStartOffset;
        const Vec3 rayEnd = rayStart + inwardDir * traceDistance;

        for (ECollisionProxyType proxyType : proxyTypes)
        {
            FPhysXRaycastHit physXHit{};
            const bool isHit = GAME->Raycast_PhysX(
                rayStart,
                inwardDir,
                traceDistance,
                physXHit,
                proxyType);

            if (!isHit)
                continue;

            FSurfaceHit hit{};
            hit.hitPoint = physXHit.position;
            hit.hitNormal = Utils::Safe_Normalize(physXHit.normal, Vec3::Up);
            hit.hitDistance = physXHit.distance;
            hit.isValid = true;

            if (!Is_WallLikeNormal(hit.hitNormal))
                continue;

            // 유지 검증에서도 proxy normal이 반대로 들어온 경우 현재 벽 normal 방향에 맞춰 뒤집는다.
            if (hit.hitNormal.Dot(outwardNormal) < 0.15f)
                hit.hitNormal = -hit.hitNormal;

            if (hit.hitNormal.Dot(outwardNormal) < 0.15f)
                continue;

            if (!bestHit.isValid || hit.hitDistance < bestHit.hitDistance)
                bestHit = hit;
        }

        Draw_TraceDebug(rayStart, rayEnd, bestHit);
    }

    outHit = bestHit;
    return bestHit.isValid;
}

void MovementComponent::Draw_TraceDebug(const Vec3& start, const Vec3& end, const FSurfaceHit& hit) const
{
    if (!_traceDebugEnabled)
        return;

    FDebugTraceLineDesc traceDesc{};
    traceDesc.start = start;
    traceDesc.end = end;
    traceDesc.isHit = hit.isValid;
    traceDesc.hitPoint = hit.hitPoint;
    traceDesc.hitNormal = hit.hitNormal;
    traceDesc.duration = 0.f;
    traceDesc.depthEnabled = true;
    traceDesc.drawHitPoint = true;
    traceDesc.drawHitNormal = true;
    traceDesc.drawRemainderOnHit = true;

    GAME->Draw_DebugTraceLine(traceDesc);
}

void MovementComponent::Apply_CeilingBlock(const Vec3& previousPos, Shared<Transform> transform)
{
    if (!transform)
        return;

    Vec3 currentPos = transform->Get_WorldPosition();
    const Vec3 moveDelta = currentPos - previousPos;

    if (moveDelta.y <= FLT_EPSILON)
        return;

    const float headTraceStartOffsetY = _moveDesc.wallTraceStartOffsetY + 0.35f;
    const float traceDistance = moveDelta.y + 0.10f;
    const Vec3 rayStart = previousPos + Vec3(0.f, headTraceStartOffsetY, 0.f);
    const Vec3 rayEnd = rayStart + Vec3::Up * traceDistance;

    FSurfaceHit bestHit{};

    const ECollisionProxyType proxyTypes[2] =
    {
        ECollisionProxyType::WorldBlock,
        ECollisionProxyType::Walkable
    };

    for (ECollisionProxyType proxyType : proxyTypes)
    {
        FPhysXRaycastHit physXHit{};
        const bool isHit = GAME->Raycast_PhysX(
            rayStart,
            Vec3::Up,
            traceDistance,
            physXHit,
            proxyType);

        if (!isHit)
            continue;

        FSurfaceHit hit{};
        hit.hitPoint = physXHit.position;
        hit.hitNormal = Utils::Safe_Normalize(physXHit.normal, Vec3::Down);
        hit.hitDistance = physXHit.distance;
        hit.isValid = true;

        if (!bestHit.isValid || hit.hitDistance < bestHit.hitDistance)
            bestHit = hit;
    }

    Draw_TraceDebug(rayStart, rayEnd, bestHit);

    if (!bestHit.isValid)
        return;

    currentPos.y = min(currentPos.y, bestHit.hitPoint.y - headTraceStartOffsetY - 0.01f);
    transform->Set_WorldPosition(currentPos);

    if (_velocity.y > 0.f)
        _velocity.y = 0.f;
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

    auto owner = _owner.lock();
    Shared<Collider> bodyCollider = owner ? owner->Get_Component<Collider>() : nullptr;

    const float wallClearance = Calculate_BodyWallClearance(bodyCollider, _moveDesc.wallAttachOffset);
    Vec3 moveDir = Utils::Safe_Normalize(moveDelta, Vec3::Forward);
    const float traceDistance = moveDelta.Length() + wallClearance + 0.05f;
    const float verticalOffsets[5] = { 0.65f, 0.3f, 0.f, -0.3f, -0.65f };
    FSurfaceHit blockHit{};
    Vec3 debugStart = previousPos + Vec3(0.f, _moveDesc.wallTraceStartOffsetY, 0.f);
    Vec3 debugEnd = debugStart + moveDir * traceDistance;

    const ECollisionProxyType proxyTypes[2] =
    {
        ECollisionProxyType::WorldBlock,
        ECollisionProxyType::Walkable
    };

    for (float verticalOffset : verticalOffsets)
    {
        const Vec3 rayStart = previousPos + Vec3(0.f, _moveDesc.wallTraceStartOffsetY + verticalOffset, 0.f);
        const Vec3 rayEnd = rayStart + moveDir * traceDistance;

        for (ECollisionProxyType proxyType : proxyTypes)
        {
            // 작은 발판 위에서 전진할 때, 현재 밟고 있는 Walkable의 아래/옆면까지 벽으로 잡으면
            // ground snap과 wall block이 서로 싸우면서 덜덜 떠는 현상이 생긴다.
            if (_onGround && proxyType == ECollisionProxyType::Walkable && verticalOffset < 0.f)
                continue;

            FPhysXRaycastHit physXHit{};
            const bool isHit = GAME->Raycast_PhysX(
                rayStart,
                moveDir,
                traceDistance,
                physXHit,
                proxyType);

            if (!isHit)
                continue;

            FSurfaceHit hit{};
            Vec3 hitNormal = Utils::Safe_Normalize(physXHit.normal, Vec3::Up);
            if (moveDir.Dot(hitNormal) > 0.f)
                hitNormal = -hitNormal;

            hit.hitPoint = physXHit.position;
            hit.hitNormal = hitNormal;
            hit.hitDistance = physXHit.distance;
            hit.isValid = true;

            // 지상 이동 중에는 발판 아래쪽에 있는 Walkable 측면을 막기 대상으로 보지 않는다.
            if (_onGround &&
                proxyType == ECollisionProxyType::Walkable &&
                hit.hitPoint.y < previousPos.y - _moveDesc.groundSnapTolerance)
            {
                continue;
            }

            if (!Is_WallLikeNormal(hit.hitNormal))
                continue;

            if (!blockHit.isValid || hit.hitDistance < blockHit.hitDistance)
            {
                blockHit = hit;
                debugStart = rayStart;
                debugEnd = rayEnd;
            }
        }
    }

    Draw_TraceDebug(debugStart, debugEnd, blockHit);

    if (!blockHit.isValid)
        return;

    const float intoWall = moveDelta.Dot(-blockHit.hitNormal);
    Vec3 correctedPos = currentPos;

    if (intoWall > 0.f)
    {
        Vec3 correctedDelta = moveDelta - (-blockHit.hitNormal) * intoWall;
        correctedPos = previousPos + correctedDelta;
        correctedPos.y = currentPos.y;
    }

    const float signedDistance = (correctedPos - blockHit.hitPoint).Dot(blockHit.hitNormal);
    if (signedDistance < wallClearance)
    {
        correctedPos += blockHit.hitNormal * (wallClearance - signedDistance);
    }

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
