#pragma once

#include "Controller.h"

NS_BEGIN(Engine)
class Behavior;
class Blackboard;
NS_END

NS_BEGIN(Client)
class MovementComponent;

class AIController : public Controller
{
    GENERATED_COMPONENT(AIController, Protocol::COMPONENT_TYPE_AI_CONTROLLER)

public:
    explicit AIController(ComPtr<Device> device, ComPtr<DeviceContext> context);
    explicit AIController(const AIController& rhs);
    virtual ~AIController();

public:
    HRESULT Initialize_Prototype() override;
    HRESULT Initialize(void* arg) override;
    void    BeginPlay() override;
    void    Update(float timeDelta) override;

protected:
    json    To_Json() const override;
    void    From_Json(const json& data) override;

private:
    Shared<Behavior> _behavior;
    Shared<MovementComponent> _movement;
    Shared<Blackboard> _blackboard;

    wstring _btFilePath;

public:
    static Shared<AIController> Create(ComPtr<Device> device, ComPtr<DeviceContext> context);
    Shared<Component>           Clone(void* arg) override;
    void                        Free() override;
};

NS_END
