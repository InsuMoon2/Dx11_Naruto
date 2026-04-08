#include "pch.h"
#include "PlayerStateMachine.h"
#include "GameObject.h"
#include "InputComponent.h"
#include "MovementComponent.h"
#include "AnimationStateComponent.h"
#include "Model.h"
#include "PlayerState_DoubleJump.h"
#include "Transform.h"
#include "PlayerState_Idle.h"
#include "PlayerState_Run.h"
#include "PlayerState_Jump.h"
#include "PlayerState_SuperJump.h"
#include "Camera.h"
#include "MyPlayer.h"
#include "PlayerState_Dash.h"
#include "PlayerState_HeightLand.h"
#include "PlayerState_Skill.h"
#include "PlayerState_SuperJumpCharge.h"
#include "GameObject.h"
#include "PlayerState_Attack.h"
#include "PlayerState_JumpDash.h"
#include "SkillComponent.h"
#include "SkillDataManager.h"
#include "EquipmentComponent.h"
#include "PlayerState_AirApproach.h"
#include "PlayerState_JumpAttack.h"
#include "PlayerState_JumpFall.h"
#include "PlayerState_Hit.h"
#include "PlayerState_WallRun.h"
#include "PlayerState_Wall_Idle.h"
#include "PlayerState_WireDash.h"


IMPLEMENT_REFLECTION(PlayerStateMachine)

bool PlayerStateMachine::Register_Properties()
{
    auto& info = GetStaticReflectionInfo();
    info.className = "PlayerStateMachine";

    //PROPERTY_ENUM("Current State", _currentStateID, EPlayerState);

    return true;
}

PlayerStateMachine::PlayerStateMachine(ComPtr<Device> device, ComPtr<DeviceContext> context)
    : Component(device, context)
{
}

PlayerStateMachine::PlayerStateMachine(const PlayerStateMachine& rhs)
    : Component(rhs)
{
    _states = rhs._states;

    _currentState = nullptr;
    _currentStateID = EPlayerState::END;
    _prevStateID = EPlayerState::END;
}


HRESULT PlayerStateMachine::Initialize_Prototype()
{
    Component::Initialize_Prototype();

    Register_State(EPlayerState::Idle, PlayerState_Idle::Create());
    Register_State(EPlayerState::Wall_Idle, PlayerState_Wall_Idle::Create());
    Register_State(EPlayerState::Run, PlayerState_Run::Create());
    Register_State(EPlayerState::Wall_Run, PlayerState_WallRun::Create());

    Register_State(EPlayerState::Jump, PlayerState_Jump::Create());
    Register_State(EPlayerState::JumpFall, PlayerState_JumpFall::Create());
    Register_State(EPlayerState::DoubleJump, PlayerState_DoubleJump::Create());
    Register_State(EPlayerState::SuperJumpCharge, PlayerState_SuperJumpCharge::Create());
    Register_State(EPlayerState::SuperJump, PlayerState_SuperJump::Create());

    Register_State(EPlayerState::HeightLand, PlayerState_HeightLand::Create());

    Register_State(EPlayerState::Dash, PlayerState_Dash::Create());
    Register_State(EPlayerState::JumpDash, PlayerState_JumpDash::Create());

    Register_State(EPlayerState::Attack, PlayerState_Attack::Create());
    Register_State(EPlayerState::JumpAttack, PlayerState_JumpAttack::Create());
    Register_State(EPlayerState::Hit, PlayerState_Hit::Create());

    Register_State(EPlayerState::WireDash, PlayerState_WireDash::Create());
    Register_State(EPlayerState::AirApproach, PlayerState_AirApproach::Create());

    return S_OK;
}

HRESULT PlayerStateMachine::Initialize(void* arg)
{
    Component::Initialize(arg);

    return S_OK;
}

void PlayerStateMachine::BeginPlay()
{
    Component::BeginPlay();

    auto owner = Get_Owner();
    _input = owner->Get_Component<InputComponent>();
    _movement = owner->Get_Component<MovementComponent>();
    _animationState = owner->Get_Component<AnimationStateComponent>();

    CHECK_NULL(_input);
    CHECK_NULL(_movement);
    CHECK_NULL(_animationState);

    auto Register_Skill = [&](int32 skill_Id)
        {
            auto skillData = GET_SINGLE(SkillDataManager)->Get_SkillData(skill_Id);
            CHECK_NULL(skillData);

            // 지상 스킬 등록
            if (!skillData->animStateName.empty())
            {
                auto enumVal = magic_enum::enum_cast<EPlayerState>(skillData->animStateName);
                if (enumVal.has_value())
                {
                    auto skillState = PlayerState_Skill::Create(skill_Id, enumVal.value());
                    Register_State(enumVal.value(), skillState);
                }
            }
            // 공중 스킬 등록
            if (!skillData->airAnimStateName.empty())
            {
                auto enumAirVal = magic_enum::enum_cast<EPlayerState>(skillData->airAnimStateName);
                if (enumAirVal.has_value())
                {
                    auto skillStateAir = PlayerState_Skill::Create(skill_Id, enumAirVal.value());
                    Register_State(enumAirVal.value(), skillStateAir);
                }
            }

        };

    // Skill 등록
    Register_Skill(ETOI(ESkillType::Rasengan));
    Register_Skill(ETOI(ESkillType::Rasen_Shuriken));

    Change_State(EPlayerState::Idle);
}

void PlayerStateMachine::Update(float timeDelta)
{
    if (Check_Global_Transitions())
        return;

    if (_currentState)
        _currentState->Update(this, timeDelta);
}

bool PlayerStateMachine::Check_Global_Transitions()
{
    auto currentId = Get_CurrentStateID();
    if (currentId == EPlayerState::Dead)
        return false;

    // TODO : 아직은 처리 안했음
    if (Check_Death())          return true;
    if (Check_Cinematic())      return true;
    if (Check_HitReaction())    return true;

    if (Check_Skill_Input())
        return true;

    Check_WeaponToggle();

    return false; // 바뀐 상태 X
}

bool PlayerStateMachine::Check_Skill_Input()
{
    auto input = Get_Input();
    const auto& frame = input->Get_Frame();

    auto movement = Get_Movement();

    bool isAir = movement && !movement->Is_OnGround();

    // 나중에, 바꿔치기도 스킬로 세팅해줄지. 우클릭, 바꿔치기도 넣을거면 여기서 슬롯을 늘려야 할듯?
    for (int slot = 0; slot < 2; ++slot)
    {
        if (frame.useSkillDown[slot])
        {
            auto player = static_pointer_cast<MyPlayer>(Get_Owner());
            auto skillComp = player->Get_Component<SkillComponent>();

            if (skillComp->Try_Activate(slot))
            {
                int32 skill_Id = skillComp->Get_EquippedSkillID(slot);
                auto skill_Data = GET_SINGLE(SkillDataManager)->Get_SkillData(skill_Id);

                if (skill_Data)
                {
                    string targetStateName = isAir
                        ? skill_Data->airAnimStateName
                        : skill_Data->animStateName;

                    if (targetStateName.empty())
                    {
                        LOG_WARN("[SKILL] 적용할 애니메이션 상태명이 비어있습니다. (isAir: %d)", isAir);
                        continue;
                    }

                    auto enumValue = magic_enum::enum_cast<EPlayerState>(targetStateName);
                    if (enumValue.has_value())
                    {
                        string msg = "[SKILL] ID: " + to_string(skill_Id) + " / 애니메이션 : " + targetStateName;

                        LOG_WARN(msg.c_str());

                        Set_ActiveSkillSlot(slot);
                        Change_State(enumValue.value());

                        return true;
                    }
                    else
                    {
                        LOG_WARN("[SKILL] 등록되지 않은 EPlayerState 이름입니다: %s", targetStateName.c_str());
                    }
                }
            }
        }
    }

    return false;
}

bool PlayerStateMachine::Check_Death()
{
    return false;
}

bool PlayerStateMachine::Check_Cinematic()
{
    return false;
}

bool PlayerStateMachine::Check_HitReaction()
{
    return false;
}

bool PlayerStateMachine::Set_CameraRelativeMoveDirection(const Vec2& moveAxis, EMoveInputDirection& outDir,
    Vec3& outWorldDir) const
{
    auto cmd = Init_MoveCommand();

    Vec2 axis = moveAxis;
    if (axis.LengthSquared() > 1.f)
        axis.Normalize();

    // 입력없을 때 Forward로 세팅
    if (axis.LengthSquared() <= FLT_EPSILON)
    {
        outDir = EMoveInputDirection::Forward;
        outWorldDir = cmd.moveBasisForward;
        outWorldDir.y = 0.f;

        if (outWorldDir.LengthSquared() <= FLT_EPSILON)
            outWorldDir = Vec3::Forward;
        else
            outWorldDir.Normalize();

        return false;
    }

    if (fabsf(axis.y) >= fabsf(axis.x))
    {
        outDir = (axis.y >= 0.f)
            ? EMoveInputDirection::Forward
            : EMoveInputDirection::Backward;
    }
    else
    {
        outDir = (axis.x >= 0.f)
            ? EMoveInputDirection::Right
            : EMoveInputDirection::Left;
    }

    Vec3 worldDir = cmd.moveBasisRight * axis.x + cmd.moveBasisForward * axis.y;
    worldDir.y = 0.f;

    if (worldDir.LengthSquared() <= FLT_EPSILON)
    {
        switch (outDir)
        {
        case EMoveInputDirection::Backward:
            worldDir = -cmd.moveBasisForward;
            break;
        case EMoveInputDirection::Left:
            worldDir = -cmd.moveBasisRight;
            break;
        case EMoveInputDirection::Right:
            worldDir = cmd.moveBasisRight;
            break;
        case EMoveInputDirection::Forward:
        default:
            worldDir = cmd.moveBasisForward;
            break;
        }
    }

    if (worldDir.LengthSquared() <= FLT_EPSILON)
        worldDir = Vec3::Forward;
    else
        worldDir.Normalize();

    outWorldDir = worldDir;

    return true;
}

string PlayerStateMachine::To_AnimationStateName(EPlayerState stateID)
{
    const auto name = magic_enum::enum_name(stateID);

    return name.empty() ? "" : string(name);
}

bool PlayerStateMachine::Check_WeaponToggle()
{
    if (Get_CurrentStateID() == EPlayerState::Attack)
        return false;

    auto input = Get_Input();
    if (!input)
        return false;

    const auto& frame = input->Get_Frame();

    if (!frame.toggleWeaponDown)
        return false;

    auto player = dynamic_pointer_cast<MyPlayer>(Get_Owner());
    if (!player)
        return false;

    auto equipment = player->Get_Component<EquipmentComponent>();
    if (!equipment)
        return false;

    auto skillCom = player->Get_Component<SkillComponent>();
    if (!skillCom)
        return false;

    auto currentWeapon = equipment->Get_CurrentWeaponType();

    if (currentWeapon == EWeaponType::Hand)
    {
        equipment->Toggle_WeaponMode();

        skillCom->Apply_WeaponSkillSet(equipment->Get_CurrentWeaponType());

        LOG_INFO("무기 변경: 격투 -> 대검");

        GAME->Get_DelegateHub().OnWeaponTypeChanged.Broadcast(
            static_cast<int32>(EWeaponType::BigSwrod));
    }
    else
    {
        equipment->Toggle_WeaponMode();

        skillCom->Apply_WeaponSkillSet(equipment->Get_CurrentWeaponType());

        LOG_INFO("무기 변경: 대검 -> 격투");

        GAME->Get_DelegateHub().OnWeaponTypeChanged.Broadcast(
            static_cast<int32>(EWeaponType::Hand));
    }

    EPlayerState currentState = Get_CurrentStateID();
    Play_AnimState(currentState);



    return false;
}

void PlayerStateMachine::Register_State(EPlayerState stateID, Shared<IPlayerState> state)
{
    _states[stateID] = state;
}

void PlayerStateMachine::Change_State(EPlayerState newState)
{
    if (newState == _currentStateID)
        return;

    auto iter = _states.find(newState);
    if (iter == _states.end())
        return;

    // 현재 상태 종료
    if (_currentState)
        _currentState->Exit(this);

    // 전환
    _prevStateID    = _currentStateID;
    _currentStateID = newState;
    _currentState   = iter->second;

    _currentState->Enter(this);

    auto player = dynamic_pointer_cast<Player>(Get_Owner());
    if (player)
    {
        player->Refresh_WeaponAttachment_ByCurrentState();
    }
}

MovementComponent::FMoveCommand PlayerStateMachine::Init_MoveCommand() const
{
    const auto& frame = _input->Get_Frame();

    MovementComponent::FMoveCommand cmd;
    cmd.moveAxis = Vec2(frame.moveX, frame.moveY);
    cmd.sprint = false;
    cmd.jump = false;

    auto activeCamera = GAME->Get_ActiveCamera();
    if (activeCamera && activeCamera->Get_ObjectType() == Protocol::OBJECT_TYPE_CAMERA_TARGET)
    {
        auto cameraTransform = activeCamera->Get_Component<Transform>();
        if (cameraTransform)
        {
            Vec3 forward = cameraTransform->Get_WorldForward();
            Vec3 right = cameraTransform->Get_WorldRight();

            forward.y = 0.f;
            right.y = 0.f;

            if (forward.LengthSquared() > FLT_EPSILON)
                forward.Normalize();

            if (right.LengthSquared() > FLT_EPSILON)
                right.Normalize();

            cmd.moveBasisForward = forward;
            cmd.moveBasisRight = right;
        }
    }

    return cmd;
}

bool PlayerStateMachine::Play_AnimState(EPlayerState stateID)
{
    string resolved = _animationState->Find_StateNameByWeapon(
        Get_Owner().get(), stateID);

    return _animationState->Play_State(resolved);
}

bool PlayerStateMachine::Play_DirectionalAnimState(EPlayerState stateID, EMoveInputDirection dir)
{
    string resolved = _animationState->Find_StateNameByWeapon(
        Get_Owner().get(), stateID);

    return _animationState->Play_DirectionalState(resolved, dir);
}

bool PlayerStateMachine::Play_AnimStateLoopOnly(EPlayerState stateID)
{
    string resolved = _animationState->Find_StateNameByWeapon(
        Get_Owner().get(), stateID);

    return _animationState->Play_StateLoopOnly(resolved);
}

void PlayerStateMachine::Request_AnimStateEnd()
{
    _animationState->Request_StateEnd();
}

bool PlayerStateMachine::Is_AnimStateFinished() const
{
    return _animationState ? _animationState->Is_CurrentStateFinished() : false;
}

bool PlayerStateMachine::Is_AnimSequenceFinished() const
{
    return _animationState ? _animationState->Is_CurrentStateSequenceFinished() : false;
}

const FStateAnimationDesc* PlayerStateMachine::Find_AnimStateDesc(EPlayerState stateID)
{
    if (!_animationState)
        return nullptr;

    string resolved = _animationState->Find_StateNameByWeapon(
        Get_Owner().get(), stateID);

    return _animationState->Find_State(resolved);
}

EAnimPhase PlayerStateMachine::Get_AnimPhase() const
{
    return _animationState ? _animationState->Get_CurrentAnimPhase() : EAnimPhase::Start;
}

float PlayerStateMachine::Get_AnimTrackPositionTicks() const
{
    return _animationState ? _animationState->Get_CurrentTrackPositionTicks() : 0.f;
}

float PlayerStateMachine::Get_AnimDurationTicks() const
{
    return _animationState ? _animationState->Get_CurrentAnimationDurationTicks() : 0.f;
}

float PlayerStateMachine::Get_AnimTrackPositionSec() const
{
    return _animationState ? _animationState->Get_CurrentTrackPositionSec() : 0.f;
}

float PlayerStateMachine::Get_AnimDurationSec() const
{
    return _animationState ? _animationState->Get_CurrentAnimationDurationSec() : 0.f;
}

void PlayerStateMachine::Force_Enter_State(EPlayerState stateID)
{
	auto iter = _states.find(stateID);
	if (iter == _states.end())
		return;

	_prevStateID = _currentStateID;
	_currentStateID = stateID;
	_currentState = iter->second;

	if (_currentState)
		_currentState->Enter(this);
}

json PlayerStateMachine::To_Json() const
{
    return Component::To_Json();
}

void PlayerStateMachine::From_Json(const json& data)
{
    Component::From_Json(data);

}

Shared<PlayerStateMachine> PlayerStateMachine::Create(ComPtr<Device> device, ComPtr<DeviceContext> context)
{
    auto instance = make_shared<PlayerStateMachine>(device, context);

    if (FAILED(instance->Initialize_Prototype()))
    {
        MSG_BOX("Failed to Create: PlayerStateMachine");
        instance.reset();
    }

    return instance;
}

Shared<Component> PlayerStateMachine::Clone(void* arg)
{
    auto clone = make_shared<PlayerStateMachine>(*this);

    if (FAILED(clone->Initialize(arg)))
    {
        MSG_BOX("Failed to Clone: PlayerStateMachine");
        clone.reset();
    }

    return clone;
}

void PlayerStateMachine::Free()
{
    _states.clear();
    _currentState = nullptr;

    Component::Free();
}
