#pragma once

#include "Controller.h"

NS_BEGIN(Engine)
class BehaviorTree;
class Blackboard;
class MovementComponent;
NS_END

NS_BEGIN(Client)
class AnimationStateComponent;

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
    void    Refresh_RuntimeBindings();

protected:
    json    To_Json() const override;
    void    From_Json(const json& data) override;

private:
    Shared<BehaviorTree>        _behavior;
    Shared<MovementComponent>   _movement;
    Shared<Blackboard>          _blackboard;
    Shared<AnimationStateComponent> _animationState;

    wstring                     _btFilePath;

    string                      _lastAnimState = "";
    int32                       _lastAnimDirection = static_cast<int32>(EMoveInputDirection::Forward);

    int32                       _lastAnimReplaySerial = 0;

public:
    static Shared<AIController> Create(ComPtr<Device> device, ComPtr<DeviceContext> context);
    Shared<Component>           Clone(void* arg) override;
    void                        Free() override;
};

NS_END
