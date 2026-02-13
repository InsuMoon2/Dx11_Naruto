#include "pch.h"
#include "AIController.h"

AIController::AIController(ComPtr<Device> device, ComPtr<DeviceContext> context)
    : Controller(device, context)
{
}

AIController::AIController(const AIController& rhs)
    : Controller(rhs)
{
}

AIController::~AIController()
{
}

HRESULT AIController::Initialize_Prototype()
{
    return Controller::Initialize_Prototype();
}

HRESULT AIController::Initialize(void* arg)
{
    return Controller::Initialize(arg);
}

void AIController::Update(float timeDelta)
{
    Controller::Update(timeDelta);

    Update_BehaviorTree(timeDelta);
}

void AIController::Update_BehaviorTree(float timeDelta)
{

}

json AIController::To_Json() const
{
    json j = Controller::To_Json();

    // TODO : BehaviorTree json 경로 저장

    return j;
}

void AIController::From_Json(const json& data)
{
    Controller::From_Json(data);
}

Shared<AIController> AIController::Create(ComPtr<Device> device, ComPtr<DeviceContext> context)
{
    auto instance = make_shared<AIController>(device, context);

    if (FAILED(instance->Initialize_Prototype()))
    {
        MSG_BOX("Failed to Create: AIController");
        instance.reset();
    }

    return instance;
}

shared_ptr<Component> AIController::Clone(void* arg)
{
    auto clone = make_shared<AIController>(*this);

    if (FAILED(clone->Initialize(arg)))
    {
        MSG_BOX("Failed to Clone: AIController");
        clone.reset();
    }

    return clone;
}

void AIController::Free()
{
    Controller::Free();
}
