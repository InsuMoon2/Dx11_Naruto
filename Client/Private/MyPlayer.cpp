#include "pch.h"
#include "MyPlayer.h"
#include "MovementComponent.h"
#include "Bounding_Sphere.h"
#include "Model.h"
#include "Bounding_Capsule.h"
#include "EquipmentComponent.h"
#include "Collider.h"
#include "CombatStat.h"
#include "InputComponent.h"
#include "MovementComponent.h"
#include "NetworkManager.h"
#include "PlayerController.h"
#include "PlayerStateMachine.h"
#include "Client_PacketHandler.h"
#include "AnimationStateComponent.h"
#include "GameObject_Factory.h"
#include "Debug_Manager.h"
#include "TargetComponent.h"
#include "Customizer_Manager.h"
#include "SkillComponent.h"
#include "Bounding_OBB.h"

REGISTER_GAMEOBJECT(MyPlayer, Protocol::OBJECT_TYPE_PLAYER)

MyPlayer::MyPlayer(ComPtr<Device> device, ComPtr<DeviceContext> context)
    : Player(device, context)
{

}

MyPlayer::MyPlayer(const MyPlayer& rhs)
    : Player(rhs)
{
}

HRESULT MyPlayer::Initialize_Prototype()
{

    return S_OK;
}

HRESULT MyPlayer::Initialize(void* arg)
{
    CHECK_FAILED(Player::Initialize(arg), E_FAIL);

    // 이름 세팅
    auto custom = GET_SINGLE(Customizer_Manager);
    wstring name = custom->Get_PlayerName();
    // Set_PlayerName(name) 또는 프로토콜 패킷에 이름 포함

    // MyPlayer만 입력/이동 컴포넌트 보유
    {
        MovementComponent::FMovementDesc moveDesc;
        moveDesc.maxWalkSpeed = 40.f;
        moveDesc.maxSprintSpeed = 70.f;

        CHECK_FAILED(Add_Component(Protocol::COMPONENT_TYPE_MOVEMENT, _movement, &moveDesc), E_FAIL);
        CHECK_FAILED(Add_Component(Protocol::COMPONENT_TYPE_INPUT, _input), E_FAIL);
    }


    CHECK_FAILED(Add_Component(Protocol::COMPONENT_TYPE_PLAYER_CONTROLLER, _playerController), E_FAIL);
    CHECK_FAILED(Add_Component(Protocol::COMPONENT_TYPE_PLAYER_STATE, _stateMachine), E_FAIL);

    // 충돌체 추가
    //Bounding_Sphere::FBoundingSphereDesc sphereDesc{};
    //sphereDesc.radius = 1.5f;

    Bounding_OBB::FBoundingOBBDesc obbDesc{};
    obbDesc.center = Vec3(0.f, 0.7f, 0.f);
    obbDesc.extents = Vec3(0.5f, 0.6f, 0.5f);

    CHECK_FAILED(Add_Component(Protocol::COMPONENT_TYPE_COLLIDER_OBB, _collider, &obbDesc), E_FAIL);
    _collider->Set_CollisionPreset(Collision_Preset::Player_Body);
	_collider->Set_IsActive(true);

    // 주먹, 발 충돌체 추가
    CHECK_FAILED(Ready_HitboxColliders(), E_FAIL);

    return S_OK;
}

void MyPlayer::BeginPlay()
{
    Player::BeginPlay();

    if (_equipment)
        _equipment->Set_WeaponType(EWeaponType::Hand);

    if (_skill)
        _skill->Apply_WeaponSkillSet(EWeaponType::Hand);

    Refresh_WeaponAttachment_ByCurrentState();
    GAME->Get_DelegateHub().OnWeaponTypeChanged.Broadcast(static_cast<int32>(EWeaponType::Hand));
}

void MyPlayer::Priority_Update(float timeDelta)
{
    Player::Priority_Update(timeDelta);

    if (_playerController)
        _playerController->Update(timeDelta);
}

void MyPlayer::Update(float timeDelta)
{
    Player::Update(timeDelta);

    Update_Combo(timeDelta);
}

void MyPlayer::Late_Update(float timeDelta)
{
    Player::Late_Update(timeDelta);

    if (_target)
        _target->Late_Update(timeDelta);

    _syncTimer += timeDelta;
    if (_syncTimer >= _syncInterval)
    {
        _syncTimer = 0.f;
        Send_MovePacket(false);
    }

    // 몸통 충돌체
    if (_collider)
    {
        _collider->Update_Collider(_transformCom->Get_WorldMatrix());
        GAME->Add_Collider(_collider);
    }

    // 격투형 히트박스
    if (_equipment && _equipment->Get_CurrentWeaponType() == EWeaponType::Hand)
    {
        for (int i = 0; i < ETOI(EHitboxTarget::END); ++i)
        {
            auto& collider = _hitboxColliders[i];

            if (!collider) continue;

            const Matrix* boneMat = _model
                ? _model->Get_SocketBoneMatrixPtr(_hitboxBoneNames[i])
                : nullptr;

            Matrix finalMat = boneMat
                ? (*boneMat) * _transformCom->Get_WorldMatrix()
                : _transformCom->Get_WorldMatrix();

            const Matrix worldMat = Matrix::CreateTranslation(finalMat.Translation());

            collider->Update_Collider(worldMat);

            GAME->Add_Collider(collider);
        }
    }

    Update_Targetting(timeDelta);
}

void MyPlayer::Force_SendMovePacket()
{
    Send_MovePacket(true);
}

void MyPlayer::Enable_Hitbox(EHitboxTarget target)
{
    int idx = ETOI(target);
    if (idx < 0 || idx >= ETOI(EHitboxTarget::END))
        return;

    if (_hitboxColliders[idx])
        _hitboxColliders[idx]->Set_IsActive(true);

}

void MyPlayer::Disable_Hitbox(EHitboxTarget target)
{
    int idx = ETOI(target);
    if (idx < 0 || idx >= ETOI(EHitboxTarget::END))
        return;

    if (_hitboxColliders[idx])
        _hitboxColliders[idx]->Set_IsActive(false);
}

void MyPlayer::Disable_All_Hitboxes()
{
    for (auto& col : _hitboxColliders)
    {
        if (col)
            col->Set_IsActive(false);
    }
}

void MyPlayer::OnBeginOverlap(Shared<Collider> self, Shared<Collider> other)
{
    Player::OnBeginOverlap(self, other);

    if (self->Get_Channel() == Collision_Channel::Player_Target)
        return;

    // 격투형은 플레이어한테 충돌체가 달려있음
    Shared<Character> hitted = dynamic_pointer_cast<Character>(other->Get_Owner());
    if (!hitted || hitted.get() == this)
        return;

    if (_combatStat && _combatStat->Apply_Damage(hitted.get()))
    {
        Add_ComboHit();
    }
}

void MyPlayer::Add_ComboHit()
{
    _comboHitCount++;
    _comboDecayTimer = COMBO_DECAY_TIME;

    GAME->Get_DelegateHub().OnPlayerComboHit.Broadcast(_comboHitCount);
}

void MyPlayer::Update_Combo(float timeDelta)
{
    if (_comboHitCount > 0)
    {
        _comboDecayTimer -= timeDelta;
        if (_comboDecayTimer <= 0.f)
        {
            _comboHitCount = 0;
            _comboDecayTimer = 0.f;
        }
    }
}

void MyPlayer::Update_Targetting(float timeDelta)
{
    if (_target)
        _target->Update_Targeting(timeDelta);

}

void MyPlayer::Send_MovePacket(bool forceSend)
{
    Protocol::ObjectInfo info = Build_NetworkInfo();

    if (!forceSend && !Should_SendMovePacket(info))
        return;

    auto buf = Client_PacketHandler::Make_C_Move(info);
    if (!buf)
        return;

    GET_SINGLE(NetworkManager)->Send_Packet(buf);

    _lastSyncPos = Vec3(info.pos().x(), info.pos().y(), info.pos().z());
    _lastSyncRot = Vec3(info.rot_x(), info.rot_y(), info.rot_z());
    _lastObjectState = info.object_state();
    _lastMoveDir = info.move_dir();
    _lastAnimPhase = info.anim_phase();
    _lastAttackProfile = info.attack_profile();
    _lastAttackComboIndex = info.attack_combo_index();
    _lastAnimStateKey = info.anim_state_key();
    _lastHitReactionType = info.hit_reaction_type();
    _lastHitReactionSerial = info.hit_reaction_serial();
}

Protocol::ObjectInfo MyPlayer::Build_NetworkInfo() const
{
    Protocol::ObjectInfo info{};
    info.set_objectid(Get_NetworkId());
    info.set_objecttype(Protocol::OBJECT_TYPE_PLAYER);

    Vec3 pos = _transformCom->Get_WorldPosition();
    auto* protoPos = info.mutable_pos();
    protoPos->set_x(pos.x);
    protoPos->set_y(pos.y);
    protoPos->set_z(pos.z);

    Vec3 rot = _transformCom->Get_LocalRotation().ToEuler();
    info.set_rot_x(rot.x);
    info.set_rot_y(rot.y);
    info.set_rot_z(rot.z);

    if (_animState && _stateMachine)
    {
        _animState->Capture_FromStateMachine(_stateMachine);
        _animState->Write_ToObjectInfo(info);
    }

    info.set_name(Utils::ToString(Get_PlayerName()));

    if (_combatStat)
        _combatStat->Serialize_ToProtobuf(*info.mutable_stat());

    if (_equipment)
        info.set_weapon_type(To_ProtoWeaponType(_equipment->Get_CurrentWeaponType()));
    else
        info.set_weapon_type(Protocol::WEAPON_TYPE_HAND);

    return info;
}

HRESULT MyPlayer::Ready_Components()
{
    CHECK_FAILED(Player::Ready_Components(), E_FAIL);

    CHECK_FAILED(Add_Component(Protocol::COMPONENT_TYPE_TARGET, _target), E_FAIL);
   

    return S_OK;
}

HRESULT MyPlayer::Ready_HitboxColliders()
{
    _hitboxBoneNames[ETOI(EHitboxTarget::RightHand)] = "R_Hand_Weapon_cnt_tr";
    _hitboxBoneNames[ETOI(EHitboxTarget::LeftHand)] = "L_Hand_Weapon_cnt_tr";
    _hitboxBoneNames[ETOI(EHitboxTarget::RightFoot)] = "RightFoot";
    _hitboxBoneNames[ETOI(EHitboxTarget::LeftFoot)] = "LeftFoot";

    Bounding_Sphere::FBoundingSphereDesc sphereDesc{};
    sphereDesc.radius = 0.45f;

    for (int i = 0; i < ETOI(EHitboxTarget::END); ++i)
    {
        Shared<Component> comp =
            GAME->Clone_Component( Protocol::COMPONENT_TYPE_COLLIDER_SPHERE, &sphereDesc);

        if (!comp)
        {
            return E_FAIL;
        }

        comp->Set_Owner(GetSharedPtr());
        auto collider = static_pointer_cast<Collider>(comp);

        collider->Set_CollisionPreset(Collision_Preset::Player_Attack);

        collider->Set_IsActive(false);
        _hitboxColliders[i] = collider;
    }

    return S_OK;
}

bool MyPlayer::Should_SendMovePacket(const Protocol::ObjectInfo& nextInfo) const
{
    const Vec3 nextPos(
        nextInfo.pos().x(),
        nextInfo.pos().y(),
        nextInfo.pos().z());

    const float posDeltaSq = Vec3::DistanceSquared(nextPos, _lastSyncPos);
    const Vec3 nextRot(nextInfo.rot_x(), nextInfo.rot_y(), nextInfo.rot_z());
    const float rotDeltaSq = Vec3::DistanceSquared(nextRot, _lastSyncRot);

    if (posDeltaSq > 0.0001f)
        return true;

    if (rotDeltaSq > XMConvertToRadians(1.f) * XMConvertToRadians(1.f))
        return true;

    if (nextInfo.object_state() != _lastObjectState)
        return true;

    if (nextInfo.move_dir() != _lastMoveDir)
        return true;

    if (nextInfo.anim_phase() != _lastAnimPhase)
        return true;

    if (nextInfo.anim_force_restart())
        return true;

    if (nextInfo.attack_profile() != _lastAttackProfile)
        return true;

    if (nextInfo.attack_combo_index() != _lastAttackComboIndex)
        return true;

    if (nextInfo.anim_state_key() != _lastAnimStateKey)
        return true;

    if (nextInfo.hit_reaction_type() != _lastHitReactionType)
        return true;

    if (nextInfo.hit_reaction_serial() != _lastHitReactionSerial)
        return true;


    return false;
}

shared_ptr<GameObject> MyPlayer::Create(ComPtr<Device> device, ComPtr<DeviceContext> context)
{
    auto instance = make_shared<MyPlayer>(device, context);

    if (FAILED(instance->Initialize_Prototype()))
    {
        MSG_BOX("Failed to Create : MyPlayer");
        instance->Free();

        return nullptr;
    }

    return instance;
}

shared_ptr<GameObject> MyPlayer::Clone(void* arg)
{
    auto clone = make_shared<MyPlayer>(*this);

    if (FAILED(clone->Initialize(arg)))
    {
        MSG_BOX("Failed to Clone : MyPlayer");
        clone->Free();

        return nullptr;
    }

    return clone;
}

void MyPlayer::Free()
{
    Player::Free();
}
