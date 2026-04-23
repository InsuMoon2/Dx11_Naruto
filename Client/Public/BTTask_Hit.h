#pragma once

#include "BTTask.h"

NS_BEGIN(Client)

class BTTask_Hit : public BTTask
{
    GENERATED_BT_REFLECTION(BTTask_Hit)

public:
    enum class EHitAnimSelectMode
    {
        Single,
        RoundRobin,
        Random,
        END
    };

public:
    explicit BTTask_Hit();
    explicit BTTask_Hit(const BTTask_Hit& rhs);
    virtual ~BTTask_Hit();

public:
    void Initialize() override;
    EBTNodeResult Update(float timeDelta) override;

    private:
    // 현재 몬스터가 공중인지 판단해서 애니메이션 재생되게
    bool Is_AirborneHit(const Shared<GameObject>& owner) const;
    vector<string> Build_HitAnimStateList(bool isAirborne) const;
    string Select_HitAnimState(const Shared<Blackboard>& blackboard, bool isAirborne) const;

    // 최종 피격 애니메이션 세팅 -> 블랙보드, HitAnim Override 등등 다 고려해서 나온 최종값
    string Resolve_HitAnimState(
        const Shared<Blackboard>& blackboard,
        const Shared<GameObject>& owner) const;

private:
    string _hitFlagKey = "IsHit";
    string _hitAnimStateKey = "HitAnimState";

    string _hitSerialKey = "HitReactionSerial";
    string _animReplaySerialKey = "AnimReplaySerial";

    string _groundHitAnimState01 = "Hit";
    string _groundHitAnimState02 = "Hit";
    string _groundHitAnimState03 = "Hit";

    string _airHitAnimState01 = "Hit_Air";
    string _airHitAnimState02 = "";
    string _airHitAnimState03 = "";

    // 지상/공중
    string _groundHitCycleIndexKey = "GroundHitCycleIndex";
    string _airHitCycleIndexKey = "AirHitCycleIndex";

    // 어떤 방식으로 피격 애니메이션을 고를지
    EHitAnimSelectMode _hitAnimSelectMode = EHitAnimSelectMode::RoundRobin;

    // true면 블랙보드 HitAnimState가 비어있지 않을 때 그 값을 우선해서 사용
    bool   _useBlackboardHitAnimOverride = false;
    bool   _requestAnimEnd = false;
    bool   _startedHit = false;

    int32 _activeHitSerial = 0;


public:
    static Shared<BTTask_Hit> Create();
    Shared<BTNode> Clone() override;
};

NS_END

