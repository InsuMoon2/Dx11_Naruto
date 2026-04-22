#pragma once

#include "Character.h"

NS_BEGIN(Engine)
class Model;
class Collider;
NS_END

NS_BEGIN(Client)

class AnimationStateComponent;
class CharkraMove_Component;
class CombatStat;
class EquipmentComponent;
class SmearEffect_Component;
class SwordTrail_Component;
class SkillComponent;

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

    void    TakeDamage(const FDamageEvent& damageEvent) override;

	void    OnDamaged(const FDamageEvent& damageEvent) override;
	void    OnDead(const FDamageEvent& damageEvent) override;

public: /* Network */
    uint64  Get_NetworkId() const { return _networkId; }
    void    Set_NetworkId(uint64 id) { _networkId = id; }
    virtual void Sync(const Protocol::ObjectInfo& info);

public:
    HRESULT Apply_CustomizingPart(EPartSlot slot, const wstring& modelAssetTag);

    void    Refresh_WeaponAttachment_ByCurrentState();
    void    Refresh_WeaponAttachment_ByReplicatedState(EWeaponType weaponType, Protocol::OBJECT_STATE_TYPE replicatedState);

    // 서버에서 이름 세팅용
    void    Set_PlayerName(const wstring& name) { _playerName = name; }
    const wstring& Get_PlayerName() const { return _playerName; }

protected:
    HRESULT Ready_Components() override;
    HRESULT Bind_ShaderResources() override;

    virtual HRESULT Ready_PartObjects();

protected:
    const Matrix* Find_WeaponSocketMatrix(EWeaponType weaponType, EPlayerState currentState) const;
    bool  Is_SwordAttackState(EPlayerState state) const;
    bool  Is_SwordAttackReplicatedState(Protocol::OBJECT_STATE_TYPE replicatedState) const;
    void  Change_WeaponAttachment(EWeaponType weaponType);

    static Protocol::WEAPON_TYPE To_ProtoWeaponType(EWeaponType weaponType);
    static EWeaponType From_ProtoWeaponType(Protocol::WEAPON_TYPE weaponType);

    void  On_WeaponTypeChagned(int32 weaponTypeIndex);

protected:
    Shared<CombatStat>                  _combatStat;
    Shared<AnimationStateComponent>     _animState;
    Shared<Model>                       _model;
    Shared<EquipmentComponent>          _equipment;
    Shared<SmearEffect_Component>       _smearEffect;
    Shared<CharkraMove_Component>       _chakraTrail;
    Shared<SwordTrail_Component>        _swordTrail;

    Shared<SkillComponent>				_skill;

protected:
    uint64                          _networkId = 0;

    FDelegateHandle                 _weaponTypeChangedHandle = {};

    wstring _playerName = L"Player";

public:
    static Shared<GameObject>  Create(ComPtr<Device> device, ComPtr<DeviceContext> context);
    virtual Shared<GameObject> Clone(void* arg) override;

};

NS_END
