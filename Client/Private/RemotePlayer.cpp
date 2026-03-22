#include "pch.h"
#include "RemotePlayer.h"
#include "GameObject_Factory.h"

REGISTER_GAMEOBJECT(RemotePlayer, Protocol::OBJECT_TYPE_REMOTE_PLAYER)

RemotePlayer::RemotePlayer(ComPtr<Device> device, ComPtr<DeviceContext> context)
    : Player(device, context)
{
    Set_ObjectType(Protocol::OBJECT_TYPE_REMOTE_PLAYER);
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

    Vec3 currentPos = _transformCom->Get_LocalPosition();
    Vec3 newPos = Vec3::Lerp(currentPos, _targetPos, _lerpSpeed * timeDelta);

    _transformCom->Set_LocalPosition(newPos);

    Quat targetRot = Quat::CreateFromYawPitchRoll(_targetRotY, 0.f, 0.f);
    Quat currentRot = _transformCom->Get_LocalRotation();
    Quat newRot = Quat::Slerp(currentRot, targetRot, _lerpSpeed * timeDelta);

    _transformCom->Set_LocalRotation(newRot);
}

void RemotePlayer::Sync(const Protocol::ObjectInfo& info)
{
    // 목표 위치만 갱신 후 Update에서 보간 진행 (자연스러운 움직임을 위해)
    _targetPos = Vec3(info.pos().x(), info.pos().y(), info.pos().z());
    _targetRotY = info.rot_y();
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
