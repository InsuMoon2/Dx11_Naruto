#include "pch.h"
#include "UI_MonsterHp.h"

#include "CombatStat.h"
#include "GameInstance.h"
#include "Texture.h"
#include "Shader.h"
#include "VIBuffer_Rect.h"
#include "GameObject_Factory.h"
#include "Monster.h"

REGISTER_GAMEOBJECT(UI_MonsterHp, Protocol::OBJECT_TYPE_UI_MONSTER_HP)

UI_MonsterHp::UI_MonsterHp(ComPtr<Device> device, ComPtr<DeviceContext> context)
    : UIObject(device, context)
{
    
}

UI_MonsterHp::UI_MonsterHp(const UI_MonsterHp& rhs)
    : UIObject(rhs)
    , _hpRatio(rhs._hpRatio)
    , _fillStartU(rhs._fillStartU)
    , _fillEndU(rhs._fillEndU)
    , _fillRangeInitialized(rhs._fillRangeInitialized)
{
}

HRESULT UI_MonsterHp::Initialize_Prototype()
{
    return UIObject::Initialize_Prototype();
}

HRESULT UI_MonsterHp::Initialize(void* arg)
{
    CHECK_FAILED(UIObject::Initialize(arg), E_FAIL);

    CHECK_FAILED(Ready_Components(), E_FAIL);


    // 위치를 몬스터의 머리통 위에
    _transformCom->Set_LocalScale(256.f, 32.f, 0.f);

    return S_OK;
}

void UI_MonsterHp::Priority_Update(float timeDelta)
{
    UIObject::Priority_Update(timeDelta);
}

void UI_MonsterHp::Update(float timeDelta)
{
    UIObject::Update(timeDelta);

	auto monster = _monster.lock();
	auto combat = _combat.lock();

	if (!monster || !combat || monster->Is_Destroy() || combat->Is_Dead())
    {
        _isVisible = false;
        return;
    }

	_isVisible = true;
	Set_Ratio(combat->Get_HpRatio());

	Vec3 worldPos = monster->Get_Transform()->Get_WorldPosition();
	worldPos.y += 1.f;

	Matrix viewMatrix = *GAME->Get_Transform(ETransformState::View);
	Matrix projMatrix = *GAME->Get_Transform(ETransformState::Proj);

	// 월드좌표 -> 투영공간
	Vec4 clipSpace = XMVector4Transform(Vec4(worldPos.x, worldPos.y, worldPos.z, 1.f), viewMatrix * projMatrix);

	// 카메라 뒤 또는 시야에서 벗어나면 제거
    if (clipSpace.w <= 0.f)
    {
        _isVisible = false;
    }
    else
    {
        float ndcX = clipSpace.x / clipSpace.w;
		float ndcY = clipSpace.y / clipSpace.w;

		float width = GAME->Get_UIReferenceWidth();
		float height = GAME->Get_UIReferenceHeight();

		float screenX = (ndcX * 0.5f + 0.5f) * width;
		float screenY = -(ndcY * 0.5f - 0.5f) * height;

		// 거리 비례 크기 조절
		float distanceScale = 10.f / clipSpace.w;
		distanceScale = ::clamp(distanceScale, 0.4f, 1.f);

		Set_UIPosition(screenX, screenY);
		Set_UIScale(256.f * distanceScale, 32.f * distanceScale);
    }


	// 위치 갱신
    __super::Update_Transform();
}

void UI_MonsterHp::Late_Update(float timeDelta)
{
    UIObject::Late_Update(timeDelta);

}

HRESULT UI_MonsterHp::Render()
{
    if (!_isVisible) return S_OK;

    CHECK_FAILED(_shaderCom->Bind_Matrix("g_WorldMatrix", &_worldMatrix), E_FAIL);
    CHECK_FAILED(__super::Bind_ShaderResource(_shaderCom, "g_ViewMatrix", ETransformState::View), E_FAIL);
    CHECK_FAILED(__super::Bind_ShaderResource(_shaderCom, "g_ProjMatrix", ETransformState::Proj), E_FAIL);

    CHECK_FAILED(_textureCom->Bind_SRV(_shaderCom, "g_Texture", 1), E_FAIL);

    // HP Body
    {
        CHECK_FAILED(_textureCom->Bind_SRV(_shaderCom, "g_Texture", 2), E_FAIL);

        Vec4 hpColor = { 0.8f, 0.2f, 0.f, 1.f };
        CHECK_FAILED(_shaderCom->Bind_RawValue("g_BaseColor", &hpColor, sizeof(Vec4)), E_FAIL);

        float alpha = 1.f;
        CHECK_FAILED(_shaderCom->Bind_RawValue("g_Alpha", &alpha, sizeof(float)), E_FAIL);

        CHECK_FAILED(_shaderCom->Begin_Pass(1), E_FAIL);
        CHECK_FAILED(_bufferCom->Bind_Resources(), E_FAIL);
        CHECK_FAILED(_bufferCom->Render(), E_FAIL);
    }

    // HP Bar
    {
        CHECK_FAILED(_textureCom->Bind_SRV(_shaderCom, "g_Texture", 1), E_FAIL);

        float alpha = 1.f;
        CHECK_FAILED(_shaderCom->Bind_RawValue("g_Alpha", &alpha, sizeof(float)), E_FAIL);

        Vec4 hpColor = { 0.3f, 1.f, 0.f, 1.f };
        CHECK_FAILED(_shaderCom->Bind_RawValue("g_BaseColor", &hpColor, sizeof(Vec4)), E_FAIL);
        _shaderCom->Bind_RawValue("g_FillRatio", &_hpRatio, sizeof(float));
        _shaderCom->Bind_RawValue("g_FillStartU", &_fillStartU, sizeof(float));
        _shaderCom->Bind_RawValue("g_FillEndU", &_fillEndU, sizeof(float));

        CHECK_FAILED(_shaderCom->Begin_Pass(3), E_FAIL);

        CHECK_FAILED(_bufferCom->Bind_Resources(), E_FAIL);
        CHECK_FAILED(_bufferCom->Render(), E_FAIL);
    }

    return S_OK;
}

void UI_MonsterHp::Bind_Monster(Shared<Monster> monster)
{
	_monster = monster;

     if (monster)
        _combat = monster->Get_Component<CombatStat>();
    else
        _combat.reset();
}

HRESULT UI_MonsterHp::Ready_Components()
{
    CHECK_FAILED(Add_Component(Protocol::COMPONENT_TYPE_TEXTURE_PLAYER_STATUS, _textureCom), E_FAIL);
    CHECK_FAILED(Add_Component(Protocol::COMPONENT_TYPE_SHADER_UI, _shaderCom), E_FAIL);
    CHECK_FAILED(Add_Component(Protocol::COMPONENT_TYPE_RECT, _bufferCom), E_FAIL);

    return S_OK;
}

Shared<UI_MonsterHp> UI_MonsterHp::Create(ComPtr<Device> device, ComPtr<DeviceContext> context)
{
    auto instance = make_shared<UI_MonsterHp>(device, context);

    if (FAILED(instance->Initialize_Prototype()))
    {
        MSG_BOX("Failed to Create Prototype : UI_MonsterHp");
        return nullptr;
    }

    return instance;
}

Shared<GameObject> UI_MonsterHp::Clone(void* arg)
{
    auto clone = make_shared<UI_MonsterHp>(*this);

    if (FAILED(clone->Initialize(arg)))
    {
        MSG_BOX("Failed to Cloned : UI_MonsterHp");

        return nullptr;
    }

    return clone;
}

void UI_MonsterHp::Free()
{
    UIObject::Free();

}
