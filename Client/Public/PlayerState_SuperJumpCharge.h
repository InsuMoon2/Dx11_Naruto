#pragma once

#include "IPlayerState.h"

NS_BEGIN(Client)

class PlayerState_SuperJumpCharge : public IPlayerState
{
public:
    PlayerState_SuperJumpCharge();
    ~PlayerState_SuperJumpCharge() override;

public:
    void Enter(PlayerStateMachine* state) override;
    void Update(PlayerStateMachine* state, float timeDelta) override;
    void Exit(PlayerStateMachine* state) override;

    EPlayerState Get_StateID() const override { return EPlayerState::SuperJumpCharge; }

private:
    bool    _loopAnimStarted = false;
    // 컨트롤을 떼고 실제 SuperJump로 발사될 때만 종료 사운드를 재생하기 위한 플래그다.
    bool    _shouldPlayChargeEndSound = false;
    // 차지 유지 중 반복 재생할 사운드 파일명이다.
    wstring _chargingLoopSoundFile = L"SuperJump_Charging.wav";
    // 차지 종료 후 SuperJump 시작 직전에 1회 재생할 사운드 파일명이다.
    wstring _chargingEndSoundFile = L"SuperJump_ChargingEnd.wav";

public:
    static Shared<PlayerState_SuperJumpCharge> Create();
};

NS_END
