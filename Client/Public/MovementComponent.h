#pragma once

#include "Component.h"

NS_BEGIN(Engine)
class Transform;
NS_END

NS_BEGIN(Client)

class MovementComponent final : public Component
{
    GENERATED_COMPONENT(MovementComponent, Protocol::COMPONENT_TYPE_MOVEMENT)

public:
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

public:
    bool Get_OrientRotationToMovement() const { return _bOrientRotationToMovement; }
    void Set_OrientRotationToMovement(bool check) { _bOrientRotationToMovement = check; }

    float Get_DashNormalizedTime() const;

private:
    void Update_Rotation(float timeDelta, Shared<Transform> transform);
    void Update_Velocity(float timeDelta, Shared<Transform> transform);
    void Apply_Movement(float timeDelta, Shared<Transform> transform);

    Vec3 Build_DesiredMoveDirection() const;

private:
    FMovementDesc _moveDesc;
    FMoveCommand _commandDesc;
    EMoveInputDirection _moveInputDirection;

    Vec3    _velocity = Vec3::Zero;

    bool    _onGround = true;
    bool    _canDoubleJump = false;

    float   _verticalVelocity = 0.f;

    Shared<Transform> _transform;

    bool _bOrientRotationToMovement = false;

    bool                _isDashing = false;
    EMoveInputDirection _dashInputDirection = EMoveInputDirection::Forward;
    Vec3                _dashWorldDirection = Vec3::Zero;
    float               _dashElapsed = 0.f;
    float               _dashDuration = 0.f;
    float               _dashSpeed = 0.f;

public:
    static Shared<MovementComponent> Create(ComPtr<Device> device, ComPtr<DeviceContext> context);
    Shared<Component> Clone(void* arg) override;
    void Free() override;
};

NS_END
