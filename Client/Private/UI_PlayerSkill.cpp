#include "pch.h"
#include "UI_PlayerSkill.h"
#include "UI_SkillSlot.h"
#include "Player.h"
#include "SkillComponent.h"
#include "SkillDataManager.h"

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

    UI_SkillSlot::FSkillSlotDesc leftDesc;
    leftDesc.posX = 0.f;
    leftDesc.posY = 0.f;
    leftDesc.sizeX = 72.f;
    leftDesc.sizeY = 72.f;
    leftDesc.zOrder = _zOrder;
    leftDesc.levelIndex = _levelIndex;
    leftDesc.baseSrvIndex = 0;
    leftDesc.iconSrvIndex = 2;

    UI_SkillSlot::FSkillSlotDesc rightDesc = leftDesc;
    rightDesc.posX = 86.f;
    rightDesc.baseSrvIndex = 1;
    rightDesc.iconSrvIndex = 3;

    _slots[0] = Create_Child<UI_SkillSlot>(EUILayer::HUD, &leftDesc);
    _slots[1] = Create_Child<UI_SkillSlot>(EUILayer::HUD, &rightDesc);

    CHECK_NULL(_slots[0], E_FAIL);
    CHECK_NULL(_slots[1], E_FAIL);
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

    for (int i = 0; i < 2; ++i)
    {
        int id = skillCom->Get_EquippedSkillID(i);
        const FSkillData* skillData = GET_SINGLE(SkillDataManager)->Get_SkillData(id);

        if (!skillData)
        {
            _slots[i]->Set_CooldownRatio(0.f);
            continue;
        }

        _slots[i]->Set_SrvIndex(skillData->srvIndex);
        _slots[i]->Set_CooldownRatio(skillCom->Get_CooldownRatio(i));
    }
}

Shared<UI_PlayerSkill> UI_PlayerSkill::Create(ComPtr<Device> device, ComPtr<DeviceContext> context, void* arg)
{
    auto instance = make_shared<UI_PlayerSkill>(device, context);

    if (FAILED(instance->Initialize(arg)))
    {
        MSG_BOX("Failed to Created : UI_PlayerSkill");
        return nullptr;
    }

    return instance;
}

void UI_PlayerSkill::Free()
{
    Panel::Free();
}
