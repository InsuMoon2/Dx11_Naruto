#include "pch.h"
#include "SkillComponent.h"
#include "GameObject.h"
#include "CombatStat.h"
#include "SkillDataManager.h"

SkillComponent::SkillComponent(ComPtr<Device> device, ComPtr<DeviceContext> context)
    : Component(device, context)
{
}

SkillComponent::SkillComponent(const SkillComponent& rhs)
    : Component(rhs)    
{
    memcpy(_slotSkill_Id, rhs._slotSkill_Id, sizeof(_slotSkill_Id));
    memcpy(_cooldownRemain, rhs._cooldownRemain, sizeof(_cooldownRemain));
}

HRESULT SkillComponent::Initialize_Prototype()
{

    return S_OK;
}

HRESULT SkillComponent::Initialize(void* arg)
{
    FSkillDesc defaultDesc = {};
    FSkillDesc* desc = arg ? static_cast<FSkillDesc*>(arg) : &defaultDesc;

    for (int i = 0; i < SLOT_COUNT; ++i)
    {
        _slotSkill_Id[i] = desc->slotSkill_Id[i];
        _cooldownRemain[i] = 0.f;
    }

    return S_OK;
}

void SkillComponent::BeginPlay()
{
    Component::BeginPlay();

    auto owner = Get_Owner();
    if (!owner)
        return;

    _combatStat = owner->Get_Component<CombatStat>();
}

void SkillComponent::Update(float timeDelta)
{
    for (int i = 0; i < SLOT_COUNT; i++)
    {
        if (_cooldownRemain[i] <= 0.f)
            continue;

        _cooldownRemain[i] -= timeDelta;

        if (_cooldownRemain[i] < 0.f)
            _cooldownRemain[i] = 0.f;
    }
}

bool SkillComponent::Try_Activate(int slot)
{
    if (slot < 0 || slot >= SLOT_COUNT)
        return false;

    const int id = _slotSkill_Id[slot];
    if (id == 0)
        return false;

    if (_cooldownRemain[slot] > 0.f)
        return false;

    auto skillDataPtr = GET_SINGLE(SkillDataManager)->Get_SkillData(id);
    if (!skillDataPtr)
    {
        LOG_WARN("SkillData not found. id={}", id);
        return false;
    }

    _cooldownRemain[slot] = max(0.f, skillDataPtr->coolDown);

    return true;
}

int SkillComponent::Get_EquippedSkillID(int slot) const
{
    if (slot < 0 || slot >= SLOT_COUNT)
        return 0;

    return _slotSkill_Id[slot];
}

float SkillComponent::Get_CooldownRatio(int slot) const
{
    if (slot < 0 || slot >= SLOT_COUNT)
        return 0.f;

    const int id = _slotSkill_Id[slot];
     
    auto skillDataPtr = GET_SINGLE(SkillDataManager)->Get_SkillData(id);
    if (!skillDataPtr || skillDataPtr->coolDown <= 0.f)
        return 0.f;

    return ::clamp(_cooldownRemain[slot] / skillDataPtr->coolDown, 0.f, 1.f);
}

json SkillComponent::To_Json() const
{
    json root = Component::To_Json();
    root["slotSkillID"] = { _slotSkill_Id[0], _slotSkill_Id[1] };

    return root;
}

void SkillComponent::From_Json(const json& data)
{
    Component::From_Json(data);

    if (data.contains("slotSkillID") && data["slotSkillID"].is_array() && data["slotSkillID"].size() >= 2)
    {
        _slotSkill_Id[0] = data["slotSkillID"][0].get<int>();
        _slotSkill_Id[1] = data["slotSkillID"][1].get<int>();
    }
}

Shared<SkillComponent> SkillComponent::Create(ComPtr<Device> device, ComPtr<DeviceContext> context)
{
    auto instance = make_shared<SkillComponent>(device, context);

    if (FAILED(instance->Initialize_Prototype()))
    {
        MSG_BOX("Failed to Create : SkillComponent");

        return nullptr;
    }

    return instance;
}

Shared<Component> SkillComponent::Clone(void* arg)
{
    auto clone = make_shared<SkillComponent>(*this);

    if (FAILED(clone->Initialize(arg)))
    {
        MSG_BOX("Failed to Clone : SkillComponent");

        return nullptr;
    }

    return clone;
}

void SkillComponent::Free()
{
    Component::Free();
}
