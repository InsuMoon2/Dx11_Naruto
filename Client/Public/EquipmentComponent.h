#pragma once

#include "Component.h"
#include "ContainerObject.h"

NS_BEGIN(Client)

class EquipmentComponent : public Component
{
    GENERATED_COMPONENT(EquipmentComponent, Protocol::COMPONENT_TYPE_EQUIPMENT)

public:
    using EPartSlot = ContainerObject::EPartSlot;

public:
    explicit EquipmentComponent(ComPtr<Device> device, ComPtr<DeviceContext> context);
    explicit EquipmentComponent(const EquipmentComponent& rhs);
    virtual ~EquipmentComponent();

public:
    HRESULT Initialize_Prototype() override;
    HRESULT Initialize(void* arg) override;
    void    BeginPlay() override;

public:
    void    Equip_Part(EPartSlot slot, const wstring& assetTag);
    void    Unequip_Part(EPartSlot slot);

    EWeaponType Get_CurrentWeaponType() const { return _currentWeaponType; }

    // 지정 슬롯 에셋 반환
    const wstring& Get_EquippedPartTag(EPartSlot slot) const;

    void    Set_WeaponType(EWeaponType weaponType) { _currentWeaponType = weaponType; }

    // 현재 무기 타입 + 공중/ 지상 여부로 EAttackProfileType 결정하기
    // PlayerState_Attack의 Enter에서 세팅하고 공격 시작되게
    EAttackProfileType Find_AttackProfileType(bool isAerial) const;

    void    Toggle_WeaponMode();

public:
    json To_Json() const override;
    void From_Json(const json& data) override;

private:
    // assetTag 문자열로 EWeaponType 를 역으로 찾기
    static EWeaponType Find_WeaponType_FromAssetTag(const wstring& assetTag);

private:
    EWeaponType _currentWeaponType = EWeaponType::Hand;

    array<wstring, ETOI(EPartSlot::END)> _equippedPartTags{};


public:
    static Shared<EquipmentComponent> Create(ComPtr<Device> device, ComPtr<DeviceContext> context);
    Shared<Component> Clone(void* arg) override;
    void Free() override;
    
};

NS_END
