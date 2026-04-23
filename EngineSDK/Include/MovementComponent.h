#pragma once

#include "Component.h"

NS_BEGIN(Engine)
class Transform;
class Model;

class ENGINE_DLL MovementComponent final : public Component
{
    GENERATED_COMPONENT(MovementComponent, Protocol::COMPONENT_TYPE_MOVEMENT)

public:
    enum class ESurfaceMoveMode
    {
        Ground,
        Air,
        WallRun,
        END
    };

    struct FCollisionModelInstance
    {
        Shared<Model> model;
        Matrix worldMatrix = Matrix::Identity;
        BoundingBox worldBounds{};
        bool hasWorldBounds = false;
    };

    struct FSurfaceHit
    {
        Shared<Model> hitModel; // 어떤 collision surface에 맞았는지 보관할 때 사용한다.
        Matrix hitWorldMatrix = Matrix::Identity;
        Vec3 hitPoint = Vec3::Zero;
        Vec3 hitNormal = Vec3::Up;
        float hitDistance = FLT_MAX;
        bool isValid = false;
    };

    struct FWireDashDesc
    {
        float maxDistance = 10.f;
        float approachSpeed = 28.f;
        float stopDistance = 0.4f;
        float traceStartOffsetY = 1.f;
        float fanAngleDegree = 8.f;
    };

    struct FMovementDesc
    {
        float maxWalkSpeed = 4.f;
        float maxSprintSpeed = 7.f;
        float acceleration = 20.f;
        float deceleration = 24.f;
        float yawSpeed = 360.f;

        float jumpVelocity = 11.f;
        float doubleJumpVelocity = 9.f;

        float superJumpMinVelocity = 10.f;
        float superJumpMaxVelocity = 50.f;

        float dashDistance = 6.f;
        float dashDuration = 0.18f;

        float gravity = -20.f;
        float groundY = 5.f;

        float groundTraceStartOffsetY = 0.5f;
        float groundSnapTolerance = 0.1f;

        float wallTraceStartOffsetY = 1.0f;
        float wallDetectDistance = 1.0f;
        float wallAttachOffset = 0.15f;

        float wallRunnableMaxUpDot = 0.35f;
        float wallJumpUpVelocity = 8.f;
        float wallJumpOutVelocity = 6.f;

        float groundWalkableMinUpDot = 0.55f;
    };

    struct FMoveCommand
    {
        Vec2 moveAxis = Vec2::Zero;
        Vec2 lookDelta = Vec2::Zero;

        bool sprint = false;
        bool jump = false;
        bool doublejump = false;
        float superJumpVelocity = 0.f;

        Vec3 moveBasisForward = Vec3(0.f, 0.f, 1.f);
        Vec3 moveBasisRight = Vec3(1.f, 0.f, 0.f);
    };

public:
    explicit MovementComponent(ComPtr<Device> device, ComPtr<DeviceContext> context);
    explicit MovementComponent(const MovementComponent& rhs);
    virtual ~MovementComponent();

public:
    HRESULT Initialize_Prototype() override;
    HRESULT Initialize(void* arg) override;
    void    BeginPlay() override;

public:
    void Apply_Command(const FMoveCommand& cmd);
    void Update(float timeDelta);

    bool Is_OnGround() { return _onGround == true; }
    bool Can_DoubleJump() { return _canDoubleJump == true; }

    const FMovementDesc& Get_MoveDesc() const { return _moveDesc; }

    void Start_Jump();
    void Start_DoubleJump();
    void Start_SuperJump(float velocity);

    void Start_Dash(const Vec3& worldDir, float distance, float duration);
    void Stop_Dash();

    void Set_GravityEnabled(bool flag) { _gravityEnabled = flag; }
    bool Is_GravityEnabled() const { return _gravityEnabled; }

    bool Is_WallRunning() const { return _isWallRunning; }
    void Start_WallJump();

    void Set_SurfaceCollisionModels(const vector<FCollisionModelInstance>& models) { _surfaceCollisionModels = models; }

    void Set_BlockCollisionModels(const vector<FCollisionModelInstance>& models) { _blockCollisionModels = models; }
    void Set_TraceDebugEnabled(bool enabled) { _traceDebugEnabled = enabled; }

    Vec3 Get_currentWallNormal() const { return _currentWallNormal; }

    void Set_Velocity(Vec3 velocity);
    Vec3 Get_Velocity() { return _velocity; }

    void Launch(const Vec3& launchVelocity, bool xyOverride = false, bool zOverride = false);

public:
    bool Get_OrientRotationToMovement() const { return _bOrientRotationToMovement; }
    void Set_OrientRotationToMovement(bool check) { _bOrientRotationToMovement = check; }

    float Get_DashNormalizedTime() const;

    void Reset_DoubleJumpCount() { _canDoubleJump = true; }

    const FWireDashDesc& Get_WireDashDesc() const { return _wireDashDesc; }

    bool Try_WireDash_WallTrace(const Vec3& traceStart, const Vec3& traceDir, FSurfaceHit& outHit) const;

    void Enter_WallRun(const FSurfaceHit& wallHit);
    void Exit_WallRun();

private:
    void Update_Rotation(float timeDelta, Shared<Transform> transform);
    void Update_Velocity(float timeDelta, Shared<Transform> transform);
    void Apply_Movement(float timeDelta, Shared<Transform> transform);


public:
    Vec3 Build_DesiredMoveDirection() const;
    Vec3 Build_WallRunMoveDirection() const;

    bool Detect_FloorBelow(const Vec3& currentPos, FSurfaceHit& outHit) const;
    bool Detect_WallAhead(const Vec3& currentPos, const Vec3& desiredDir, FSurfaceHit& outHit) const;
    bool Detect_TopLanding(const Vec3& currentPos, const Vec3& wallNormal, FSurfaceHit& outHit) const;

    bool Can_EnterWallRun(const FSurfaceHit& wallHit, const Vec3& desiredMoveDir) const;

    void Apply_WallRunPosition(Shared<Transform> transform, const FSurfaceHit& wallHit);
    void Apply_WallRunRotation(float timeDelta, Shared<Transform> transform);

    void Restore_DefaultUpRotation(Shared<Transform> transform);

    void Apply_NotifyMotionDelta(const Vec3& worldDelta, bool constrainToGround);
    bool Apply_NotifyMotionStep(const Vec3& stepDelta, bool constrainToGround);
    bool Resolve_NotifyGroundSnap(const Vec3& previousPos, Shared<Transform> transform);
    Vec3 Build_NotifyMotionStepDelta(const Vec3& stepDelta, bool constrainToGround) const;

private:
    bool Trace_Surface(const Ray& ray, float maxDistance, FSurfaceHit& outHit) const;
    bool Trace_BlockSurface(const Ray& ray, float maxDistance, FSurfaceHit& outHit) const;

    bool Is_GroundLikeNormal(const Vec3& hitNormal) const;
    bool Is_WallLikeNormal(const Vec3& hitNormal) const;
    void Draw_TraceDebug(const Vec3& start, const Vec3& end, const FSurfaceHit& hit) const;

    static Vec3 Rotate_HorizontalDirection(const Vec3& dir, float degrees);
    static bool Is_CharacterBodyChannel(Collision_Channel channel);
    bool Is_BlockedByCharacterBody(const Vec3& testPosition, Shared<Transform> transform) const;
    void Apply_CharacterBodyBlock(const Vec3& previousPos, Shared<Transform> transform);

    void Apply_WallBlock(const Vec3& previousPos, Shared<Transform> transform);

private:
    FMovementDesc _moveDesc;
    FMoveCommand _commandDesc;
    EMoveInputDirection _moveInputDirection;

    Vec3 _velocity = Vec3::Zero;

    bool _onGround = true;
    bool _canDoubleJump = false;

    Shared<Transform> _transform;

    bool _bOrientRotationToMovement = false;

    bool _isDashing = false;
    EMoveInputDirection _dashInputDirection = EMoveInputDirection::Forward;
    Vec3 _dashWorldDirection = Vec3::Zero;
    float _dashElapsed = 0.f;
    float _dashDuration = 0.f;
    float _dashSpeed = 0.f;

    bool _gravityEnabled = true;

    vector<FCollisionModelInstance> _surfaceCollisionModels;
    vector<FCollisionModelInstance> _blockCollisionModels;

    bool _traceDebugEnabled = false;

    bool _isWallRunning = false;

    Vec3 _currentWallNormal = Vec3::Up;
    Vec3 _currentWallHitPoint = Vec3::Zero;
    float _wallRunLostContactElapsed = 0.f;

    float _wallJumpCooldown = 0.f;

    FWireDashDesc _wireDashDesc;

public:
    static Shared<MovementComponent> Create(ComPtr<Device> device, ComPtr<DeviceContext> context);
    Shared<Component> Clone(void* arg) override;
    void Free() override;
};

NS_END
