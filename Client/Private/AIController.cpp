#include "pch.h"
#include "AIController.h"
#include "BehaviorTree.h"
#include "MovementComponent.h"
#include "GameObject.h"
#include "Blackboard.h"

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

void AIController::BeginPlay()
{
    Controller::BeginPlay();

    auto pawn = Get_Pawn();

    _movement = pawn->Get_Component<MovementComponent>();
    _behavior = pawn->Get_Component<BehaviorTree>();

    if (_behavior)
        _blackboard = _behavior->Get_Blackboard();

}

void AIController::Update(float timeDelta)
{
    Controller::Update(timeDelta);

    if (_behavior)
        _behavior->Update(timeDelta);

    if (_movement && _blackboard)
    {
        MovementComponent::FMoveCommand command;

        if (_blackboard->HasKey("MoveAxisX"))
            command.moveAxis.x = _blackboard->Get_ValueAsFloat("MoveAxisX");

        if (_blackboard->HasKey("MoveAxisY"))
            command.moveAxis.x = _blackboard->Get_ValueAsFloat("MoveAxisY");

        if (_blackboard->HasKey("Sprint"))
            command.sprint = _blackboard->Get_ValueAsBool("Sprint");

        _movement->Apply_Command(command);
        _movement->Update(timeDelta);
    }

}

json AIController::To_Json() const
{
    json j = Controller::To_Json();

    if (_btFilePath.length() > 0)
        j["bt_path"] = Utils::ToString(_btFilePath);

    return j;
}

void AIController::From_Json(const json& data)
{
    Controller::From_Json(data);

    if (data.contains("bt_path"))
    {
        wstring path = Utils::ToWString(data["bt_path"].get<string>());
        _btFilePath = path;

        if (_behavior)
            _behavior->Load_FromJson(path);
    }
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
