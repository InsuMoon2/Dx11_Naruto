#include "pch.h"
#include "MyPlayer.h"

#include "CombatStat.h"
#include "InputComponent.h"
#include "MovementComponent.h"
#include "NetworkManager.h"
#include "PlayerController.h"
#include "PlayerStateMachine.h"
#include "Client_PacketHandler.h"
#include "SkillComponent.h"
#include "AnimationStateComponent.h"
#include "GameObject_Factory.h"

REGISTER_GAMEOBJECT(MyPlayer, Protocol::OBJECT_TYPE_PLAYER)

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
        SkillComponent::FSkillDesc skillDesc;

        skillDesc.slotSkill_Id[0] = 1001; // 나선환
        skillDesc.slotSkill_Id[1] = 1002; // 나선 수리검

        CHECK_FAILED(Add_Component(Protocol::COMPONENT_TYPE_SKILL, _skill, &skillDesc), E_FAIL);
    }
    {
        MovementComponent::FMovementDesc moveDesc;
        moveDesc.maxWalkSpeed = 40.f;
        moveDesc.maxSprintSpeed = 70.f;

        CHECK_FAILED(Add_Component(Protocol::COMPONENT_TYPE_MOVEMENT, _movement, &moveDesc), E_FAIL);
        CHECK_FAILED(Add_Component(Protocol::COMPONENT_TYPE_INPUT, _input), E_FAIL);
    }


    CHECK_FAILED(Add_Component(Protocol::COMPONENT_TYPE_PLAYER_CONTROLLER, _playerController), E_FAIL);
    CHECK_FAILED(Add_Component(Protocol::COMPONENT_TYPE_PLAYER_STATE, _stateMachine), E_FAIL);

    return S_OK;
}

void MyPlayer::Priority_Update(float timeDelta)
{
    Player::Priority_Update(timeDelta);

    if (_playerController)
        _playerController->Update(timeDelta);

    if (_skill)
        _skill->Update(timeDelta);
}

void MyPlayer::Update(float timeDelta)
{
    Player::Update(timeDelta);

 
}

void MyPlayer::Late_Update(float timeDelta)
{
    Player::Late_Update(timeDelta);

    _syncTimer += timeDelta;
    if (_syncTimer >= _syncInterval)
    {
        _syncTimer = 0.f;
        Send_MovePacket(false);
    }
}

void MyPlayer::Force_SendMovePacket()
{
    Send_MovePacket(true);
}

void MyPlayer::Send_MovePacket(bool forceSend)
{
    Protocol::ObjectInfo info = Build_NetworkInfo();

    if (!forceSend && !Should_SendMovePacket(info))
        return;

    auto buf = Client_PacketHandler::Make_C_Move(info);
    if (!buf)
        return;

    GET_SINGLE(NetworkManager)->Send_Packet(buf);

    _lastSyncPos = Vec3(info.pos().x(), info.pos().y(), info.pos().z());
    _lastSyncRotY = info.rot_y();
    _lastObjectState = info.object_state();
    _lastMoveDir = info.move_dir();
    _lastAnimPhase = info.anim_phase();
}

Protocol::ObjectInfo MyPlayer::Build_NetworkInfo() const
{
    Protocol::ObjectInfo info{};
    info.set_objectid(Get_NetworkId());
    info.set_objecttype(Protocol::OBJECT_TYPE_PLAYER);

    Vec3 pos = _transformCom->Get_WorldPosition();
    auto* protoPos = info.mutable_pos();
    protoPos->set_x(pos.x);
    protoPos->set_y(pos.y);
    protoPos->set_z(pos.z);

    float rotY = _transformCom->Get_LocalRotation().ToEuler().y;
    info.set_rot_y(rotY);

    if (_animState && _stateMachine)
    {
        _animState->Capture_FromStateMachine(_stateMachine);
        _animState->Write_ToObjectInfo(info);
    }

    return info;
}

HRESULT MyPlayer::Ready_Components()
{
    Player::Ready_Components();

    return S_OK;
}

bool MyPlayer::Should_SendMovePacket(const Protocol::ObjectInfo& nextInfo) const
{
    const Vec3 nextPos(
        nextInfo.pos().x(),
        nextInfo.pos().y(),
        nextInfo.pos().z());

    const float posDeltaSq = Vec3::DistanceSquared(nextPos, _lastSyncPos);
    const float rotDelta = fabsf(nextInfo.rot_y() - _lastSyncRotY);

    if (posDeltaSq > 0.0001f)
        return true;

    if (rotDelta > XMConvertToRadians(1.f))
        return true;

    if (nextInfo.object_state() != _lastObjectState)
        return true;

    if (nextInfo.move_dir() != _lastMoveDir)
        return true;

    if (nextInfo.anim_phase() != _lastAnimPhase)
        return true;

    if (nextInfo.anim_force_restart())
        return true;

    return false;
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
