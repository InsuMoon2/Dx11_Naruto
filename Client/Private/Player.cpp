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

    _weaponTypeChangedHandle = delegate.OnWeaponTypeChanged.Add(this, &Player::On_WeaponTypeChagned);

    if (_equipment)
        Change_WeaponAttachment(_equipment->Get_CurrentWeaponType());
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
}

void Player::Late_Update(float timeDelta)
{
    Character::Late_Update(timeDelta);

    if (_collider)
        _collider->Update_Collider(_transformCom->Get_WorldMatrix());
}

HRESULT Player::Render()
{
    //Character::Render();

    return S_OK;
}

void Player::Sync(const Protocol::ObjectInfo& info)
{
    _transformCom->Set_LocalPosition(info.pos().x(), info.pos().y(), info.pos().z());
}

HRESULT Player::Apply_CustomizingPart(EPartSlot slot, const wstring& modelAssetTag)
{
    if (_equipment)
    {
        // 장착 해제
        if (modelAssetTag == TEXT("None") || modelAssetTag.empty())
            _equipment->Unequip_Part(slot);
        else
            _equipment->Equip_Part(slot, modelAssetTag);
    }

    // 슬롯이 무기인 경우
    if (slot == EPartSlot::Weapon)
    {
        Weapon::FWeaponDesc weaponDesc{};
        weaponDesc.parentTransform = _transformCom;
        weaponDesc.modelAssetTag = modelAssetTag;

        EWeaponType currentWeaponType = EWeaponType::Hand;
        if (_equipment)
            currentWeaponType = _equipment->Get_CurrentWeaponType();

        weaponDesc.socketMatrix = Find_WeaponSocketMatrix(currentWeaponType);

        HRESULT hr = Change_PartObject(slot, Protocol::OBJECT_TYPE_PART_WEAPON, &weaponDesc);

        if (SUCCEEDED(hr))
            Change_WeaponAttachment(currentWeaponType);

        return hr;
    }

    // 일반 파츠인 경우
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

    // 충돌체 추가
    Bounding_Capsule::FBoundingCapsuleDesc capsuleDesc{};
    capsuleDesc.radius = 0.5f;
    capsuleDesc.halfHeight = 0.3f;

    CHECK_FAILED(Add_Component(Protocol::COMPONENT_TYPE_COLLIDER_CAPSULE, _collider, &capsuleDesc), E_FAIL);
    _collider->Set_CollisionPreset(Collision_Preset::Player);
    GAME->Add_Collider(_collider);

   /* Bounding_AABB::FBoundingAABBDesc aabbDesc{};
    aabbDesc.extents = Vec3(0.4f, 0.6f, 0.4f);
    aabbDesc.center = Vec3(0.f, aabbDesc.extents.y, 0.f);

    CHECK_FAILED(Add_Component(Protocol::COMPONENT_TYPE_COLLIDER_AABB, _collider, &aabbDesc), E_FAIL);
    _collider->Set_CollisionPreset(Collision_Preset::Player);
    GAME->Add_Collider(_collider);*/

    return S_OK;
}

HRESULT Player::Bind_ShaderResources()
{   
    Matrix worldMatrix = _transformCom->Get_WorldMatrix();
    _shaderCom->Bind_Matrix("g_WorldMatrix", &worldMatrix);
    _shaderCom->Bind_Matrix("g_ViewMatrix", GAME->Get_Transform(ETransformState::View));
    _shaderCom->Bind_Matrix("g_ProjMatrix", GAME->Get_Transform(ETransformState::Proj));

    GAME->Bind_CamPosition(_shaderCom, "g_vCamPosition");
    CHECK_FAILED(Bind_Lights(), E_FAIL);

    return S_OK;
}

HRESULT Player::Bind_Lights()
{
    const FLightDesc* lightDesc = GAME->Get_LightDesc(0);

    FLightDesc defaultLight;
    if (!lightDesc)
    {
        defaultLight.direction = Vec4(0.f, -1.f, 1.f, 0.f);
        defaultLight.diffuse = Vec4(1.f, 1.f, 1.f, 1.f);
        defaultLight.ambient = Vec4(0.4f, 0.4f, 0.4f, 1.f);
        defaultLight.specular = Vec4(1.f, 1.f, 1.f, 1.f);
        lightDesc = &defaultLight;
    }

    CHECK_NULL(lightDesc, E_FAIL);

    CHECK_FAILED(_shaderCom->Bind_RawValue("g_vLightDir", &lightDesc->direction, sizeof(Vec4)), E_FAIL);
    CHECK_FAILED(_shaderCom->Bind_RawValue("g_vLightDiffuse", &lightDesc->diffuse, sizeof(Vec4)), E_FAIL);
    CHECK_FAILED(_shaderCom->Bind_RawValue("g_vLightAmbient", &lightDesc->ambient, sizeof(Vec4)), E_FAIL);
    CHECK_FAILED(_shaderCom->Bind_RawValue("g_vLightSpecular", &lightDesc->specular, sizeof(Vec4)), E_FAIL);

    return S_OK;
}

HRESULT Player::Ready_PartObjects()
{
    // TODO : 추가될 파츠 : Headgear, Face, Body Upper, Body Lower, Weapon

    // Headgear
    PartObject::FPartObjectDesc headDesc{};
    //headDesc.parentMatrix = &_transformCom->Get_WorldMatrix();
    headDesc.parentTransform = _transformCom;
    headDesc.modelAssetTag = TEXT("Model_Headgear_Man_Cap1");
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
    onePieceDesc.modelAssetTag = TEXT("Model_Body_Upper_Coat15");
    onePieceDesc.masterPoseModel = _model;
    CHECK_FAILED(Add_PartObject(EPartSlot::Onepiece, Protocol::OBJECT_TYPE_PART_OBJECT, &onePieceDesc), E_FAIL);

    // 무기 세팅
    {
        Weapon::FWeaponDesc weaponDesc{};
        weaponDesc.parentTransform = _transformCom;
        weaponDesc.modelAssetTag = TEXT("Model_BigSword");

        EWeaponType currentWeaponType = EWeaponType::Hand;
        if (_equipment)
            currentWeaponType = _equipment->Get_CurrentWeaponType();

        weaponDesc.socketMatrix = Find_WeaponSocketMatrix(currentWeaponType);

        CHECK_FAILED(Add_PartObject(EPartSlot::Weapon, Protocol::OBJECT_TYPE_PART_WEAPON, &weaponDesc), E_FAIL);
    }

    return S_OK;
}

const Matrix* Player::Find_WeaponSocketMatrix(EWeaponType weaponType) const
{
    if (!_model)
        return nullptr;

    if (weaponType == EWeaponType::Hand)
        return _model->Get_SocketBoneMatrixPtr("Attach_Sword"); // Hand면 등 뒤 소켓으로

    if (const Matrix* rightWeaponSocket = _model->Get_SocketBoneMatrixPtr("R_Hand_Weapon_cnt_tr"))
        return rightWeaponSocket;

    // Temp : 만약, 뼈대가 없는 모델이라면 그냥 오른손에 부착해보기 -> 플레이어 모델은 존재함 
    if (const Matrix* rightHandSocket = _model->Get_SocketBoneMatrixPtr("RightHand"))
        return rightHandSocket;

    // 오른손도 없다면, 등 뒤에 그대로
    return _model->Get_SocketBoneMatrixPtr("Attach_Sword");
}

void Player::Change_WeaponAttachment(EWeaponType weaponType)
{
    auto weaponPartBase = Get_PartObject(EPartSlot::Weapon);
    if (!weaponPartBase)
        return;

    auto weaponPart = dynamic_pointer_cast<Weapon>(weaponPartBase);
    if (!weaponPart)
        return;

    const Matrix* socketMatrix = Find_WeaponSocketMatrix(weaponType);
    weaponPart->Set_SocketMatrix(socketMatrix);
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
