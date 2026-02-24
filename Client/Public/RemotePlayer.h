#pragma once

#include "Player.h"

NS_BEGIN(Client)

class RemotePlayer : public Player
{
    GENERATED_BODY(RemotePlayer)

public:
    explicit RemotePlayer(ComPtr<Device> device, ComPtr<DeviceContext> context);
    explicit RemotePlayer(const RemotePlayer& rhs);
    virtual ~RemotePlayer();

public:
    HRESULT Initialize_Prototype() override;
    HRESULT Initialize(void* arg) override;
    void    Update(float timeDelta) override;

public:
    void Sync(const Protocol::ObjectInfo& info) override;

private:
    // 보간용 데이터
    Vec3 _targetPos = {};
    float _targetRotY = 0.f;
    float _lerpSpeed = 10.f;

public:
    static shared_ptr<GameObject> Create(ComPtr<Device> device, ComPtr<DeviceContext> context);
    shared_ptr<GameObject> Clone(void* arg) override;
    void Free() override;
};

NS_END
