#include "pch.h"
#include "UI_BossHp.h"

#include "Background.h"
#include "CombatStat.h"
#include "GameInstance.h"
#include "GameObject_Factory.h"
#include "Shader.h"
#include "Texture.h"
#include "VIBuffer_Rect.h"

REGISTER_GAMEOBJECT(UI_BossHp, Protocol::OBJECT_TYPE_UI_BOSS_HP)

NS_BEGIN(Client)

IMPLEMENT_REFLECTION(UI_BossHp)

bool UI_BossHp::Register_Properties()
{
    auto& info = GetStaticReflectionInfo();
    info.className = "UI_BossHp";

    PROPERTY_UIOBJECT_FORCE_VISIBLE();

    return true;
}

UI_BossHp::UI_BossHp(ComPtr<Device> device, ComPtr<DeviceContext> context)
    : HUD(device, context)
{
}

UI_BossHp::UI_BossHp(const UI_BossHp& rhs)
    : HUD(rhs)
    , _targetHpRatio(rhs._targetHpRatio)
    , _displayHpRatio(rhs._displayHpRatio)
    , _fillStartU(rhs._fillStartU)
    , _fillEndU(rhs._fillEndU)
    , _fillColor(rhs._fillColor)
    , _introStartHpRatio(rhs._introStartHpRatio)
    , _introFillDuration(rhs._introFillDuration)
    , _introElapsed(0.f)
    , _isIntroPlaying(false)
{
    _bossObject.reset();
    _bossCombat.reset();
}

HRESULT UI_BossHp::Initialize_Prototype()
{
    return HUD::Initialize_Prototype();
}

HRESULT UI_BossHp::Initialize(void* arg)
{
    CHECK_FAILED(HUD::Initialize(arg), E_FAIL);
    CHECK_FAILED(Ready_Components(), E_FAIL);

    Set_Visibility(false);

    return S_OK;
}

void UI_BossHp::Update(float timeDelta)
{
    HUD::Update(timeDelta);

    Update_BossBinding(timeDelta);
}

HRESULT UI_BossHp::Render()
{
    if (!Is_VisibleForRender())
        return S_OK;

    CHECK_FAILED(_shaderCom->Bind_Matrix("g_WorldMatrix", &_worldMatrix), E_FAIL);
    CHECK_FAILED(__super::Bind_ShaderResource(_shaderCom, "g_ViewMatrix", ETransformState::View), E_FAIL);
    CHECK_FAILED(__super::Bind_ShaderResource(_shaderCom, "g_ProjMatrix", ETransformState::Proj), E_FAIL);

    CHECK_FAILED(_textureCom->Bind_SRV(_shaderCom, "g_Texture", 1), E_FAIL);
    CHECK_FAILED(_shaderCom->Bind_RawValue("g_BaseColor", &_fillColor, sizeof(Color)), E_FAIL);
    CHECK_FAILED(_shaderCom->Bind_RawValue("g_Alpha", &_opacity, sizeof(float)), E_FAIL);
    CHECK_FAILED(_shaderCom->Bind_RawValue("g_FillRatio", &_displayHpRatio, sizeof(float)), E_FAIL);
    CHECK_FAILED(_shaderCom->Bind_RawValue("g_FillStartU", &_fillStartU, sizeof(float)), E_FAIL);
    CHECK_FAILED(_shaderCom->Bind_RawValue("g_FillEndU", &_fillEndU, sizeof(float)), E_FAIL);

    CHECK_FAILED(_shaderCom->Begin_Pass(3), E_FAIL);
    CHECK_FAILED(_bufferCom->Bind_Resources(), E_FAIL);
    CHECK_FAILED(_bufferCom->Render(), E_FAIL);

    return S_OK;
}

void UI_BossHp::Bind_Boss(Shared<GameObject> bossObject)
{
    Clear_Boss();
    CHECK_NULL(bossObject);

    if (bossObject->Get_ObjectType() != Protocol::OBJECT_TYPE_BOSS_PAIN)
        return;

    _bossObject = bossObject;
    _bossCombat = bossObject->Get_Component<CombatStat>();
    _hpRatio = 1.f;

    if (auto bossCombat = _bossCombat.lock())
    {
        const float targetRatio = clamp(bossCombat->Get_HpRatio(), 0.f, 1.f);
        Begin_IntroFill(targetRatio);
        Set_Visibility(true);
    }
        
}

void UI_BossHp::Clear_Boss()
{
     _bossObject.reset();
    _bossCombat.reset();

    _targetHpRatio = 1.f;
    _displayHpRatio = 1.f;

    _introElapsed = 0.f;
    _isIntroPlaying = false;

    Set_Visibility(false);
}

HRESULT UI_BossHp::Ready_Components()
{
    CHECK_FAILED(Add_Component(Protocol::COMPONENT_TYPE_BOSS_HP, _textureCom), E_FAIL);
    CHECK_FAILED(Add_Component(Protocol::COMPONENT_TYPE_SHADER_UI, _shaderCom), E_FAIL);
    CHECK_FAILED(Add_Component(Protocol::COMPONENT_TYPE_RECT, _bufferCom), E_FAIL);

    return S_OK;
}

HRESULT UI_BossHp::Ready_UI()
{
    

    return S_OK;
}

void UI_BossHp::Update_BossBinding(float timeDelta)
{
    auto bossObject = _bossObject.lock();
    auto bossCombat = _bossCombat.lock();

    if (!bossObject || !bossCombat)
    {
        Clear_Boss();
        return;
    }

    if (bossObject->Is_Destroy() || bossCombat->Is_Dead())
    {
        Clear_Boss();
        return;
    }

    _targetHpRatio = clamp(bossCombat->Get_HpRatio(), 0.f, 1.f);

    if (_isIntroPlaying)
    {
        if (_targetHpRatio <= _introStartHpRatio)
        {
            _displayHpRatio = _targetHpRatio;
            _isIntroPlaying = false;
        }
        else
        {
            _introElapsed += timeDelta;

            const float alpha = clamp(_introElapsed / _introFillDuration, 0.f, 1.f);
            _displayHpRatio = _introStartHpRatio + (_targetHpRatio - _introStartHpRatio) * alpha;

            if (alpha >= 1.f)
            {
                _displayHpRatio = _targetHpRatio;
                _isIntroPlaying = false;
            }
        }
    }
    else
    {
        _displayHpRatio = _targetHpRatio;
    }

    Set_Visibility(true);
}

void UI_BossHp::Begin_IntroFill(float targetHpRatio)
{
    _targetHpRatio = clamp(targetHpRatio, 0.f, 1.f);
    _displayHpRatio = _introStartHpRatio;
    _introElapsed = 0.f;
    _isIntroPlaying = true;

    if (_targetHpRatio <= _introStartHpRatio)
    {
        _displayHpRatio = _targetHpRatio;
        _isIntroPlaying = false;
    }
}

Shared<UI_BossHp> UI_BossHp::Create(ComPtr<Device> device, ComPtr<DeviceContext> context)
{
    auto instance = make_shared<UI_BossHp>(device, context);

    if (FAILED(instance->Initialize_Prototype()))
    {
        MSG_BOX("Failed to Create Prototype : UI_BossHp");
        return nullptr;
    }

    return instance;
}

Shared<GameObject> UI_BossHp::Clone(void* arg)
{
    auto clone = make_shared<UI_BossHp>(*this);

    if (FAILED(clone->Initialize(arg)))
    {
        MSG_BOX("Failed to Clone : UI_BossHp");
        return nullptr;
    }

    return clone;
}

void UI_BossHp::Free()
{
    HUD::Free();
}

NS_END
