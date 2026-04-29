#pragma once

#include "Monster.h"

NS_BEGIN(Client)

class CharkraMove_Component;

class Monster_Leaf : public Monster
{
    GENERATED_BODY(Monster_Leaf)

public:
    explicit Monster_Leaf(ComPtr<Device> device, ComPtr<DeviceContext> context);
    explicit Monster_Leaf(const Monster_Leaf& rhs);
    virtual ~Monster_Leaf() = default;

public:
    HRESULT Initialize_Prototype() override;
    HRESULT Initialize(void* arg) override;

    void    BeginPlay() override;

    void    Update(float timeDelta) override;
    void    Late_Update(float timeDelta) override;
    HRESULT Render() override;

protected:
    HRESULT Ready_Components() override;
    Protocol::OBJECT_TYPE Get_EnemyObjectType() const override;

private:
    bool _glovePartsReady = false; // BeginPlay에서 글러브 소켓 파츠를 이미 준비했는지 추적한다.
    bool _glovePartsClearedOnDeath = false; // 사망 시 글러브 파츠를 중복 제거하지 않도록 한 번만 처리했는지 추적한다.

    // Monster_Leaf가 죽는 순간 양손 글러브 파츠를 즉시 숨기기 위해 호출한다.
    // Update에서 사망 상태를 감지했을 때 한 번만 실행된다.
    void Clear_GloveParts_OnDeath();

    HRESULT Ready_GloveParts();

private:
    Shared<CharkraMove_Component> _chakraTrail;

public:
    static Shared<Monster_Leaf> Create(ComPtr<Device> device, ComPtr<DeviceContext> context);
    Shared<GameObject> Clone(void* arg) override;
    void Free() override;

};

NS_END
