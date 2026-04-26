#pragma once
#include "BTTask.h"

NS_BEGIN(Client)

class BTTask_Dash : public BTTask
{
    GENERATED_BT_REFLECTION(BTTask_Dash)

public:
    explicit BTTask_Dash();
    explicit BTTask_Dash(const BTTask_Dash& rhs);
    virtual ~BTTask_Dash() = default;

public:
    void Initialize() override;

    EBTNodeResult Update(float timeDelta) override;

private:
    string _targetObjectKey   = "TargetObjectKey";
    string _leftAnimState     = "Dodge_Run_Left";
    string _rightAnimState    = "Dodge_Run_Right";

    bool   _faceTarget        = true;
    bool   _requestAnimEnd    = true;

    // 좌우 대시 이동 입력을 유지할 시간이다.
    float  _dashDuration      = 0.35f;

    // MovementComponent에 전달할 좌우 이동 입력 세기다.
    float  _dashMovePower     = 1.f;

    // 좌우 대시가 순수 횡이동으로 보이지 않도록 뒤쪽 방향을 얼마나 섞을지 정한다.
    float  _backwardBlendPower = 0.f;

    // 대시 중 Sprint 플래그를 켤지 여부다.
    bool   _useSprint         = false;

    // 대시 종료 시 이동 입력을 0으로 되돌릴지 여부다.
    bool   _stopMovementOnFinish = true;

    bool   _startedDash       = false;
    float  _elapsedDashTime   = 0.f;
    int32  _selectedDashSide  = 1;
    string _selectedAnimState = "";

public:
    static Shared<BTTask_Dash> Create();
    Shared<BTNode> Clone() override;
};

NS_END
