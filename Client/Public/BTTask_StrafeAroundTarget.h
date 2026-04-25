#pragma once

#include "BTTask.h"

NS_BEGIN(Engine)
class Blackboard;
NS_END

NS_BEGIN(Client)

class BTTask_StrafeAroundTarget : public BTTask
{
    GENERATED_BT_REFLECTION(BTTask_StrafeAroundTarget)

public:
    explicit BTTask_StrafeAroundTarget();
    explicit BTTask_StrafeAroundTarget(const BTTask_StrafeAroundTarget& rhs);
    virtual ~BTTask_StrafeAroundTarget() = default;

public:
    void Initialize() override;
    EBTNodeResult Update(float timeDelta) override;

private:
    void Finish_Strafe(const Shared<Blackboard>& blackboard); // 횡이동 종료 또는 실패 시 이동 입력을 정리하기 위해 호출한다.

private:
    string _targetObjectKey = "TargetObjectKey"; // 횡이동 기준이 되는 타겟 오브젝트 블랙보드 키다.
    string _strafeAnimState = "Run"; // 횡이동 중 재생할 이동 애니메이션 상태명이다.

    float _strafeDuration = 0.45f; // 한 번 선택됐을 때 횡이동을 유지할 시간이다.
    float _idealDistance = 3.2f; // 횡이동 중 유지하고 싶은 기준 거리다.
    float _distanceCorrectionStrength = 0.35f; // 기준 거리에서 벗어났을 때 앞뒤 보정 비율이다.

    bool _lockFacingToTarget = true; // 횡이동 중 보스가 타겟을 계속 바라볼지 결정한다.
    bool _useSprint = false; // 횡이동 입력에 스프린트를 같이 줄지 결정한다.
    bool _stopMovementOnFinish = true; // 횡이동 종료 시 이동 입력을 0으로 정리할지 결정한다.

    bool _startedStrafe = false; // 현재 횡이동 태스크가 시작됐는지 기록한다.
    float _elapsedStrafeTime = 0.f; // 횡이동 시작 후 흐른 시간이다.
    int32 _selectedSide = 1; // 이번 횡이동에서 선택된 좌/우 방향이다.

public:
    static Shared<BTTask_StrafeAroundTarget> Create();
    Shared<BTNode> Clone() override;
};

NS_END
