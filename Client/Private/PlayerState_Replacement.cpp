#include "pch.h"
#include "PlayerState_Replacement.h"
#include "PlayerStateMachine.h"
#include "InputComponent.h"
#include "MovementComponent.h"
#include "Model.h"
#include "GameObject.h"
#include "Debug_Manager.h"
#include "MyPlayer.h"
#include "Weapon.h"

static Vec3 Rotate_HorizontalDirectionY(const Vec3& dir, float degrees)
{
    Vec3 horizontal = dir;
    horizontal.y = 0.f;

    if (horizontal.LengthSquared() <= FLT_EPSILON)
        horizontal = Vec3::Forward;
    else
        horizontal.Normalize();

    Matrix rotation = Matrix::CreateRotationY(XMConvertToRadians(degrees));
    Vec3 rotated = Vec3::TransformNormal(horizontal, rotation);
    rotated.y = 0.f;

    if (rotated.LengthSquared() <= FLT_EPSILON)
        rotated = Vec3::Forward;
    else
        rotated.Normalize();

    return rotated;
}

void PlayerState_Replacement::Enter(PlayerStateMachine* state)
{
    if (!state)
        return;

    auto owner = state->Get_Owner();
    auto movement = state->Get_Movement();
    auto input = state->Get_Input();
    if (!owner || !movement || !input)
        return;

    auto transform = owner->Get_Transform();
    if (!transform)
        return;

    input->Set_InputMode(EPlayerInputMode::LookOnly);
    movement->Set_OrientRotationToMovement(false);
    movement->Set_Velocity(Vec3::Zero);

    Vec3 teleportPos = _teleportDestination;
    teleportPos.y += 2.5f;
    transform->Set_WorldPosition(teleportPos);

    state->Set_PendingLandingDir(_landingDirection);
    state->Play_AnimState(EPlayerState::JumpFall);

    auto myPlayer = dynamic_pointer_cast<MyPlayer>(owner);
    if (myPlayer)
        myPlayer->Disable_All_Hitboxes();

    auto container = dynamic_pointer_cast<ContainerObject>(owner);
    if (container)
    {
        auto weapon = dynamic_pointer_cast<Weapon>(
            container->Get_PartObject(ContainerObject::EPartSlot::Weapon));

        if (weapon)
            weapon->Set_ColliderActive(false);
    }

}

void PlayerState_Replacement::Update(PlayerStateMachine* state, float timeDelta)
{
    if (!state)
        return;

    auto movement = state->Get_Movement();
    auto input = state->Get_Input();

    if (!movement || !input)
        return;

    if (movement)
    {
        auto cmd = state->Init_MoveCommand();
        movement->Apply_Command(cmd);
        movement->Update(timeDelta);
    }

    if (movement->Is_OnGround())
    {
        if (input->Has_MoveInput())
        {
            state->Change_State(EPlayerState::Run);
            return;

        }
        else
        {
            state->Change_State(EPlayerState::Idle);
            return;
        }
    }

}

void PlayerState_Replacement::Exit(PlayerStateMachine* state)
{
    if (!state)
        return;

    auto input = state->Get_Input();
    auto movement = state->Get_Movement();

    if (input)
        input->Set_InputMode(EPlayerInputMode::Normal);

    if (movement)
        movement->Set_OrientRotationToMovement(true);

    _teleportDestination = Vec3::Zero;
    _landingDirection = Vec3::Forward;
    _isPrepared = false;
}

bool PlayerState_Replacement::Prepare_Replacement(PlayerStateMachine* state)
{
    if (!state)
        return false;

    auto movement = state->Get_Movement();
    auto owner = state->Get_Owner();
    if (!movement || !owner)
        return false;

    auto transform = owner->Get_Transform();
    if (!transform)
        return false;

    const Vec3 baseDirection = Compute_BaseDirection(state);

    Vec3 destination = Vec3::Zero;
    Vec3 landingDirection = Vec3::Forward;

    if (!Try_FindReplacementDestination(state, baseDirection, destination, landingDirection))
        return false;

    _teleportDestination = destination;
    _landingDirection = landingDirection;
    _isPrepared = true;

    return true;
}

Vec3 PlayerState_Replacement::Compute_BaseDirection(PlayerStateMachine* state)
{
    auto input = state->Get_Input();
    auto owner = state->Get_Owner();
    if (!input || !owner)
        return Vec3::Backward;

    Vec3 worldDir = Vec3::Zero;
    EMoveInputDirection inputDir = EMoveInputDirection::Forward;

    const bool hasMoveInput = state->Set_CameraRelativeMoveDirection(
        input->Get_MoveAxis(),
        inputDir,
        worldDir);

    if (hasMoveInput)
        return worldDir;

    auto transform = owner->Get_Transform();
    if (!transform)
        return Vec3::Backward;

    Vec3 backward = -transform->Get_WorldForward();
    backward.y = 0.f;

    if (backward.LengthSquared() <= FLT_EPSILON)
        backward = Vec3::Backward;
    else
        backward.Normalize();

    return backward;
}

bool PlayerState_Replacement::Try_FindReplacementDestination(PlayerStateMachine* state, const Vec3& baseDirection,
    Vec3& outDestination, Vec3& outLandingDirection) const
{
    auto owner = state->Get_Owner();
    if (!owner)
        return false;

    auto transform = owner->Get_Transform();
    if (!transform)
        return false;

    const Vec3 currentPos = transform->Get_WorldPosition();

    vector<Vec3> candidateDirections;
    candidateDirections.reserve(5);
    candidateDirections.push_back(baseDirection);
    candidateDirections.push_back(Rotate_HorizontalDirectionY(baseDirection, 30.f));
    candidateDirections.push_back(Rotate_HorizontalDirectionY(baseDirection, -30.f));
    candidateDirections.push_back(Rotate_HorizontalDirectionY(baseDirection, 90.f));
    candidateDirections.push_back(Rotate_HorizontalDirectionY(baseDirection, -90.f));

    const float candidateDistances[] = { 5.f, 3.5f, 2.f };

    for (const Vec3& dir : candidateDirections)
    {
        for (float distance : candidateDistances)
        {
            Vec3 candidate = currentPos + dir * distance;
            candidate.y = currentPos.y;

            Vec3 groundedPosition = Vec3::Zero;
            if (!Validate_GroundCandidate(state, candidate, groundedPosition))
                continue;

            outDestination = groundedPosition;
            outLandingDirection = dir;
            return true;
        }
    }

    return false;
}

bool PlayerState_Replacement::Validate_GroundCandidate(PlayerStateMachine* state, const Vec3& candidatePosition,
    Vec3& outGroundedPosition) const
{
    auto movement = state->Get_Movement();
    auto owner = state->Get_Owner();
    if (!movement || !owner)
        return false;

    auto transform = owner->Get_Transform();
    if (!transform)
        return false;

    MovementComponent::FSurfaceHit groundHit{};
    if (!movement->Detect_GroundSurface(candidatePosition, groundHit))
        return false;

    const Vec3 currentPos = transform->Get_WorldPosition();
    const float heightDiff = fabsf(groundHit.hitPoint.y - currentPos.y);

    if (heightDiff > 2.5f)
        return false;

    outGroundedPosition = Vec3(candidatePosition.x, groundHit.hitPoint.y, candidatePosition.z);
    return true;
}

Shared<PlayerState_Replacement> PlayerState_Replacement::Create()
{
    return make_shared<PlayerState_Replacement>();
}
