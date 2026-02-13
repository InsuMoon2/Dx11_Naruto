#pragma once

#include "Controller.h"

NS_BEGIN(Client)

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
    void    Update(float timeDelta) override;

public:
    void    Update_BehaviorTree(float timeDelta);

protected:
    json    To_Json() const override;
    void    From_Json(const json& data) override;

private:
    // TODO : BehaviorTree Com, Blackboard 추가


public:
    static Shared<AIController> Create(ComPtr<Device> device, ComPtr<DeviceContext> context);
    Shared<Component>           Clone(void* arg) override;
    void                        Free() override;
};

NS_END
