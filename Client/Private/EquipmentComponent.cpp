#include "pch.h"
#include "EquipmentComponent.h"

EquipmentComponent::EquipmentComponent(ComPtr<Device> device, ComPtr<DeviceContext> context)
    : Component(device, context)
{
}

EquipmentComponent::EquipmentComponent(const EquipmentComponent& rhs)
    : Component(rhs)
{
}

EquipmentComponent::~EquipmentComponent()
{
}

HRESULT EquipmentComponent::Initialize_Prototype()
{
    return Component::Initialize_Prototype();
}

HRESULT EquipmentComponent::Initialize(void* arg)
{
    return Component::Initialize(arg);
}

void EquipmentComponent::BeginPlay()
{
    Component::BeginPlay();

    // 처음에 무기 장착되고 시작
    _currentWeaponType = Find_WeaponType_FromAssetTag(_equippedPartTags[ETOI(EPartSlot::Weapon)]);
}

void EquipmentComponent::Equip_Part(EPartSlot slot, const wstring& assetTag)
{
    _equippedPartTags[ETOI(slot)] = assetTag;

    if (slot == EPartSlot::Weapon)
    {
        _currentWeaponType = Find_WeaponType_FromAssetTag(assetTag);
    }
}

void EquipmentComponent::Unequip_Part(EPartSlot slot)
{
    _equippedPartTags[ETOI(slot)].clear();

    if (slot == EPartSlot::Weapon)
    {
        _currentWeaponType = EWeaponType::Hand;
    }
}

const wstring& EquipmentComponent::Get_EquippedPartTag(EPartSlot slot) const
{
    return _equippedPartTags[ETOI(slot)];
}

EAttackProfileType EquipmentComponent::Find_AttackProfileType(bool isAerial) const
{
    switch (_currentWeaponType)
    {
    case EWeaponType::BigSwrod:
        return isAerial ? EAttackProfileType::BigSword_Aerial
                        : EAttackProfileType::BigSword_Ground;
    case EWeaponType::Hand:
    default:
        return isAerial ? EAttackProfileType::Hand_Aerial
            : EAttackProfileType::Hand_Ground;
    }
}

void EquipmentComponent::Toggle_WeaponMode()
{
    if (_currentWeaponType == EWeaponType::Hand)
        _currentWeaponType = EWeaponType::BigSwrod;
    else
        _currentWeaponType = EWeaponType::Hand;
}

EWeaponType EquipmentComponent::Find_WeaponType_FromAssetTag(const wstring& assetTag)
{
    // 무기 추가되면 늘려야함
    if (assetTag.find(TEXT("BigSword")) != wstring::npos)
        return EWeaponType::BigSwrod;

    return EWeaponType::Hand;
}

json EquipmentComponent::To_Json() const
{
    json root = Component::To_Json();

    json parts = json::object();
    for (int32 i = 0; i < ETOI(EPartSlot::END); ++i)
    {
        if (!_equippedPartTags[i].empty())
        {
            string narrow = Utils::ToString(_equippedPartTags[i]);

            parts[ContainerObject::Get_PartSlotName(static_cast<EPartSlot>(i))] = narrow;
        }
    }

    root["equipped_parts"] = parts;

    return root;
}

void EquipmentComponent::From_Json(const json& data)
{
    Component::From_Json(data);

    if (!data.contains("equipped_parts"))
        return;

    const auto& parts = data["equipped_parts"];

    for (int32 i = 0; i < ETOI(EPartSlot::END); ++i)
    {
        string slotName = ContainerObject::Get_PartSlotName(static_cast<EPartSlot>(i));
        if (parts.contains(slotName))
        {
            string narrow = parts[slotName].get<string>();
            _equippedPartTags[i] = wstring(narrow.begin(), narrow.end());
        }
    }

    // 역직렬화 후 무기 타입 재판정
    _currentWeaponType = Find_WeaponType_FromAssetTag(
        _equippedPartTags[ETOI(EPartSlot::Weapon)]);
}

Shared<EquipmentComponent> EquipmentComponent::Create(ComPtr<Device> device, ComPtr<DeviceContext> context)
{
    auto instance = make_shared<EquipmentComponent>(device, context);

    if (FAILED(instance->Initialize_Prototype()))
    {
        MSG_BOX("Failed to Create : EquipmentComponent");

        return nullptr;
    }

    return instance;
}

Shared<Component> EquipmentComponent::Clone(void* arg)
{
    auto clone = make_shared<EquipmentComponent>(*this);

    if (FAILED(clone->Initialize(arg)))
    {
        MSG_BOX("Failed to Clone : EquipmentComponent");

        return nullptr;
    }

    return clone;
}

void EquipmentComponent::Free()
{
    Component::Free();
}
