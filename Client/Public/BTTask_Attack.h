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
    string _attackAnimState = "Attack_04";

    float _attackRange = 2.5f;

    bool _startedAttack = false;

    // 공격 시작 전에 타겟 방향으로 회전할지
    bool _faceTarget = true;

    // Sequence 애니메이션이면 End 요청을 줄지
    bool _requestAnimEnd = false;

public:
    static Shared<BTTask_Attack> Create();
    Shared<BTNode> Clone() override;
};

NS_END
