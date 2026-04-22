#include "pch.h"
#include "RemotePlayer.h"

#include "AnimationStateComponent.h"
#include "CombatStat.h"
#include "GameObject_Factory.h"

REGISTER_GAMEOBJECT(RemotePlayer, Protocol::OBJECT_TYPE_REMOTE_PLAYER)

RemotePlayer::RemotePlayer(ComPtr<Device> device, ComPtr<DeviceContext> context)
    : Player(device, context)
{
    Set_ObjectType(Protocol::OBJECT_TYPE_REMOTE_PLAYER);
    Set_Local(false);
}

RemotePlayer::RemotePlayer(const RemotePlayer& rhs)
    : Player(rhs)
{
}

RemotePlayer::~RemotePlayer()
{
}

HRESULT RemotePlayer::Initialize_Prototype()
{

    return S_OK;
}

HRESULT RemotePlayer::Initialize(void* arg)
{
    CHECK_FAILED(Player::Initialize(arg), E_FAIL);

    // 입력, 이동 컴포넌트는 없고 서버 데이터에만 반영한다.

    return S_OK;
}

void RemotePlayer::Update(float timeDelta)
{
    Player::Update(timeDelta);

    Vec3 currentPos = _transformCom->Get_WorldPosition();
    Vec3 newPos = Vec3::Lerp(currentPos, _targetPos, _lerpSpeed * timeDelta);
    _transformCom->Set_WorldPosition(newPos);

    Quat currentRot = _transformCom->Get_LocalRotation();
    Quat newRot = Quat::Slerp(currentRot, _targetRotation, _lerpSpeed * timeDelta);
    _transformCom->Set_LocalRotation(newRot);

    if (_animState)
    {
        _animState->Apply_NetworkState();
    }
}

void RemotePlayer::Sync(const Protocol::ObjectInfo& info)
{
    const Vec3 nextPos(info.pos().x(), info.pos().y(), info.pos().z());
    const Quat nextRotation = Quat::CreateFromYawPitchRoll(
        info.rot_y(),
        info.rot_x(),
        info.rot_z());

    const Vec3 currentPos = _transformCom->Get_WorldPosition();
    const float distSq = Vec3::DistanceSquared(currentPos, nextPos);

    if (!_hasReceivedFirstSync || distSq >= _snapDistanceSq)
    {
        _transformCom->Set_WorldPosition(nextPos);
        _transformCom->Set_LocalRotation(nextRotation);

        _targetPos = nextPos;
        _targetRotation = nextRotation;
        _hasReceivedFirstSync = true;
    }
    else
    {
        _targetPos = nextPos;
        _targetRotation = nextRotation;
    }

    if (_animState)
    {
        _animState->Read_FromObjectInfo(info);
    }

    Refresh_WeaponAttachment_ByReplicatedState(
        From_ProtoWeaponType(info.weapon_type()),
        info.object_state());

    if (_combatStat && info.has_stat())
    {
        Protocol::CombatStat stat = info.stat();
        _combatStat->Sync_FromProtobuf(stat);
    }
}

shared_ptr<GameObject> RemotePlayer::Create(ComPtr<Device> device, ComPtr<DeviceContext> context)
{
    auto instance = make_shared<RemotePlayer>(device, context);

    if (FAILED(instance->Initialize_Prototype()))
    {
        MSG_BOX("Failed to Create : RemotePlayer");
        instance->Free();

        return nullptr;
    }

    return instance;
}

shared_ptr<GameObject> RemotePlayer::Clone(void* arg)
{
    auto clone = make_shared<RemotePlayer>(*this);

    if (FAILED(clone->Initialize(arg)))
    {
        MSG_BOX("Failed to Clone : RemotePlayer");
        clone->Free();

        return nullptr;
    }

    return clone;
}

void RemotePlayer::Free()
{
    Player::Free();
}
