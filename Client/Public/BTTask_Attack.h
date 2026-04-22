#pragma once

#include "BTTask.h"

NS_BEGIN(Client)

class BTTask_Attack : public BTTask
{
    GENERATED_BT_REFLECTION(BTTask_Attack)

public:
    explicit BTTask_Attack();
    explicit BTTask_Attack(const BTTask_Attack& rhs);
    virtual ~BTTask_Attack() = default;

public:
    void Initialize() override;
    EBTNodeResult Update(float timeDelta) override;

    string _targetObjectKey = "TargetObjectKey";

    string _selectedAttackStateKey = "SelectedAttackState";

    // 리플렉션, 에디터에서 세팅이 편하게 그냥 배열로 안만들기
    string _attackAnimState01 = "Attack_01";
    string _attackAnimState02 = "Attack_02";
    string _attackAnimState03 = "Attack_03";
    string _attackAnimState04 = "Attack_04";

    string _attackCycleIndexKey = "AttackCycleIndex";
    bool   _useRoundRobin = true;             // 단일공격 vs 순환공격 체크
    string _selectedAttackAnimState = "";     // 이번 공격 진입에서 확정된 애니메이션 상태 유지

    float _attackRange = 2.5f;
    bool  _startedAttack = false;
    bool  _faceTarget = true;        // 공격 시작 전에 타겟 방향으로 회전할지
    bool  _requestAnimEnd = false;   // Sequence 애니메이션이면 End 요청을 줄지

private:
    vector<string> Build_AttackAnimStateList() const;
    string Select_AttackAnimState(const Shared<Blackboard>& blackboard);

public:
    static Shared<BTTask_Attack> Create();
    Shared<BTNode> Clone() override;
};

NS_END
