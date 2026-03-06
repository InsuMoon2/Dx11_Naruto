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
        float yawSpeed = 0.2f;      // Mouse delta -> Yaw factor

        float jumpVelocity = 8.f;
        float gravity = -20.f;
        float groundY = 5.f;        // Temp값. 일단 5로 조절
    };

    struct FMoveCommand
    {
        Vec2 moveAxis = Vec2::Zero;
        Vec2 lookDelta = Vec2::Zero;
        bool sprint = false;
        bool jump = false;
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

private:
    void Update_Rotation(float timeDelta, Shared<Transform> transform);
    void Update_Velocity(float timeDelta, Shared<Transform> transform);
    void Apply_Movement(float timeDelta, Shared<Transform> transform);

private:
    FMovementDesc _moveDesc;
    FMoveCommand _commandDesc;

    Vec3    _velocity = Vec3::Zero;

    bool    _onGround = true;
    float   _verticalVelocity = 0.f;

    Shared<Transform> _transform;

public:
    static Shared<MovementComponent> Create(ComPtr<Device> device, ComPtr<DeviceContext> context);
    Shared<Component> Clone(void* arg) override;
    void Free() override;
};

NS_END
