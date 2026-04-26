#include "pch.h"
#include "UI_PlayerSkill.h"
#include "UI_SkillSlot.h"
#include "Player.h"
#include "SkillComponent.h"
#include "SkillDataManager.h"
#include "GameObject_Factory.h"
#include "UI_WeaponType.h"
#include "EquipmentComponent.h"

REGISTER_GAMEOBJECT(UI_PlayerSkill, Protocol::OBJECT_TYPE_UI_PLAYER_SKILL)
IMPLEMENT_REFLECTION(UI_PlayerSkill)

bool UI_PlayerSkill::Register_Properties()
{
    auto& info = GetStaticReflectionInfo();
    info.className = "UI_PlayerSkill";

    PROPERTY_UIOBJECT_FORCE_VISIBLE();

    return true;
}

UI_PlayerSkill::UI_PlayerSkill(ComPtr<Device> device, ComPtr<DeviceContext> context)
    : Panel(device, context)
{
    
}

UI_PlayerSkill::UI_PlayerSkill(const UI_PlayerSkill& rhs)
    : Panel(rhs)
{
}

HRESULT UI_PlayerSkill::Initialize_Prototype()
{
    CHECK_FAILED(Panel::Initialize_Prototype(), E_FAIL);

    return S_OK;
}

HRESULT UI_PlayerSkill::Initialize(void* arg)
{
    CHECK_FAILED(Panel::Initialize(arg), E_FAIL);

    CHECK_FAILED(Ready_Skill(arg), E_FAIL);

    return S_OK;
}

void UI_PlayerSkill::BeginPlay()
{
    Panel::BeginPlay();

   
}

void UI_PlayerSkill::Update(float timeDelta)
{
    Panel::Update(timeDelta);

    auto player = _player.lock();
    if (!player)
        return;

    auto skillCom = player->Get_Component<SkillComponent>();
    if (!skillCom)
        return;

    // Main Skill
    for (int i = 0; i < 2; ++i)
    {
        int skill_Id = skillCom->Get_EquippedSkillID(i);
        const FSkillData* skillData = GET_SINGLE(SkillDataManager)->Get_SkillData(skill_Id);

        if (!skillData)
        {
            _skillSlots[i]->Set_SrvIndex(0);
            _skillSlots[i]->Set_CooldownRatio(0.f);
            continue;
        }

        const uint32 iconSrvIndex =
            GET_SINGLE(SkillDataManager)->Get_SkillIconSrvIndex(skill_Id);

        _skillSlots[i]->Set_SrvIndex(iconSrvIndex);
        _skillSlots[i]->Set_CooldownRatio(skillCom->Get_CooldownRatio(i));
    }

    // Sub Skill
    for (int i = 0; i < 2; ++i)
    {
        const ESubSkillType subSkillType = static_cast<ESubSkillType>(i);
        const int32 subSkill_ID = skillCom->Get_SubSkillID(subSkillType);
        const FSkillData* subSkillData = GET_SINGLE(SkillDataManager)->Get_SkillData(subSkill_ID);

        if (!subSkillData)
        {
            _subSkillSlot[i]->Set_CooldownRatio(0.f);
            continue;
        }

        _subSkillSlot[i]->Set_SrvIndex(subSkillData->uiIconSrvIndex);
        _subSkillSlot[i]->Set_CooldownRatio(skillCom->Get_SubSkillCooldownRatio(subSkillType));
    }
    

}

void UI_PlayerSkill::On_WeaponTypeChanged(int32 weaponTypeIndex)
{
    if (!_weaponTypeUI)
        return;

    EWeaponType newType = static_cast<EWeaponType>(weaponTypeIndex);

    if (newType == EWeaponType::BigSwrod)
        _weaponTypeUI->Set_WeaponType(UI_WeaponType::EWeaponTypeBG::Sword);
    else
        _weaponTypeUI->Set_WeaponType(UI_WeaponType::EWeaponTypeBG::Fighter);
}

void UI_PlayerSkill::Bind_Player(Shared<Player> player)
{
    _player = player;

    // 이미 있으면 해제 후 등록
    if (_weaponTypeHandle.IsValid())
        GAME->Get_DelegateHub().OnWeaponTypeChanged.Remove(_weaponTypeHandle);

    _weaponTypeHandle = GAME->Get_DelegateHub().OnWeaponTypeChanged.Add(
        this, &UI_PlayerSkill::On_WeaponTypeChanged);

    // HUD 바인드 시점에 현재 전투 타입을 즉시 반영해서 초기 표기와 실제 장착 상태를 맞춘다.
    auto equipment = player ? player->Get_Component<EquipmentComponent>() : nullptr;
    if (equipment)
        On_WeaponTypeChanged(static_cast<int32>(equipment->Get_CurrentWeaponType()));
}

HRESULT UI_PlayerSkill::Ready_Skill(void* arg)
{
    // 스킬
    UI_SkillSlot::FSkillSlotDesc leftDesc;
    leftDesc.posX = 0.f;
    leftDesc.posY = 0.f;
    leftDesc.sizeX = 94.f;
    leftDesc.sizeY = 94.f;
    leftDesc.zOrder = _zOrder;
    leftDesc.levelIndex = _levelIndex;

    leftDesc.baseSrvIndex = 0;
    leftDesc.maskSrvIndex = 1;
    leftDesc.iconSrvIndex = 0;

    UI_SkillSlot::FSkillSlotDesc rightDesc = leftDesc;
    rightDesc.posX = 121.8f;

    rightDesc.baseSrvIndex = 0;
    rightDesc.maskSrvIndex = 1;
    rightDesc.iconSrvIndex = 0;

    _skillSlots[0] = Create_Child<UI_SkillSlot>(Protocol::OBJECT_TYPE_UI_SKILL_SLOT, EUILayer::HUD, &leftDesc);
    _skillSlots[1] = Create_Child<UI_SkillSlot>(Protocol::OBJECT_TYPE_UI_SKILL_SLOT, EUILayer::HUD, &rightDesc);

    CHECK_NULL(_skillSlots[0], E_FAIL);
    CHECK_NULL(_skillSlots[1], E_FAIL);

    // 서브 스킬 : 쿠나이 던지기, 바꿔치기
    UI_SkillSlot::FSkillSlotDesc leftSubDesc;
    leftSubDesc.posX = leftDesc.posX - 220.f;
    leftSubDesc.posY = 0.f;
    leftSubDesc.sizeX = 74.f;
    leftSubDesc.sizeY = 74.f;
    leftSubDesc.zOrder = _zOrder;
    leftSubDesc.levelIndex = _levelIndex;

    leftSubDesc.baseSrvIndex = 0;
    leftSubDesc.maskSrvIndex = 1;
    leftSubDesc.iconSrvIndex = 1;
    leftSubDesc.textureComponentType = Protocol::COMPONENT_TYPE_TEXTURE_SKILL_SUB;

    UI_SkillSlot::FSkillSlotDesc rightSubDesc = leftSubDesc;
    rightSubDesc.posX = leftSubDesc.posX + 101.8f;

    rightSubDesc.baseSrvIndex = 0;
    rightSubDesc.maskSrvIndex = 1;
    rightSubDesc.iconSrvIndex = 0;

    _subSkillSlot[0] = Create_Child<UI_SkillSlot>(Protocol::OBJECT_TYPE_UI_SKILL_SLOT, EUILayer::HUD, &leftSubDesc);
    _subSkillSlot[1] = Create_Child<UI_SkillSlot>(Protocol::OBJECT_TYPE_UI_SKILL_SLOT, EUILayer::HUD, &rightSubDesc);

    CHECK_NULL(_subSkillSlot[0], E_FAIL);
    CHECK_NULL(_subSkillSlot[1], E_FAIL);

    // 무기 타입 텍스트
    UI_WeaponType::FWeaponTypeDesc weaponDesc;

    weaponDesc.posX = leftDesc.posX;
    weaponDesc.posY = leftDesc.posY - 120.f;
    weaponDesc.sizeX = 292.f;
    weaponDesc.sizeY = 40.f;
    weaponDesc.zOrder = _zOrder;
    weaponDesc.levelIndex = _levelIndex;

    // 처음에는 격투형으로
    weaponDesc.weaponTypeBG = UI_WeaponType::EWeaponTypeBG::Fighter;
    weaponDesc.textureType = Protocol::COMPONENT_TYPE_TEXTURE_WEAPON_TYPE;

    _weaponTypeUI = Create_Child<UI_WeaponType>(
        Protocol::OBJECT_TYPE_UI_WEAPON_TYPE,
        EUILayer::HUD,
        &weaponDesc);
    CHECK_NULL(_weaponTypeUI, E_FAIL);

    // 제대로 안들어와서, 명시적으로 한번 더 세팅
    _weaponTypeUI->Set_WeaponType(UI_WeaponType::EWeaponTypeBG::Fighter);

    return S_OK;
}

Shared<UI_PlayerSkill> UI_PlayerSkill::Create(ComPtr<Device> device, ComPtr<DeviceContext> context)
{
    auto instance = make_shared<UI_PlayerSkill>(device, context);

    if (FAILED(instance->Initialize_Prototype()))
    {
        MSG_BOX("Failed to Create Prototype : UI_PlayerSkill");
        return nullptr;
    }

    return instance;
}

Shared<GameObject> UI_PlayerSkill::Clone(void* arg)
{
    auto clone = make_shared<UI_PlayerSkill>(*this);

    if (FAILED(clone->Initialize(arg)))
    {
        MSG_BOX("Failed to Cloned : UI_PlayerSkill");

        return nullptr;
    }

    return clone;
}

void UI_PlayerSkill::Free()
{
    if (_weaponTypeHandle.IsValid())
        GAME->Get_DelegateHub().OnWeaponTypeChanged.Remove(_weaponTypeHandle);

    Panel::Free();
}
