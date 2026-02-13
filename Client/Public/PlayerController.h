#pragma once

#include "Controller.h"

NS_BEGIN(Client)

class PlayerController : public Controller
{
    GENERATED_COMPONENT(PlayerController, Protocol::COMPONENT_TYPE_PLAYER_CONTROLLER)

public:
    explicit PlayerController(ComPtr<Device> device, ComPtr<DeviceContext> context);
    explicit PlayerController(const PlayerController& rhs);
    virtual ~PlayerController();
    
public:
    HRESULT Initialize_Prototype() override;
    HRESULT Initialize(void* arg) override;
    void    Update(float timeDelta) override;

public:
    void    Handle_Input(float timeDelta);
    void    Send_MovePacket();

protected:
    json    To_Json() const override;
    void    From_Json(const json& data) override;

public:
    static Shared<PlayerController> Create(ComPtr<Device> device, ComPtr<DeviceContext> context);
    Shared<Component>               Clone(void* arg) override;
    void                            Free() override;

};

NS_END;
