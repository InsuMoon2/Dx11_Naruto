#include "pch.h"
#include "UI_PlayerStatus.h"
#include "Shader.h"
#include "Texture.h"
#include "VIBuffer_Rect.h"
#include "UI_Text.h"
#include "Player.h"
#include "UI_PlayerHP.h"
#include "CombatStat.h"
#include "GameObject_Factory.h"
#include "SkillComponent.h"
#include "SkillDataManager.h"

REGISTER_GAMEOBJECT(UI_PlayerStatus, Protocol::OBJECT_TYPE_UI_PLAYER_STATUS)

UI_PlayerStatus::UI_PlayerStatus(ComPtr<Device> device, ComPtr<DeviceContext> context)
    : Panel(device, context)
{
    
}

UI_PlayerStatus::UI_PlayerStatus(const UI_PlayerStatus& rhs)
    : Panel(rhs)
{
    _player.reset();
    _combat.reset();
    _hpBar = nullptr;
    _ultimateSlot = nullptr;
}

HRESULT UI_PlayerStatus::Initialize_Prototype()
{
    return Panel::Initialize_Prototype();
}

HRESULT UI_PlayerStatus::Initialize(void* arg)
{
    CHECK_FAILED(Panel::Initialize(arg), E_FAIL);
    CHECK_FAILED(Ready_Components(), E_FAIL);

    UI_PlayerHP::FPlayerHPDesc hpDesc;
    hpDesc.posX = 0.f;
    hpDesc.posY = 0.f;
    hpDesc.zOrder = _zOrder + 0.01f;
    hpDesc.levelIndex = _levelIndex;
    hpDesc.textureIndex = 0;
    hpDesc.textureType = Protocol::COMPONENT_TYPE_TEXTURE_DEFAULT;

    _hpBar = Create_Child<UI_PlayerHP>(Protocol::OBJECT_TYPE_UI_PLAYER_HP, EUILayer::HUD, &hpDesc);
    if (!_hpBar) return E_FAIL;

    _hpBar->Set_FillRange(98.f / 512.f, 413.f / 512.f);
    _hpBar->Get_Transform()->Set_LocalPosition(53.5f, 25.2f, 0.f);

    // 궁 슬롯
    {
        UI_SkillSlot::FSkillSlotDesc ultiDesc;
        ultiDesc.posX = 0.f;
        ultiDesc.posY = 0.f;
        ultiDesc.sizeX = 82.f;
        ultiDesc.sizeY = 82.f;
        ultiDesc.zOrder = _zOrder + 0.02f;
        ultiDesc.levelIndex = _levelIndex;

        ultiDesc.baseSrvIndex = 0;
        ultiDesc.maskSrvIndex = 1;
        ultiDesc.iconSrvIndex = 0;
        ultiDesc.textureComponentType = Protocol::COMPONENT_TYPE_TEXTURE_SKILL_ICON;

        _ultimateSlot = Create_Child<UI_SkillSlot>(
            Protocol::OBJECT_TYPE_UI_SKILL_SLOT,
            EUILayer::HUD,
            &ultiDesc);

        CHECK_NULL(_ultimateSlot, E_FAIL);

        _ultimateSlot->Get_Transform()->Set_LocalPosition(-157.7f, 4.6f, 0.f);
    }

    return S_OK;
}

void UI_PlayerStatus::Priority_Update(float timeDelta)
{
    Panel::Priority_Update(timeDelta);
}

void UI_PlayerStatus::Update(float timeDelta)
{
    Panel::Update(timeDelta);

    __super::Update_Transform();

    auto combat = _combat.lock();
    if (!combat || !_hpBar)
        return;

    _hpBar->Set_Ratio(combat->Get_HpRatio());

    auto skill = _skill.lock();
    if (!skill || !_ultimateSlot)
        return;

    const int32 ultimateSkill_ID = skill->Get_EquippedSkillID(2);
    const FSkillData* ultimateSkillData =
        GET_SINGLE(SkillDataManager)->Get_SkillData(ultimateSkill_ID);

    if (!ultimateSkillData)
    {
        _ultimateSlot->Set_SrvIndex(0);
        _ultimateSlot->Set_CooldownRatio(0.f);
        return;
    }

    const uint32 iconSrvIndex =
        GET_SINGLE(SkillDataManager)->Get_SkillIconSrvIndex(ultimateSkill_ID);

    _ultimateSlot->Set_SrvIndex(iconSrvIndex);
    _ultimateSlot->Set_CooldownRatio(skill->Get_CooldownRatio(2));
}

void UI_PlayerStatus::Late_Update(float timeDelta)
{
    Panel::Late_Update(timeDelta);
}

HRESULT UI_PlayerStatus::Render()
{
    if (!_isVisible) return S_OK;

    _shaderCom->Bind_Matrix("g_WorldMatrix", &_worldMatrix);

    Panel::Bind_ShaderResource(_shaderCom, "g_ViewMatrix", ETransformState::View);
    Panel::Bind_ShaderResource(_shaderCom, "g_ProjMatrix", ETransformState::Proj);

    CHECK_FAILED(_textureCom->Bind_SRV(_shaderCom, "g_Texture", 0), E_FAIL);

#pragma region Alpha값 테스트
    float fAlpha = 1.f;
    _shaderCom->Bind_RawValue("g_Alpha", &fAlpha, sizeof(float));
#pragma endregion

    CHECK_FAILED(_shaderCom->Begin_Pass(0), E_FAIL);
    CHECK_FAILED(_bufferCom->Bind_Resources(), E_FAIL);
    CHECK_FAILED(_bufferCom->Render(), E_FAIL);

    return S_OK;
}

void UI_PlayerStatus::Bind_Player(Shared<Player> player)
{
    _player = player;

    if (player)
    {
        _combat = player->Get_Component<CombatStat>();
        _skill = player->Get_Component<SkillComponent>();
    }
    else
    {
        _combat.reset();
        _skill.reset();
    }
}

HRESULT UI_PlayerStatus::Ready_Components()
{
    CHECK_FAILED(Add_Component(Protocol::COMPONENT_TYPE_TEXTURE_PLAYER_STATUS, _textureCom), E_FAIL);
    CHECK_FAILED(Add_Component(Protocol::COMPONENT_TYPE_SHADER_UI, _shaderCom), E_FAIL);
    CHECK_FAILED(Add_Component(Protocol::COMPONENT_TYPE_RECT, _bufferCom), E_FAIL);

    return S_OK;
}

Shared<UI_PlayerStatus> UI_PlayerStatus::Create(ComPtr<Device> device, ComPtr<DeviceContext> context)
{
    auto instance = make_shared<UI_PlayerStatus>(device, context);

    if (FAILED(instance->Initialize_Prototype()))
    {
        MSG_BOX("Failed to Create Prototype : UI_PlayerStatus");
        instance->Free();

        return nullptr;
    }

    return instance;
}

Shared<GameObject> UI_PlayerStatus::Clone(void* arg)
{
    auto clone = make_shared<UI_PlayerStatus>(*this);

    if (FAILED(clone->Initialize(arg)))
    {
        MSG_BOX("Failed to Cloned : UI_PlayerStatus");

        return nullptr;
    }

    return clone;
}

void UI_PlayerStatus::Free()
{
    Panel::Free();

}
