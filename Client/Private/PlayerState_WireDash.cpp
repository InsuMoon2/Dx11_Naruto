#include "pch.h"
#include "PlayerState_WireDash.h"
#include "PlayerStateMachine.h"
#include "InputComponent.h"
#include "MovementComponent.h"
#include "Model.h"
#include "GameObject.h"
#include "Debug_Manager.h"
#include "PlayerState_AirApproach.h"
#include "WireMeshEffect.h"
#include "GameInstance.h"

PlayerState_WireDash::PlayerState_WireDash()
{
}

PlayerState_WireDash::~PlayerState_WireDash()
{
}

void PlayerState_WireDash::Enter(PlayerStateMachine* state)
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

    movement->Set_GravityEnabled(false);
    movement->Set_Velocity(Vec3(movement->Get_Velocity().x, 0.f, movement->Get_Velocity().z));

    input->Set_InputMode(EPlayerInputMode::LookOnly);

    _isWireAttach = false;
    _wireAttachPosition = Vec3::Zero;
    _wireMeshEffect.reset();

    const auto& wireDashDesc = movement->Get_WireDashDesc();

    Vec3 viewportRayStart{};
    Vec3 viewportRayDir{};

    if (Build_ViewportCenterRay(viewportRayStart, viewportRayDir))
    {
        Vec3 traceStart = transform->Get_WorldPosition();
        traceStart.y += wireDashDesc.traceStartOffsetY;

        const Vec3 traceDir = Utils::Safe_Normalize(viewportRayDir, transform->Get_WorldForward());
        const Vec3 traceEnd = traceStart + traceDir * wireDashDesc.maxDistance;

        Vec3 lookDir = traceDir;
        lookDir.y = 0.f;
        lookDir = Utils::Safe_Normalize(lookDir, transform->Get_WorldForward());

        if (lookDir.LengthSquared() > 0.0001f)
            transform->LookAt(transform->Get_WorldPosition() + lookDir);

        MovementComponent::FSurfaceHit wallHit{};
        bool isHit = movement->Try_WireDash_WallTrace(traceStart, traceDir, wallHit);

        /*{
            FDebugTraceLineDesc traceDesc{};
            traceDesc.start = traceStart;
            traceDesc.end = traceEnd;
            traceDesc.isHit = isHit;
            traceDesc.hitPoint = wallHit.hitPoint;
            traceDesc.hitNormal = wallHit.hitNormal;
            traceDesc.duration = 0.f;
            traceDesc.depthEnabled = true;
            traceDesc.drawHitPoint = true;
            traceDesc.drawHitNormal = true;
            traceDesc.drawRemainderOnHit = true;

            GAME->Draw_DebugTraceLine(traceDesc);
        }*/

        const Vec3 wireTargetPosition = isHit ? wallHit.hitPoint : traceEnd;
        Spawn_WireMesh(state, wireTargetPosition);

        if (isHit)
        {
            _isWireAttach = true;

            const float attachOffset = movement->Get_MoveDesc().wallAttachOffset;
            _wireAttachPosition = wallHit.hitPoint + wallHit.hitNormal * attachOffset;

            auto airApproach = state->Get_State<PlayerState_AirApproach>(EPlayerState::AirApproach);
            if (airApproach)
            {
                PlayerState_AirApproach::FApproachDesc desc{};
                desc.targetPosition = _wireAttachPosition;
                desc.stopDistance = movement->Get_WireDashDesc().stopDistance;
                desc.moveSpeed = movement->Get_WireDashDesc().approachSpeed;
                desc.maxApproachTime = 0.8f;
                desc.arriveAction = PlayerState_AirApproach::EArriveAction::WallAttach;
                desc.nextStateOnFail = EPlayerState::JumpFall;
                desc.wallNormal = wallHit.hitNormal;
                desc.wireMeshEffect = Consume_WireMeshEffect();

                airApproach->Set_AirApproachDesc(desc);
            }
        }
    }

    state->Play_AnimState(EPlayerState::WireDash);
}

void PlayerState_WireDash::Update(PlayerStateMachine* state, float timeDelta)
{
    (void)timeDelta;

    if (!state)
        return;

    auto cmd = state->Init_MoveCommand();
    cmd.jump = false;
    cmd.doublejump = false;

    if (state->Is_AnimStateFinished())
    {
        if (_isWireAttach)
        {
            state->Change_State(EPlayerState::AirApproach);
            return;
        }

        Destroy_WireMesh();

        state->Change_State(EPlayerState::JumpFall);
        return;
    }
}

void PlayerState_WireDash::Exit(PlayerStateMachine* state)
{
    if (!state)
        return;

    auto movement = state->Get_Movement();
    auto input = state->Get_Input();
    if (!movement || !input)
        return;

    if (!_isWireAttach)
        Destroy_WireMesh();

    movement->Set_GravityEnabled(true);
    input->Set_InputMode(EPlayerInputMode::Normal);
}

bool PlayerState_WireDash::Build_ViewportCenterRay(Vec3& outStart, Vec3& outDir) const
{
    const Matrix* viewMatrix = GAME->Get_Transform(ETransformState::View);
    const Matrix* projMatrix = GAME->Get_Transform(ETransformState::Proj);

    if (!viewMatrix || !projMatrix)
        return false;

    const float viewportWidth = max(1.f, GAME->Get_UIViewportWidth());
    const float viewportHeight = max(1.f, GAME->Get_UIViewportHeight());

    DirectX::SimpleMath::Viewport viewport(
        0.f,
        0.f,
        viewportWidth,
        viewportHeight,
        0.f,
        1.f);

    const float centerX = viewportWidth * 0.5f;
    const float centerY = viewportHeight * 0.5f;

    const Vec3 nearPoint = viewport.Unproject(
        Vec3(centerX, centerY, 0.f),
        *projMatrix,
        *viewMatrix,
        Matrix::Identity);

    const Vec3 farPoint = viewport.Unproject(
        Vec3(centerX, centerY, 1.f),
        *projMatrix,
        *viewMatrix,
        Matrix::Identity);

    outStart = nearPoint;
    outDir = Utils::Safe_Normalize(farPoint - nearPoint, Vec3::Forward);

    return true;
}

void PlayerState_WireDash::Spawn_WireMesh(PlayerStateMachine* state, const Vec3& targetPosition)
{
    if (!state)
        return;

    auto owner = state->Get_Owner();
    if (!owner)
        return;

    auto transform = owner->Get_Transform();
    auto model = owner->Get_Component<Model>();
    if (!transform || !model)
        return;

    const string boneName = "R_Hand_Weapon_cnt_tr";

    Vec3 spawnPos = transform->Get_WorldPosition();

    if (const Matrix* socketMatrix = model->Get_SocketBoneMatrixPtr(boneName))
    {
        Matrix boneWorldMatrix = (*socketMatrix) * transform->Get_WorldMatrix();
        spawnPos = boneWorldMatrix.Translation();
    }

    WireMeshEffect::FWireMeshEffectDesc desc{};
    desc.position = spawnPos;
    desc.effectAssetName = "WireDash";
    desc.ownerObj = owner;
    desc.trackBoneName = boneName;
    desc.targetPosition = targetPosition;
    desc.meshOriginalLength = 1.f;
    desc.thickness = Vec3(1.f, 1.f, 1.f);

    auto spawned = GAME->Clone_And_Add_GameObject(
        0,
        Protocol::OBJECT_TYPE_WIRE_MESH_EFFECT,
        GAME->Current_Level(),
        TEXT("Layer_Effect"),
        &desc);

    auto wireEffect = dynamic_pointer_cast<WireMeshEffect>(spawned);
    if (!wireEffect)
        return;

    _wireMeshEffect = wireEffect;
}

Weak<WireMeshEffect> PlayerState_WireDash::Consume_WireMeshEffect()
{
    Weak<WireMeshEffect> result = _wireMeshEffect;
    _wireMeshEffect.reset();

    return result;
}

void PlayerState_WireDash::Destroy_WireMesh()
{
    auto wireEffect = _wireMeshEffect.lock();
    if (wireEffect)
        wireEffect->Set_Destroy(true);

    _wireMeshEffect.reset();
}

Shared<PlayerState_WireDash> PlayerState_WireDash::Create()
{
    return make_shared<PlayerState_WireDash>();
}
