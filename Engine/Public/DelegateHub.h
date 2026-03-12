#pragma once

#include "Base.h"
#include "Delegate.h"

NS_BEGIN(Engine)

class Transform;
class GameObject;

DECLARE_DELEGATE(FOnPlayerSpawned, Shared<Transform>);
DECLARE_DELEGATE(FOnPlayerObjectSpawned, Shared<GameObject>);

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
    FOnPlayerSpawned        OnPlayerSpawned;
    // 플레이어
    FOnPlayerObjectSpawned  OnPlayerObjectSpawned;

    // 추후 확장 할 것들
    // LevelChanged, OnBossKill, MonsterKill .. etc

    
};

NS_END
