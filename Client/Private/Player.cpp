#include "pch.h"
#include "Player.h"
#include "CombatStat.h"
#include "Shader.h"
#include "Model.h"
#include "PartObject.h"
#include "EquipmentComponent.h"
#include "Weapon.h"
#include "AnimationStateComponent.h"
#include "Bounding_Capsule.h"
#include "Bounding_AABB.h"
#include "Collider.h"
#include "CharkraMove_Component.h"
#include "GhostEffect_Component.h"
#include "PlayerStateMachine.h"
#include "SmearEffect_Component.h"
#include "SwordTrail_Component.h"
#include "SkillComponent.h"
#include "Transform.h"

Player::Player(ComPtr<Device> device, ComPtr<DeviceContext> context)
    : Character(device, context)
{
    Set_ObjectType(Protocol::OBJECT_TYPE_PLAYER);
}

Player::Player(const Player& rhs)
    : Character(rhs)
{

}

HRESULT Player::Initialize_Prototype()
{

    return S_OK;
}

HRESULT Player::Initialize(void* arg)
{
    CHECK_FAILED(Character::Initialize(arg), E_FAIL);

    CHECK_FAILED(Ready_PartObjects(), E_FAIL);

    return S_OK;
}

void Player::BeginPlay()
{
    Character::BeginPlay();

    auto& delegate = GAME->Get_DelegateHub();

    if (_weaponTypeChangedHandle.IsValid())
    {
        delegate.OnWeaponTypeChanged.Remove(_weaponTypeChangedHandle);
        _weaponTypeChangedHandle.Reset();
    }

    if (Is_Local())
    {
        _weaponTypeChangedHandle = delegate.OnWeaponTypeChanged.Add(this, &Player::On_WeaponTypeChagned);
    }

    //if (_equipment)
    //    Change_WeaponAttachment(_equipment->Get_CurrentWeaponType());
    Refresh_WeaponAttachment_ByCurrentState();
}

void Player::Priority_Update(float timeDelta)
{
    Character::Priority_Update(timeDelta);
}

void Player::Update(float timeDelta)
{
    Character::Update(timeDelta);

    if (_model)
        _model->Play_Animation(timeDelta, true);

    if (_skill)
        _skill->Update(timeDelta);

    if (_smearEffect)
        _smearEffect->Update_Smear(timeDelta);

    if (_chakraTrail)
        _chakraTrail->Update_ChakraMove(timeDelta);

    if (_swordTrail)
        _swordTrail->Update_SwordTrail(timeDelta);


}

void Player::Late_Update(float timeDelta)
{
    Character::Late_Update(timeDelta);

    if ((_smearEffect && _smearEffect->Has_ActiveSmear()) ||
        (_chakraTrail && _chakraTrail->Has_ActiveChakraMove()) ||
        (_swordTrail && _swordTrail->Has_ActiveSwordTrail()))
    {
        GAME->Add_RenderGroup(ERenderGroup::Blend, this->GetSharedPtr());
    }

}

HRESULT Player::Render()
{
    //Character::Render();

    if (_smearEffect)
        CHECK_FAILED(_smearEffect->Render(), E_FAIL);

    if (_chakraTrail)
        CHECK_FAILED(_chakraTrail->Render(), E_FAIL);

    if (_swordTrail)
        CHECK_FAILED(_swordTrail->Render(), E_FAIL);


    return S_OK;
}

void Player::TakeDamage(const FDamageEvent& damageEvent)
{
    if (auto stateMachine = Get_Component<PlayerStateMachine>())
    {
        switch (stateMachine->Get_CurrentStateID())
        {
        case EPlayerState::Dash:
        case EPlayerState::JumpDash:
        case EPlayerState::WireDash:
            return;
        default:
            break;
        }
    }

    Character::TakeDamage(damageEvent);

    if (_combatStat && _combatStat->Is_Dead())
        return;

    if (_combatStat)
        _combatStat->Take_Damage(damageEvent);

}

void Player::OnDamaged(const FDamageEvent& damageEvent)
{
    Character::OnDamaged(damageEvent);

    Set_RotationToDamageCauser(damageEvent);

    auto sm = Get_Component<PlayerStateMachine>();
    if (sm && _combatStat && !_combatStat->Is_Dead())
    {
        sm->Trigger_HitReaction(
            damageEvent.hitReactionType,
            damageEvent.hitReactionSerial,
            damageEvent.forceHitRestart,
            damageEvent.hitAnimStateOverride); // 스테이트 머신에서 슈퍼아머인지 판단하고 상태 변환
    }
}

void Player::Set_RotationToDamageCauser(const FDamageEvent& damageEvent)
{
    if (!_transformCom)
        return;

    auto damageCauser = damageEvent.damageCauser;
    if (!damageCauser)
        return;

    auto causerTransform = damageCauser->Get_Component<Transform>();
    if (!causerTransform)
        return;

    const Vec3 myPos = _transformCom->Get_WorldPosition();
    Vec3 targetPos = causerTransform->Get_WorldPosition();

    targetPos.y = myPos.y;

    Vec3 lookDir = targetPos - myPos;
    if (lookDir.LengthSquared() <= 0.0001f)
        return;

    lookDir.Normalize();
    _transformCom->LookAt(myPos + lookDir);
}

void Player::OnDead(const FDamageEvent& damageEvent)
{
    Character::OnDead(damageEvent);

    auto sm = Get_Component<PlayerStateMachine>();
    if (sm)
    {
        sm->Force_Enter_State(EPlayerState::Dead);
    }

    // 애니메이션 끝나고 죽이기
    //Set_Destroy(true);
}

void Player::Sync(const Protocol::ObjectInfo& info)
{
    // 서버 권위 위치/회전을 즉시 반영해 최초 스폰과 이후 재동기화를 같은 기준으로 맞춘다.
    _transformCom->Set_LocalPosition(info.pos().x(), info.pos().y(), info.pos().z());
    _transformCom->Set_LocalEulerAngles(info.rot_x(), info.rot_y(), info.rot_z());

    // 서버에서 이름 세팅한거 로컬에도 세팅되게
    if (!info.name().empty())
    {
        Set_PlayerName(Utils::ToWString(info.name()));
    }

    // Hp 갱신도
    if (_combatStat && info.has_stat())
    {
        Protocol::CombatStat stat = info.stat();
        _combatStat->Sync_FromProtobuf(stat);
    }
}

HRESULT Player::Apply_CustomizingPart(EPartSlot slot, const wstring& modelAssetTag)
{
    const bool isUnequipRequest = (modelAssetTag == TEXT("None") || modelAssetTag.empty());

    if (_equipment)
    {
        if (isUnequipRequest)
            _equipment->Unequip_Part(slot);
        else
            _equipment->Equip_Part(slot, modelAssetTag);
    }

    if (isUnequipRequest)
    {
        return Change_PartObject(slot, 0, nullptr);
    }

    if (slot == EPartSlot::Weapon)
    {
        Weapon::FWeaponDesc weaponDesc{};
        weaponDesc.parentTransform = _transformCom;
        weaponDesc.modelAssetTag = modelAssetTag;

        EWeaponType currentWeaponType = EWeaponType::Hand;
        if (_equipment)
            currentWeaponType = _equipment->Get_CurrentWeaponType();

        EPlayerState currentState = EPlayerState::Idle;
        auto stateMachine = Get_Component<PlayerStateMachine>();
        if (stateMachine)
            currentState = stateMachine->Get_CurrentStateID();

        weaponDesc.socketMatrix = Find_WeaponSocketMatrix(currentWeaponType, currentState);

        HRESULT hr = Change_PartObject(slot, Protocol::OBJECT_TYPE_PART_WEAPON, &weaponDesc);

        if (SUCCEEDED(hr))
            Change_WeaponAttachment(currentWeaponType);

        return hr;
    }

    else
    {
        PartObject::FPartObjectDesc partDesc{};
        partDesc.parentTransform = _transformCom;
        partDesc.modelAssetTag = modelAssetTag;
        partDesc.masterPoseModel = _model;
        return Change_PartObject(slot, Protocol::OBJECT_TYPE_PART_OBJECT, &partDesc);
    }
}

HRESULT Player::Ready_Components()
{
    Character::Ready_Components();

    {
        CombatStat::FCombatStatDesc statDesc;
        statDesc.maxHp = 100.f;
        statDesc.attack = 100.f;

        CHECK_FAILED(Add_Component(Protocol::COMPONENT_TYPE_COMBAT_STAT, _combatStat, &statDesc), E_FAIL);
    }

    CHECK_FAILED(Add_Component(Protocol::COMPONENT_TYPE_ANIMATION_STATE, _animState), E_FAIL);

    uint32 testModelKey = static_cast<uint32>(std::hash<string>{}("Model_TestModel"));
    CHECK_FAILED(Add_Component(testModelKey, _model), E_FAIL);

    CHECK_FAILED(Add_Component(Protocol::COMPONENT_TYPE_EQUIPMENT, _equipment), E_FAIL);

    // 대쉬 잔상
    CHECK_FAILED(Add_Component(Protocol::COMPONENT_TYPE_SMEAR_EFFECT, _smearEffect), E_FAIL);

    CHECK_FAILED(Add_Component(Protocol::COMPONENT_TYPE_CHAKRA_MOVE, _chakraTrail), E_FAIL);
    CHECK_FAILED(Add_Component(Protocol::COMPONENT_TYPE_SWORD_TRAIL, _swordTrail), E_FAIL);

    // 스킬
    {
        SkillComponent::FSkillDesc skillDesc{};
        CHECK_FAILED(Add_Component(Protocol::COMPONENT_TYPE_SKILL, _skill, &skillDesc), E_FAIL);
    }

    return S_OK;
}

HRESULT Player::Bind_ShaderResources()
{   
    Matrix worldMatrix = _transformCom->Get_WorldMatrix();
    _shaderCom->Bind_Matrix("g_WorldMatrix", &worldMatrix);
    _shaderCom->Bind_Matrix("g_ViewMatrix", GAME->Get_Transform(ETransformState::View));
    _shaderCom->Bind_Matrix("g_ProjMatrix", GAME->Get_Transform(ETransformState::Proj));

    return S_OK;
}

HRESULT Player::Ready_PartObjects()
{
    // Headgear
    PartObject::FPartObjectDesc headDesc{};
    //headDesc.parentMatrix = &_transformCom->Get_WorldMatrix(); 이제 그냥 Transform 넘기기
    headDesc.parentTransform = _transformCom;
    headDesc.modelAssetTag = TEXT("Model_SnowHead");
    headDesc.masterPoseModel = _model;
    CHECK_FAILED(Add_PartObject(EPartSlot::Headegear, Protocol::OBJECT_TYPE_PART_OBJECT, &headDesc), E_FAIL);

    // Face
    PartObject::FPartObjectDesc faceDesc{};
    faceDesc.parentTransform = _transformCom;
    faceDesc.modelAssetTag = TEXT("Model_Face_Face1");
    faceDesc.masterPoseModel = _model;
    CHECK_FAILED(Add_PartObject(EPartSlot::Face, Protocol::OBJECT_TYPE_PART_OBJECT, &faceDesc), E_FAIL);

    // Onepiece
    PartObject::FPartObjectDesc onePieceDesc{};
    onePieceDesc.parentTransform = _transformCom;
    onePieceDesc.modelAssetTag = TEXT("Model_OnePiece_Armor3");
    onePieceDesc.masterPoseModel = _model;
    CHECK_FAILED(Add_PartObject(EPartSlot::Onepiece, Protocol::OBJECT_TYPE_PART_OBJECT, &onePieceDesc), E_FAIL);

    // 무기 세팅
    {
        Weapon::FWeaponDesc weaponDesc{};
        weaponDesc.parentTransform = _transformCom;
        weaponDesc.modelAssetTag = TEXT("Model_Samehada");

        if (_equipment)
            _equipment->Equip_Part(EPartSlot::Weapon, weaponDesc.modelAssetTag);

        EWeaponType currentWeaponType = EWeaponType::Hand;
        if (_equipment)
            currentWeaponType = _equipment->Get_CurrentWeaponType();

        EPlayerState currentState = EPlayerState::Idle;
        auto stateMachine = Get_Component<PlayerStateMachine>();
        if (stateMachine)
            currentState = stateMachine->Get_CurrentStateID();

        weaponDesc.socketMatrix = Find_WeaponSocketMatrix(currentWeaponType, currentState);

        CHECK_FAILED(Add_PartObject(EPartSlot::Weapon, Protocol::OBJECT_TYPE_PART_WEAPON, &weaponDesc), E_FAIL);
    }

    return S_OK;
}

const Matrix* Player::Find_WeaponSocketMatrix(EWeaponType weaponType, EPlayerState currentState) const
{
    if (!_model)
        return nullptr;

    // 격투형은 계속 등에 유지
    if (weaponType == EWeaponType::Hand)
        return _model->Get_SocketBoneMatrixPtr("Attach_Sword");

    // 검술형이어도 공격 상태가 아니면 등에 유지
    if (!Is_SwordAttackState(currentState))
        return _model->Get_SocketBoneMatrixPtr("Attach_Sword");

    // 검술 공격 상태일 때만 손 소켓 사용
    if (const Matrix* rightWeaponSocket = _model->Get_SocketBoneMatrixPtr("R_Hand_Weapon_cnt_tr"))
        return rightWeaponSocket;

    if (const Matrix* rightHandSocket = _model->Get_SocketBoneMatrixPtr("RightHand"))
        return rightHandSocket;

    return _model->Get_SocketBoneMatrixPtr("Attach_Sword");
}

bool Player::Is_SwordAttackState(EPlayerState state) const
{
    switch (state)
    {
    case EPlayerState::Attack:
    case EPlayerState::JumpAttack:
    case EPlayerState::Attack_Sword_01:
    case EPlayerState::Attack_Sword_02:
    case EPlayerState::Attack_Sword_03:
    case EPlayerState::Attack_Sword_04:
    case EPlayerState::Attack_SwordAir_01:
    case EPlayerState::Attack_SwordAir_02:
        return true;
    default:
        return false;
    }
}

void Player::Refresh_WeaponAttachment_ByCurrentState()
{
    if (!_equipment)
        return;

    Change_WeaponAttachment(_equipment->Get_CurrentWeaponType());
}

void Player::Refresh_WeaponAttachment_ByReplicatedState(EWeaponType weaponType, Protocol::OBJECT_STATE_TYPE replicatedState)
{
    if (_equipment)
    {
        _equipment->Set_WeaponType(weaponType);
    }

    auto weaponPartBase = Get_PartObject(EPartSlot::Weapon);
    if (!weaponPartBase)
        return;

    auto weaponPart = dynamic_pointer_cast<Weapon>(weaponPartBase);
    if (!weaponPart)
        return;

    const EPlayerState attachmentState =
        Is_SwordAttackReplicatedState(replicatedState) ? EPlayerState::Attack : EPlayerState::Idle;

    const Matrix* socketMatrix = Find_WeaponSocketMatrix(weaponType, attachmentState);
    weaponPart->Set_SocketMatrix(socketMatrix);
}

bool Player::Is_SwordAttackReplicatedState(Protocol::OBJECT_STATE_TYPE replicatedState) const
{
    switch (replicatedState)
    {
    case Protocol::OBJECT_STATE_TYPE_ATTACK:
    case Protocol::OBJECT_STATE_TYPE_JUMP_ATTACK:
    case Protocol::OBJECT_STATE_TYPE_ATTACK_SWORD_01:
    case Protocol::OBJECT_STATE_TYPE_ATTACK_SWORD_02:
    case Protocol::OBJECT_STATE_TYPE_ATTACK_SWORD_03:
    case Protocol::OBJECT_STATE_TYPE_ATTACK_SWORD_04:
    case Protocol::OBJECT_STATE_TYPE_ATTACK_SWORD_AIR_01:
    case Protocol::OBJECT_STATE_TYPE_ATTACK_SWORD_AIR_02:
        return true;
    default:
        return false;
    }
}

void Player::Change_WeaponAttachment(EWeaponType weaponType)
{
    auto weaponPartBase = Get_PartObject(EPartSlot::Weapon);
    if (!weaponPartBase)
        return;

    auto weaponPart = dynamic_pointer_cast<Weapon>(weaponPartBase);
    if (!weaponPart)
        return;

    EPlayerState currentState = EPlayerState::Idle;
    auto stateMachine = Get_Component<PlayerStateMachine>();
    if (stateMachine)
        currentState = stateMachine->Get_CurrentStateID();

    const Matrix* socketMatrix = Find_WeaponSocketMatrix(weaponType, currentState);
    weaponPart->Set_SocketMatrix(socketMatrix);
}

Protocol::WEAPON_TYPE Player::To_ProtoWeaponType(EWeaponType weaponType)
{
    switch (weaponType)
    {
    case EWeaponType::BigSwrod:
        return Protocol::WEAPON_TYPE_BIGSWORD;
    case EWeaponType::Hand:
    default:
        return Protocol::WEAPON_TYPE_HAND;
    }
}

EWeaponType Player::From_ProtoWeaponType(Protocol::WEAPON_TYPE weaponType)
{
    switch (weaponType)
    {
    case Protocol::WEAPON_TYPE_BIGSWORD:
        return EWeaponType::BigSwrod;
    case Protocol::WEAPON_TYPE_HAND:
    default:
        return EWeaponType::Hand;
    }
}

void Player::On_WeaponTypeChagned(int32 weaponTypeIndex)
{
    EWeaponType weaponType = static_cast<EWeaponType>(weaponTypeIndex);

    Change_WeaponAttachment(weaponType);
}

Shared<GameObject> Player::Create(ComPtr<Device> device, ComPtr<DeviceContext> context)
{
    auto instance = make_shared<Player>(device, context);

    if (FAILED(instance->Initialize_Prototype()))
    {
        return nullptr;
    }

    return instance;
}

Shared<GameObject> Player::Clone(void* arg)
{
    auto instance = make_shared<Player>(*this);

    if (FAILED(instance->Initialize(arg)))
    {
        return nullptr;
    }

    return instance;
}
