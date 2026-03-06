#pragma once

#include "Controller.h"

NS_BEGIN(Client)
class InputComponent;
class MovementComponent;
class PlayerStateMachine;

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
    void    BeginPlay() override;

    void    Update(float timeDelta) override;

public:
    void    Send_MovePacket();

protected:
    json    To_Json() const override;
    void    From_Json(const json& data) override;

private:
    Shared<InputComponent>      _input;
    Shared<MovementComponent>   _movement;
    Shared<PlayerStateMachine>  _stateMachine;

public:
    static Shared<PlayerController> Create(ComPtr<Device> device, ComPtr<DeviceContext> context);
    Shared<Component>               Clone(void* arg) override;
    void                            Free() override;

};

NS_END;
