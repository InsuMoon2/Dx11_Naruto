#pragma once

#include "Character.h"

NS_BEGIN(Engine)
class Model;
NS_END

NS_BEGIN(Client)

class AnimationStateComponent;
class CombatStat;
class EquipmentComponent;

class Player : public Character
{
    GENERATED_BODY(Player)

public:
    explicit Player(ComPtr<Device> device, ComPtr<DeviceContext> context);
    explicit Player(const Player& rhs);
    virtual ~Player() = default;

public:
    HRESULT Initialize_Prototype() override;
    HRESULT Initialize(void* arg) override;
    void    BeginPlay() override;
    void    Priority_Update(float timeDelta) override;
    void    Update(float timeDelta) override;
    void    Late_Update(float timeDelta) override;
    HRESULT Render() override;

public: /* Network */
    uint64  Get_NetworkId() const { return _networkId; }
    void    Set_NetworkId(uint64 id) { _networkId = id; }
    virtual void Sync(const Protocol::ObjectInfo& info);

public:
    HRESULT Apply_CustomizingPart(EPartSlot slot, const wstring& modelAssetTag);

protected:
    HRESULT Ready_Components() override;
    HRESULT Bind_ShaderResources() override;
    HRESULT Bind_Lights() override;

    virtual HRESULT Ready_PartObjects();

protected:
    const Matrix* Find_WeaponSocketMatrix(EWeaponType weaponType) const;
    void  Change_WeaponAttachment(EWeaponType weaponType);
    void  On_WeaponTypeChagned(int32 weaponTypeIndex);

protected:
    Shared<CombatStat>              _combatStat;
    Shared<AnimationStateComponent> _animState;
    Shared<Model>                   _model;
    Shared<EquipmentComponent>      _equipment;

protected:
    uint64                          _networkId = 0;

    FDelegateHandle                 _weaponTypeChangedHandle = {};

public:
    static Shared<GameObject>  Create(ComPtr<Device> device, ComPtr<DeviceContext> context);
    virtual Shared<GameObject> Clone(void* arg) override;

};

NS_END
