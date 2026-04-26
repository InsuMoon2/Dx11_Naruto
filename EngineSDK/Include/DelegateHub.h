#pragma once

#include "Base.h"
#include "Delegate.h"
#include "Protocol.pb.h"

NS_BEGIN(Engine)

class Transform;
class GameObject;
class Character;

DECLARE_DELEGATE(FOnPlayerSpawned, Shared<Transform>);
DECLARE_DELEGATE(FOnPlayerObjectSpawned, Shared<GameObject>);
DECLARE_DELEGATE(FOnWeaponTypeChanged, int32);

DECLARE_DELEGATE(FOnDamaged, Shared<Character> /*맞은놈*/, float/*데미지*/);
DECLARE_DELEGATE(FOnDead, Shared<Character> /*죽은놈*/, Shared<GameObject> /*죽인놈*/);

DECLARE_DELEGATE(FOnPlayerComboHit, uint32/*콤보 수*/);

DECLARE_DELEGATE(FOnRemotePlayerObjectSpawned, Shared<GameObject>);

DECLARE_DELEGATE(FOnLobbySnapshotReceived, const Protocol::S_LobbySnapshot&);
DECLARE_DELEGATE(FOnLobbyChatReceived, const Protocol::S_LobbyChat&);
DECLARE_DELEGATE(FOnLobbyStartGameReceived);

DECLARE_DELEGATE(FOnWaveStarted, const string&);
DECLARE_DELEGATE(FOnWaveCleared, const string&);

DECLARE_DELEGATE(FOnBossObjectSpawned, Shared<GameObject>);

DECLARE_DELEGATE(FOnWireLockOnVisible, bool);

DECLARE_DELEGATE(FOnMissionMarkerTargetChanged, Shared<GameObject>);
DECLARE_DELEGATE(FOnMissionMarkerTargetCleared);

// 델리게이트들을 모아놓을 허브 : 매니저 역할이긴하네..
class ENGINE_DLL DelegateHub : public Base
{
public:
    GENERATED_BODY(DelegateHub)

public:
    explicit DelegateHub();
    virtual ~DelegateHub();

public:
    void Set_MissionMarkerTarget(Shared<GameObject> target)
    {
        _missionMarkerTarget = target;
        OnMissionMarkerTargetChanged.Broadcast(target);
    }

    void Clear_MissionMarkerTarget()
    {
        _missionMarkerTarget.reset();
        OnMissionMarkerTargetCleared.Broadcast();
    }

    Shared<GameObject> Get_MissionMarkerTarget() const
    {
        return _missionMarkerTarget.lock();
    }

public:
    // 위치
    FOnPlayerSpawned			OnPlayerSpawned;
    // 플레이어 스폰
    FOnPlayerObjectSpawned		OnPlayerObjectSpawned;
    // 무기 변경
    FOnWeaponTypeChanged		OnWeaponTypeChanged;

    FOnDamaged					OnDamaged;
    FOnDead						OnDead;

    FOnPlayerComboHit			OnPlayerComboHit;

    FOnRemotePlayerObjectSpawned OnRemotePlayerObjectSpawned;

	// Server
	FOnLobbySnapshotReceived	OnLobbySnapshotReceived;
	FOnLobbyChatReceived		OnLobbyChatReceived;
	FOnLobbyStartGameReceived	OnLobbyStartGameReceived;

    FOnWaveStarted              OnWaveStarted;
    FOnWaveCleared              OnWaveCleared;

    // 보스 체력바
    FOnBossObjectSpawned        OnBossObjectSpawned;

    // 와이어 액션
    FOnWireLockOnVisible        OnWireLockOnVisible;

    FOnMissionMarkerTargetChanged OnMissionMarkerTargetChanged;
    FOnMissionMarkerTargetCleared OnMissionMarkerTargetCleared;

private:
    Weak<GameObject> _missionMarkerTarget;
};

NS_END
