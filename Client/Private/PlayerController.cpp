#include "pch.h"
#include "PlayerController.h"
#include "GameObject.h"
#include "InputComponent.h"
#include "Transform.h"
#include "Input_Manager.h"
#include "MovementComponent.h"
//#include "ServerSession.h"
#include "Camera_Target.h"
#include "PlayerStateMachine.h"

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

void PlayerController::BeginPlay()
{
    Controller::BeginPlay();

    auto pawn = Get_Pawn();

    _input          = pawn->Get_Component<InputComponent>();
    _movement       = pawn->Get_Component<MovementComponent>();
    _stateMachine   = pawn->Get_Component<PlayerStateMachine>();
}

void PlayerController::Update(float timeDelta)
{
    Controller::Update(timeDelta);

    _input->Update_Input(timeDelta);

    auto activeCamera = GAME->Get_ActiveCamera();
    if (activeCamera && activeCamera->Get_ObjectType() == Protocol::OBJECT_TYPE_CAMERA_TARGET)
    {
        auto camera = dynamic_pointer_cast<Camera_Target>(
            GAME->Find_Camera(Protocol::OBJECT_TYPE_CAMERA_TARGET));

        if (camera && camera->Get_UseControlYaw())
        {
            // 카메라 Yaw 값을 플레이어한테 적용
            float cameraYaw = camera->Get_Yaw();
            auto transform = Get_Pawn()->Get_Component<Transform>();

            transform->Set_WorldRotation(
                0.f,
                (cameraYaw),
                0.f);
        }
    }

    if (_stateMachine)
        _stateMachine->Update(timeDelta);

}

void PlayerController::Send_MovePacket()
{
    // TODO : 서버에 패킷보내기
}

json PlayerController::To_Json() const
{
    json root = Controller::To_Json();

    return root;
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
