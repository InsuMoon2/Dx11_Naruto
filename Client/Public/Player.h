#pragma once

#include "Character.h"

NS_BEGIN(Engine)
class Model;
NS_END

NS_BEGIN(Client)

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

protected:
    HRESULT Ready_Components() override;
    HRESULT Bind_ShaderResources() override;
    HRESULT Bind_Lights() override;

private:
    Shared<Model>   _model;

    uint64 _networkId = 0;

public:
    static shared_ptr<GameObject>  Create(ComPtr<Device> device, ComPtr<DeviceContext> context);
    virtual shared_ptr<GameObject> Clone(void* arg) override;

};

NS_END
