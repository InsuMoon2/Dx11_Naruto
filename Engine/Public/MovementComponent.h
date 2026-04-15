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
        Shared<Model> hitModel; // 어떤 CollisionModel에 맞았는지.

        Matrix hitWorldMatrix = Matrix::Identity;

        Vec3 hitPoint = Vec3::Zero;
        Vec3 hitNormal = Vec3::Up;

        float hitDistance = FLT_MAX;
        bool  isValid = false;
    };

    struct FWireDashDesc
    {
        float maxDistance = 10.f;
        float approachSpeed = 28.f;
        float stopDistance = 0.4f;
        float traceStartOffsetY = 1.f;
        float fanAngleDegree = 8.f; // 정면 단일 ray 보강용 좌우 fan 각도
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

        float gravity = -20.f;      // 인스팩터에서 조절해야한다.
        float groundY = 5.f;        // Temp값. 일단 5로 조절

        // 벽타기 추가
        float groundTraceStartOffsetY = 0.5f;
        float groundSnapTolerance = 0.1f;

        float wallTraceStartOffsetY = 1.0f;
        float wallDetectDistance = 1.0f; // 전방 벽 감지 거리
        float wallAttachOffset = 0.15f;

        // 벽으로 인정할 수 있는 표면의 최대 Up dot 절댓값
        float wallRunnableMaxUpDot = 0.35f;
        float wallJumpUpVelocity = 8.f;
        float wallJumpOutVelocity = 6.f;

    };

    struct FMoveCommand
    {
        Vec2  moveAxis = Vec2::Zero;
        Vec2  lookDelta = Vec2::Zero;

        bool  sprint = false;

        bool  jump = false;
        bool  doublejump = false;
        float superJumpVelocity = 0.f;

        Vec3  moveBasisForward = Vec3(0.f, 0.f, 1.f);
        Vec3  moveBasisRight = Vec3(1.f, 0.f, 0.f);
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

    //void Start_Dash(EMoveInputDirection inputDir, float distance, float duration);
    void Start_Dash(const Vec3& worldDir, float distance, float duration);
    void Stop_Dash();

    // 공중콤보 중력조절
    void Set_GravityEnabled(bool flag) { _gravityEnabled = flag; }
    bool Is_GravityEnabled() const     { return _gravityEnabled; }

    bool Is_WallRunning() const { return _isWallRunning; }
    void Start_WallJump();
    void Set_WallCollisionModels(const vector<FCollisionModelInstance>& models) { _wallCollisionModels = models; }
    Vec3 Get_currentWallNormal() const { return _currentWallNormal; }

    void Set_Velocity(Vec3 velocity);
    Vec3 Get_Velocity() { return _velocity; }

    void Launch(const Vec3& launchVelocity, bool xyOverride = false, bool zOverride = false);

public:
    bool Get_OrientRotationToMovement() const { return _bOrientRotationToMovement; }
    void Set_OrientRotationToMovement(bool check) { _bOrientRotationToMovement = check; }

    float Get_DashNormalizedTime() const;

    void Reset_DoubleJumpCount() { _canDoubleJump = true; }

    // 바닥 충돌 모델 리스트 세팅
    void Set_GroundCollisionModels(const vector<FCollisionModelInstance>& models) { _groundCollisionModels = models; }

    const FWireDashDesc& Get_WireDashDesc() const { return _wireDashDesc; }

    bool Try_WireDash_WallTrace(const Vec3& traceStart, const Vec3& traceDir, FSurfaceHit& outHit) const;

private:
    void Update_Rotation(float timeDelta, Shared<Transform> transform);
    void Update_Velocity(float timeDelta, Shared<Transform> transform);
    void Apply_Movement(float timeDelta, Shared<Transform> transform);

    Vec3 Build_DesiredMoveDirection() const;

private: /* 벽타기 */
    Vec3 Build_WallRunMoveDirection() const;
    bool Detect_GroundSurface(const Vec3& currentPos, FSurfaceHit& outHit) const;
    bool Detect_WallSurface(const Vec3& currentPos, const Vec3& castDir, FSurfaceHit& outHit) const;

    bool Can_EnterWallRun(const FSurfaceHit& wallHit, const Vec3& desiredMoveDir) const;

    void Enter_WallRun(const FSurfaceHit& wallHit);
    void Exit_WallRun();

    void Apply_WallRunPosition(Shared<Transform> transform, const FSurfaceHit& wallHit);
    void Apply_WallRunRotation(float timeDelta, Shared<Transform> transform);

    void Restore_DefaultUpRotation(Shared<Transform> transform);

private:
    FMovementDesc _moveDesc;
    FMoveCommand _commandDesc;
    EMoveInputDirection _moveInputDirection;

    Vec3    _velocity = Vec3::Zero;

    bool    _onGround = true;
    bool    _canDoubleJump = false;

    Shared<Transform> _transform;

    bool _bOrientRotationToMovement = false;

    bool                _isDashing = false;
    EMoveInputDirection _dashInputDirection = EMoveInputDirection::Forward;
    Vec3                _dashWorldDirection = Vec3::Zero;
    float               _dashElapsed = 0.f;
    float               _dashDuration = 0.f;
    float               _dashSpeed = 0.f;

    // 중력
    bool _gravityEnabled = true;

    // 바닥 판정에 사용하는 충돌
    vector<FCollisionModelInstance> _groundCollisionModels;

    // 벽 감지/벽타기에 사용하는 충돌
    vector<FCollisionModelInstance> _wallCollisionModels;

    bool    _isWallRunning = false;

    Vec3    _currentWallNormal = Vec3::Up;
    Vec3    _currentWallHitPoint = Vec3::Zero;

    float   _wallJumpCooldown = 0.f;

    // Wire Dash
    FWireDashDesc _wireDashDesc;

public:
    static Shared<MovementComponent> Create(ComPtr<Device> device, ComPtr<DeviceContext> context);
    Shared<Component> Clone(void* arg) override;
    void Free() override;
};

NS_END
