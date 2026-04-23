#include "pch.h"
#include "ANS_Move.h"
#include "GameObject.h"
#include "MyPlayer.h"
#include "TargetComponent.h"
#include "Transform.h"
#include "AnimNotify_Factory.h"
#include "Monster.h"
#include "PlayerStateMachine.h"
#include "MovementComponent.h"

REGISTER_ANIM_NOTIFY_STATE(ANS_Move);
IMPLEMENT_REFLECTION(ANS_Move);

bool ANS_Move::Register_Properties()
{
    auto& info = GetStaticReflectionInfo();
    info.className = "ANS_Move";

    PROPERTY_FLOAT_JSON("Move Speed", "move_speed", _moveSpeed, -100.f, 100.f);
    PROPERTY_BOOL_JSON("Rotate To Target", "rotate_to_target", _rotateToTarget);
    PROPERTY_FLOAT_JSON("Rotation Speed", "rotation_speed", _rotationSpeed, 0.f, 1800.f);

    PROPERTY_ENUM("Direction Source", _directionSource, ANS_Move::EMoveDirectionSource);

    PROPERTY_VEC3_JSON("Move Velocity", "move_velocity", _moveVelocity, 0.1f);

    PROPERTY_BOOL_JSON("스킬 Y축 무시(지상용)", "ignore_y", _ignoreY);
    PROPERTY_BOOL_JSON("지형이동 매끄럽게", "constrain_to_ground", _constrainToGround);

    return true;
}

string ANS_Move::Get_TypeName() const
{
    return "ANS_Move";
}

void ANS_Move::On_Begin(const FAnimNotifyContext& context)
{
    if (context.isPreview)
        return;

    auto owner = context.owner;
    CHECK_NULL(owner);

    auto transform = owner->Get_Transform();
    CHECK_NULL(transform);

    Vec3 resolvedDir = Vec3::Zero;
    if (!Set_MoveDirection(context, resolvedDir))
    {
        resolvedDir = transform->Get_WorldForward();
        if (_ignoreY || _constrainToGround)
            resolvedDir.y = 0.f;

        if (resolvedDir.LengthSquared() <= FLT_EPSILON)
            resolvedDir = Vec3::Forward;
        else
            resolvedDir.Normalize();
    }

    _moveDir = resolvedDir;

    if (_rotateToTarget &&
        _directionSource == EMoveDirectionSource::TargetDirection &&
        _rotationSpeed <= 0.f)
    {
        Vec3 lookDir = _moveDir;

        if (_ignoreY || _constrainToGround)
            lookDir.y = 0.f;

        if (lookDir.LengthSquared() > FLT_EPSILON)
        {
            lookDir.Normalize();

            const Vec3 ownerPos = transform->Get_WorldPosition();
            transform->LookAt(ownerPos + lookDir);
        }
    }
}

void ANS_Move::On_Tick(const FAnimNotifyContext& context)
{
    if (context.isPreview)
        return;

    auto owner = context.owner;
    if (!owner)
        return;

    auto transform = owner->Get_Transform();
    if (!transform)
        return;

    auto movement = owner->Get_Component<MovementComponent>();
    if (!movement)
        return;

    const float dt = context.deltaTime;

    if (_rotateToTarget && _directionSource == EMoveDirectionSource::TargetDirection)
    {
        Vec3 targetDir = Vec3::Zero;
        if (Set_TargetDirection(context, targetDir))
        {
            Vec3 lookDir = targetDir;

            if (_ignoreY || _constrainToGround)
                lookDir.y = 0.f;

            if (lookDir.LengthSquared() > FLT_EPSILON)
            {
                lookDir.Normalize();

                if (_rotationSpeed > 0.f)
                {
                    Quat curRot = transform->Get_WorldRotation();
                    Quat targetRot = Quat::FromToRotation(Vec3::Backward, lookDir);

                    const float alpha = min(1.f, _rotationSpeed * dt / 180.f);
                    transform->Set_WorldRotation(Quat::Slerp(curRot, targetRot, alpha));
                }
                else
                {
                    const Vec3 ownerPos = transform->Get_WorldPosition();
                    transform->LookAt(ownerPos + lookDir);
                }
            }

            _moveDir = targetDir;
        }
    }

    if (_moveDir.LengthSquared() <= FLT_EPSILON)
        return;

    Vec3 forwardDir = _moveDir;
    Vec3 rightDir = Vec3::Up.Cross(forwardDir);

    rightDir.Normalize();
    Vec3 upDir = Vec3::Up;

    Vec3 finalOffset = (forwardDir * (_moveSpeed + _moveVelocity.z))
                     + (rightDir * _moveVelocity.x)
                     + (upDir * _moveVelocity.y);

    if (_ignoreY)
        finalOffset.y = 0.f;

    movement->Apply_NotifyMotionDelta(finalOffset * dt, _constrainToGround);
}

void ANS_Move::On_End(const FAnimNotifyContext& context)
{
    if (context.isPreview)
        return;
}

json ANS_Move::Serialize_Payload() const
{
    json j = AnimNotifyState::Serialize_Payload();
    j["direction_source"] = string(magic_enum::enum_name(_directionSource));

    return j;
}

void ANS_Move::Deserialize_Payload(const json& payload)
{
    AnimNotifyState::Deserialize_Payload(payload);

    if (payload.contains("direction_source"))
    {
        if (payload["direction_source"].is_string())
        {
            const string sourceName = payload["direction_source"].get<string>();
            _directionSource = magic_enum::enum_cast<EMoveDirectionSource>(sourceName)
                .value_or(EMoveDirectionSource::OwnerForward);
        }
        else if (payload["direction_source"].is_number_integer())
        {
            _directionSource = static_cast<EMoveDirectionSource>(payload["direction_source"].get<int>());
        }
    }
}

bool ANS_Move::Set_MoveDirection(const FAnimNotifyContext& context, Vec3& outDir) const
{
    auto owner = context.owner;
    if (!owner)
        return false;

    auto transform = owner->Get_Transform();
    if (!transform)
        return false;

    switch (_directionSource)
    {
    case EMoveDirectionSource::DashInputDirection:
        if (Set_DashDirection(context, outDir))
            return true;
        break;

    case EMoveDirectionSource::TargetDirection:
        if (Set_TargetDirection(context, outDir))
            return true;
        break;

    case EMoveDirectionSource::OwnerBackward:
        outDir = -transform->Get_WorldForward();
        break;

    case EMoveDirectionSource::OwnerForward:
    default:
        outDir = transform->Get_WorldForward();
        break;
    }

    if (_ignoreY)
        outDir.y = 0.f;

    if (outDir.LengthSquared() <= FLT_EPSILON)
        return false;

    outDir.Normalize();
    return true;
}

bool ANS_Move::Set_TargetDirection(const FAnimNotifyContext& context, Vec3& outDir) const
{
    auto owner = context.owner;
    if (!owner)
        return false;

    auto transform = owner->Get_Transform();
    if (!transform)
        return false;

    const Vec3 ownerPos = transform->Get_WorldPosition();

    auto myPlayer = dynamic_cast<MyPlayer*>(owner);
    auto targetCom = myPlayer
        ? myPlayer->Get_Component<TargetComponent>()
        : nullptr;

    if (!targetCom || !targetCom->IsLockOn())
        return false;

    auto lockedTarget = targetCom->Get_LockedTarget().lock();
    if (!lockedTarget)
        return false;

    auto targetTransform = lockedTarget->Get_Transform();
    if (!targetTransform)
        return false;

     Vec3 toTarget = targetTransform->Get_WorldPosition() - ownerPos;

    if (_ignoreY || _constrainToGround)
        toTarget.y = 0.f;

    if (toTarget.LengthSquared() <= 0.0001f)
        return false;

    toTarget.Normalize();
    outDir = toTarget;

    return true;
}

bool ANS_Move::Set_DashDirection(const FAnimNotifyContext& context, Vec3& outDir) const
{
    auto owner = context.owner;
    if (!owner)
        return false;

    auto stateMachine = owner->Get_Component<PlayerStateMachine>();
    if (!stateMachine)
        return false;

    outDir = stateMachine->Get_PendingDashWorldDirection();

    if (_ignoreY || _constrainToGround)
        outDir.y = 0.f;

    if (outDir.LengthSquared() <= FLT_EPSILON)
        return false;
}
