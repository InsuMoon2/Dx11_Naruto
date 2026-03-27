#include "pch.h"
#include "Player.h"
#include "CombatStat.h"
#include "Shader.h"
#include "Model.h"
#include "PartObject.h"
#include "AnimationStateComponent.h"
#include "Weapon.h"

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
    if (modelAssetTag.empty())
        return Change_PartObject(slot, Protocol::OBJECT_TYPE_PART_OBJECT, nullptr);

    PartObject::FPartObjectDesc partDesc{};
    //partDesc.parentMatrix = &_transformCom->Get_WorldMatrix();
    partDesc.parentTransform = _transformCom;
    partDesc.modelAssetTag = modelAssetTag;
    partDesc.masterPoseModel = _model;

    return Change_PartObject(slot, Protocol::OBJECT_TYPE_PART_OBJECT, &partDesc);
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

    // 소켓 생성해서 무기 붙이기
    {
        Weapon::FWeaponDesc weaponDesc{};
        weaponDesc.parentTransform = _transformCom;
        weaponDesc.modelAssetTag = TEXT("Model_BigSword");
        weaponDesc.socketMatrix = _model->Get_SocketBoneMatrixPtr("Attach_Sword");

        CHECK_FAILED(Add_PartObject(EPartSlot::Weapon, Protocol::OBJECT_TYPE_PART_WEAPON, &weaponDesc), E_FAIL);
    }

    return S_OK;
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
