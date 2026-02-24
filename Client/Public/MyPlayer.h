#pragma once

#include "Player.h"

NS_BEGIN(Client)

class CombatStat;
class PlayerController;
class InputComponent;
class MovementComponent;

class MyPlayer final : public Player
{
    GENERATED_BODY(MyPlayer)

public:
    explicit MyPlayer(ComPtr<Device> device, ComPtr<DeviceContext> context);
    explicit MyPlayer(const MyPlayer& rhs);
    virtual ~MyPlayer() = default;

public:
    HRESULT Initialize_Prototype() override;
    HRESULT Initialize(void* arg) override;
    void    Update(float timeDelta) override;
    void    Late_Update(float timeDelta) override;

    // 내 캐릭터는 서버 위치 무시
    void Sync(const Protocol::ObjectInfo& info) override {}

private:
    void    Send_MovePacket();
    HRESULT Ready_Components() override;

private:
    Shared<CombatStat>          _combatStat;
    Shared<InputComponent>      _input;
    Shared<MovementComponent>   _movement;
    Shared<PlayerController>    _playerController;

private:
    float _syncTimer = 0.f;
    float _syncInterval = 0.1f; // 일단 0.1초마다. 나중에 늘릴 예정
    Vec3 _lastSyncPos = {};

public:
    static shared_ptr<GameObject> Create(ComPtr<Device> device, ComPtr<DeviceContext> context);
    shared_ptr<GameObject> Clone(void* arg) override;
    void Free() override;
};

NS_END
