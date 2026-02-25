#pragma once

#include "GameObject.h"

NS_BEGIN(Engine)

class ENGINE_DLL PlayerStart : public GameObject
{
    GENERATED_BODY(PlayerStart)

public:
    struct FPlayerStartDesc : public FGameObjectDesc
    {
        uint32 spawnIndex = 0; // 멀티 스폰 포인트 구분용
    };

public:
    explicit PlayerStart(ComPtr<Device> device, ComPtr<DeviceContext> context);
    explicit PlayerStart(const PlayerStart& rhs);
    virtual ~PlayerStart();

public:
    virtual HRESULT Initialize_Prototype()  override;
    virtual HRESULT Initialize(void* arg)   override;
    virtual void    BeginPlay()             override;

    virtual void    Priority_Update(float timeDelta) override;
    virtual void    Update(float timeDelta)          override;
    virtual void    Late_Update(float timeDelta)     override;
    virtual HRESULT Render()                         override;

public:
    uint32  Get_SpawnIndex() const { return _spawnIndex; }

private:
    uint32 _spawnIndex = 0;

public:
    static Shared<PlayerStart> Create(ComPtr<Device> device, ComPtr<DeviceContext> context);
    virtual Shared<GameObject> Clone(void* arg) override;
    virtual void Free() override;

};

NS_END
