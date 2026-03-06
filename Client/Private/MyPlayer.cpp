#include "pch.h"
#include "MyPlayer.h"

#include "CombatStat.h"
#include "InputComponent.h"
#include "MovementComponent.h"
#include "NetworkManager.h"
#include "PlayerController.h"
#include "PlayerStateMachine.h"
#include "Client_PacketHandler.h"

MyPlayer::MyPlayer(ComPtr<Device> device, ComPtr<DeviceContext> context)
    : Player(device, context)
{
}

MyPlayer::MyPlayer(const MyPlayer& rhs)
    : Player(rhs)
{
}

HRESULT MyPlayer::Initialize_Prototype()
{

    return S_OK;
}

HRESULT MyPlayer::Initialize(void* arg)
{
    CHECK_FAILED(Player::Initialize(arg), E_FAIL);

    // MyPlayer만 입력/이동 컴포넌트 보유
    {
        CombatStat::FCombatStatDesc statDesc;
        statDesc.maxHp = 200.f;
        statDesc.attack = 100.f;

        CHECK_FAILED(Add_Component(Protocol::COMPONENT_TYPE_COMBAT_STAT, _combatStat, &statDesc), E_FAIL);
    }
    {
        MovementComponent::FMovementDesc moveDesc;
        moveDesc.maxWalkSpeed = 4.f;
        moveDesc.maxSprintSpeed = 7.f;

        CHECK_FAILED(Add_Component(Protocol::COMPONENT_TYPE_MOVEMENT, _movement, &moveDesc), E_FAIL);
        CHECK_FAILED(Add_Component(Protocol::COMPONENT_TYPE_INPUT, _input), E_FAIL);
    }

    CHECK_FAILED(Add_Component(Protocol::COMPONENT_TYPE_PLAYER_CONTROLLER, _playerController), E_FAIL);
    CHECK_FAILED(Add_Component(Protocol::COMPONENT_TYPE_PLAYER_STATE, _stateMachine), E_FAIL);

    return S_OK;
}

void MyPlayer::Update(float timeDelta)
{
    Player::Update(timeDelta);

    _playerController->Update(timeDelta);
}

void MyPlayer::Late_Update(float timeDelta)
{
    Player::Late_Update(timeDelta);

    _syncTimer += timeDelta;
    if (_syncTimer >= _syncInterval)
    {
        _syncTimer = 0.f;
        Send_MovePacket();
    }
}

void MyPlayer::Send_MovePacket()
{
    Vec3 pos = _transformCom->Get_LocalPosition();

    //if (pos == _lastSyncPos)
    //    return;

    _lastSyncPos = pos;
    float rotY = _transformCom->Get_LocalRotation().ToEuler().y;

    //LOG_INFO("Send rotY: {}", rotY);

    auto buf = Client_PacketHandler::Make_C_Move(pos.x, pos.y, pos.z, rotY);

    if (buf)
    {
        GET_SINGLE(NetworkManager)->Send_Packet(buf);
    }
}

HRESULT MyPlayer::Ready_Components()
{
    Player::Ready_Components();

    return S_OK;
}

shared_ptr<GameObject> MyPlayer::Create(ComPtr<Device> device, ComPtr<DeviceContext> context)
{
    auto instance = make_shared<MyPlayer>(device, context);

    if (FAILED(instance->Initialize_Prototype()))
    {
        MSG_BOX("Failed to Create : MyPlayer");
        instance->Free();

        return nullptr;
    }

    return instance;
}

shared_ptr<GameObject> MyPlayer::Clone(void* arg)
{
    auto clone = make_shared<MyPlayer>(*this);

    if (FAILED(clone->Initialize(arg)))
    {
        MSG_BOX("Failed to Clone : MyPlayer");
        clone->Free();

        return nullptr;
    }

    return clone;
}

void MyPlayer::Free()
{
    Player::Free();

}
