#pragma once

#include "Base.h"
#include "Delegate.h"

NS_BEGIN(Engine)

class Transform;

DECLARE_DELEGATE(FOnPlayerSpawned, Shared<Transform>);

// 델리게이트들을 모아놓을 허브
class DelegateHub : public Base
{
public:
    GENERATED_BODY(DelegateHub)

public:
    explicit DelegateHub();
    virtual ~DelegateHub();

public:
    FOnPlayerSpawned    OnPlayerSpawned;

    // 추후 확장 할 것들
    // LevelChanged, OnBossKill, MonsterKill 등등? 이벤트들 관리하는 허브.

    
};

NS_END
