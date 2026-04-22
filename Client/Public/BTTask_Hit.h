#pragma once

#include "BTTask.h"

NS_BEGIN(Client)

class BTTask_Hit : public BTTask
{
    GENERATED_BT_REFLECTION(BTTask_Hit)

public:
    explicit BTTask_Hit();
    explicit BTTask_Hit(const BTTask_Hit& rhs);
    virtual ~BTTask_Hit();

public:
    void Initialize() override;
    EBTNodeResult Update(float timeDelta) override;

private:
    string _hitFlagKey = "IsHit";
    // EnemyCharacter::OnDamaged()가 기록한 실제 피격 애니메이션 상태 키를 읽는다.
    string _hitAnimStateKey = "HitAnimState";

    string _hitSerialKey = "HitReactionSerial";
    // AIController가 같은 AnimState라도 다시 재생하게 만드는 재생 serial 키다.
    string _animReplaySerialKey = "AnimReplaySerial";
    string _defaultHitAnimState = "Hit";

    bool   _requestAnimEnd = false;
    bool   _startedHit = false;

    int32 _activeHitSerial = 0;

private:
    // 현재 피격에서 재생해야할 애니메이션
    string Resolve_HitAnimState(const Shared<Blackboard>& blackboard) const;

public:
    static Shared<BTTask_Hit> Create();
    Shared<BTNode> Clone() override;
};

NS_END

