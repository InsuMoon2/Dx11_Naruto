#include "pch.h"
#include "PlayerController.h"
#include "GameObject.h"
#include "Transform.h"
#include "Input_Manager.h"
//#include "ServerSession.h"

PlayerController::PlayerController(ComPtr<Device> device, ComPtr<DeviceContext> context)
    : Controller(device, context)
{
}

PlayerController::PlayerController(const PlayerController& rhs)
    : Controller(rhs)
{
}

PlayerController::~PlayerController()
{
}

HRESULT PlayerController::Initialize_Prototype()
{
    Controller::Initialize_Prototype();

    return S_OK;
}

HRESULT PlayerController::Initialize(void* arg)
{
    Controller::Initialize(arg);

    return S_OK;
}

void PlayerController::Update(float timeDelta)
{
    Controller::Update(timeDelta);

    Handle_Input(timeDelta);
}

void PlayerController::Handle_Input(float timeDelta)
{
    auto pawn = Get_Pawn();


}

void PlayerController::Send_MovePacket()
{
    // TODO : 서버에 패킷보내기
}

json PlayerController::To_Json() const
{
    json j = Controller::To_Json();

    return j;
}

void PlayerController::From_Json(const json& data)
{
    Controller::From_Json(data);
}

Shared<PlayerController> PlayerController::Create(ComPtr<Device> device, ComPtr<DeviceContext> context)
{
    auto instance = make_shared<PlayerController>(device, context);

    if (FAILED(instance->Initialize_Prototype()))
    {
        MSG_BOX("Failed to Create: PlayerController");
        instance.reset();
    }

    return instance;
}

shared_ptr<Component> PlayerController::Clone(void* arg)
{
    auto clone = make_shared<PlayerController>(*this);

    if (FAILED(clone->Initialize(arg)))
    {
        MSG_BOX("Failed to Clone: PlayerController");
        clone.reset();
    }

    return clone;
}

void PlayerController::Free()
{
    Controller::Free();
}
