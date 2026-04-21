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

DECLARE_DELEGATE(FOnWaveCleared, const string&);

// 델리게이트들을 모아놓을 허브 : 매니저 역할이긴하네..
class ENGINE_DLL DelegateHub : public Base
{
public:
    GENERATED_BODY(DelegateHub)

public:
    explicit DelegateHub();
    virtual ~DelegateHub();

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

    FOnWaveCleared              OnWaveCleared;

    // 추후 확장 할 것들
    // LevelChanged, OnBossKill, MonsterKill .. etc

    
};

NS_END
