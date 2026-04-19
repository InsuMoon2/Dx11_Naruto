#include "pch.h"
#include "SkillComponent.h"
#include "GameObject.h"
#include "CombatStat.h"
#include "SkillDataManager.h"
#include "SkillObject.h"
#include "SkillObject_Projectile.h"
#include "Model.h"
#include "EquipmentComponent.h"

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

    for (int i = 0; i < SUB_SKILL_COUNT; ++i)
    {
        _subSkillIds[i] = 0;
        _subSkillCooldownRemain[i] = 0.f;
    }

    return S_OK;
}

void SkillComponent::BeginPlay()
{
    Component::BeginPlay();

    auto owner = Get_Owner();
    if (!owner)
        return;

    _equipment = owner->Get_Component<EquipmentComponent>();
    CHECK_NULL(_equipment.lock());

    Apply_WeaponSkillSet(_equipment.lock()->Get_CurrentWeaponType());

    _combatStat = owner->Get_Component<CombatStat>();
    CHECK_NULL(_combatStat.lock());

    // Sub SKill
    {
        const FSkillData* replacementData =
            GET_SINGLE(SkillDataManager)->Find_SkillByCategoryAndSlot(ESkillCategory::Sub, 0);

        if (replacementData)
            _subSkillIds[To_SubSkillIndex(ESubSkillType::Replacement)] = static_cast<int32>(replacementData->skill_Id);

        const FSkillData* shurikenData =
            GET_SINGLE(SkillDataManager)->Find_SkillByCategoryAndSlot(ESkillCategory::Sub, 1);

        if (shurikenData)
            _subSkillIds[To_SubSkillIndex(ESubSkillType::Shuriken)] = static_cast<int32>(shurikenData->skill_Id);
    }

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

    for (int i = 0; i < SUB_SKILL_COUNT; ++i)
    {
        if (_subSkillCooldownRemain[i] <= 0.f)
            continue;

        _subSkillCooldownRemain[i] -= timeDelta;

        if (_subSkillCooldownRemain[i] < 0.f)
            _subSkillCooldownRemain[i] = 0.f;
    }

    auto meleeSkill = _attachedMeleeSkill.lock();
    if (meleeSkill && !meleeSkill->Is_Destroy())
    {
        auto model = Get_Owner()->Get_Component<Model>();
        if (model)
        {
            const Matrix* boneMatrix = model->Get_SocketBoneMatrixPtr(_attachedBoneName);
            if (boneMatrix)
            {
                Matrix boneWorld = (*boneMatrix) * Get_Owner()->Get_Transform()->Get_WorldMatrix();
                meleeSkill->Sync_AttachedTransform(boneWorld);
            }
        }
    }
    else if (!_attachedMeleeSkill.expired())
    {
        Clear_MeleeSkill();
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

void SkillComponent::Equip_MeleeSkill(Protocol::OBJECT_TYPE type, Shared<SkillObject> skill, const string& boneName)
{
    Clear_MeleeSkill();

    _attachedMeleeSkill = skill;
    _attachedBoneName = boneName;
}

void SkillComponent::Clear_MeleeSkill()
{
    auto skill = _attachedMeleeSkill.lock();

    if (skill && !skill->Is_Destroy())
    {
        skill->Set_Destroy(true);
    }

    _attachedMeleeSkill.reset();
}

bool SkillComponent::Can_ActivateSubSkill(ESubSkillType type) const
{
    const int32 index = To_SubSkillIndex(type);
    if (index < 0)
        return false;

    const int32 skill_ID = _subSkillIds[index];
    if (skill_ID == 0)
        return false;

    if (_subSkillCooldownRemain[index] > 0.f)
        return false;

    return true;
}

void SkillComponent::Start_SubSkillCooldown(ESubSkillType type)
{
    const int32 index = To_SubSkillIndex(type);
    if (index < 0)
        return;

    const int32 skillID = _subSkillIds[index];
    if (skillID == 0)
        return;

    const FSkillData* skillData = GET_SINGLE(SkillDataManager)->Get_SkillData(skillID);
    if (!skillData)
    {
        LOG_WARN("SubSkillData not found. id={}", skillID);
        return;
    }

    _subSkillCooldownRemain[index] = max(0.f, skillData->coolDown);
}

int32 SkillComponent::Get_SubSkillID(ESubSkillType type) const
{
    const int32 index = To_SubSkillIndex(type);
    if (index < 0)
        return 0;

    return _subSkillIds[index];
}

float SkillComponent::Get_SubSkillCooldownRatio(ESubSkillType type) const
{
    const int32 index = To_SubSkillIndex(type);
    if (index < 0)
        return 0.f;

    const int32 skillID = _subSkillIds[index];
    if (skillID == 0)
        return 0.f;

    const FSkillData* skillData = GET_SINGLE(SkillDataManager)->Get_SkillData(skillID);
    if (!skillData || skillData->coolDown <= 0.f)
        return 0.f;

    return ::clamp(_subSkillCooldownRemain[index] / skillData->coolDown, 0.f, 1.f);
}

void SkillComponent::Set_PendingSkill(Protocol::OBJECT_TYPE type, Shared<SkillObject_Projectile> skill)
{
    auto iter = _pendingSkills.find(type);
    if (iter != _pendingSkills.end())
    {
        if (!iter->second.expired())
        {
            LOG_WARN("SkillComponent::Set_PendingSkill - 타입({})으로 이미 PendingSkill이 등록되어 있습니다. 덮어씁니다.", ETOI(type));
        }
    }

    _pendingSkills[type] = skill;
}

bool SkillComponent::Launch_PendingSkill(Protocol::OBJECT_TYPE type, const Vec3& direction)
{
    auto iter = _pendingSkills.find(type);
    if (iter == _pendingSkills.end())
    {
        LOG_WARN("SkillComponent::Launch_PendingSkill - 타입({})의 PendingSkill이 없습니다.", ETOI(type));
        return false;
    }

    auto skill = iter->second.lock();
    if (!skill)
    {
        // 이미 파괴된 경우 슬롯 정리만 하고 종료
        _pendingSkills.erase(iter);
        LOG_WARN("SkillComponent::Launch_PendingSkill - 타입({})의 PendingSkill이 이미 파괴되었습니다.", ETOI(type));
        return false;
    }

    if (skill->IsLaunched())
    {
        // 이미 발사된 경우 슬롯 정리만 하고 종료
        _pendingSkills.erase(iter);
        return false;
    }

    // 발사 후 슬롯에서 제거 (이미 날아가는 스킬은 SkillObject가 스스로 생명주기 관리)
    skill->Launch(direction);
    _pendingSkills.erase(iter);

    return true;
}

void SkillComponent::Clear_PendingSkill(Protocol::OBJECT_TYPE type)
{
    auto it = _pendingSkills.find(type);
    if (it == _pendingSkills.end())
        return;

    // 아직 살아있고 발사도 안 된 경우 -> 즉시 파괴
    auto skill = it->second.lock();

    if (skill && !skill->IsLaunched())
        skill->Set_Destroy(true);

    _pendingSkills.erase(it);
}

Weak<SkillObject_Projectile> SkillComponent::Get_PendingSkill(Protocol::OBJECT_TYPE type) const
{
    auto it = _pendingSkills.find(type);

    if (it == _pendingSkills.end())
        return {};

    return it->second;
}

void SkillComponent::Apply_WeaponSkillSet(EWeaponType weaponType)
{
    // 무기 타입별 기본 스킬 슬롯 세팅
    switch (weaponType)
    {
    case EWeaponType::BigSwrod:
        Set_EquippedSkill_ID(0, ETOI(ESkillType::Chidori));
        Set_EquippedSkill_ID(1, ETOI(ESkillType::FireBall));
        Set_EquippedSkill_ID(2, ETOI(ESkillType::Kirin));
        break;

    case EWeaponType::Hand:
    default:
        Set_EquippedSkill_ID(0, ETOI(ESkillType::Rasengan));
        Set_EquippedSkill_ID(1, ETOI(ESkillType::Rasen_Shuriken));
        Set_EquippedSkill_ID(2, ETOI(ESkillType::ShinsuSenju));
        break;
    }
}

void SkillComponent::Set_EquippedSkill_ID(int slot, int skill_Id)
{
    if (slot < 0 || slot >= SLOT_COUNT)
        return;

    _slotSkill_Id[slot] = skill_Id;
    _cooldownRemain[slot] = 0.f;
}

void SkillComponent::Add_ActiveStretchingMesh(Shared<GameObject> effect)
{
    if (!effect)
        return;

    Cleanup_ActiveStretchingMeshes();
    _activeStretchingMeshes.emplace_back(effect);
}

void SkillComponent::Cleanup_ActiveStretchingMeshes()
{
    _activeStretchingMeshes.erase(
        remove_if(_activeStretchingMeshes.begin(), _activeStretchingMeshes.end(),
            [](const Weak<GameObject>& weakObj)
            {
                auto obj = weakObj.lock();

                return !obj || obj->Is_Destroy();

            }), _activeStretchingMeshes.end());
}

void SkillComponent::Destroy_AllStretchingMeshes()
{
    Cleanup_ActiveStretchingMeshes();

    for (auto& weakObj : _activeStretchingMeshes)
    {
        auto obj = weakObj.lock();
        if (!obj || obj->Is_Destroy())
            continue;

        obj->Set_Destroy(true);
    }

    _activeStretchingMeshes.clear();
}

json SkillComponent::To_Json() const
{
    json root = Component::To_Json();
    root["slotSkillID"] = { _slotSkill_Id[0], _slotSkill_Id[1], _slotSkill_Id[2] };

    return root;
}

void SkillComponent::From_Json(const json& data)
{
    Component::From_Json(data);

    if (!data.contains("slotSkillID"))
        return;

    const auto& slotData = data["slotSkillID"];

    for (int32 i = 0; i < SLOT_COUNT; ++i)
    {
        if (i < static_cast<int32>(slotData.size()))
            _slotSkill_Id[i] = slotData[i].get<int32>();
        else
            _slotSkill_Id[i] = 0;

        _cooldownRemain[i] = 0.f;
    }
}

int32 SkillComponent::To_SubSkillIndex(ESubSkillType type)
{
    const int32 index = static_cast<int32>(type);

    if (index < 0 || index >= SUB_SKILL_COUNT)
        return -1;

    return index;
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
