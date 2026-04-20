#pragma once

#include "GameObject.h"

NS_BEGIN(Server)

class Monster : public GameObject
{
public:
    explicit Monster();
    virtual ~Monster();

public:
    void Update(float timeDelta);

    void Initialize_FromSpawn(const Vec3& spawnPos, float spawnYaw);

private:
    static float Convert_RadiansToDegrees(float radians);

    void Enter_State(Protocol::OBJECT_STATE_TYPE state, float durationSec);

    void Update_Idle(float timeDelta);
    void Update_Run(float timeDelta);
    void Update_Attack(float timeDelta);

    void Write_DefaultStat();

private:
    Vec3  _spawnPos = Vec3(0.f, 0.f, 0.f);
    float _spawnYaw = 0.f;

    float _stateElapsed = 0.f;
    float _stateDuration = 0.f;

    float _idleDuration = 1.5f;
    float _runDuration = 2.5f;
    float _attackDuration = 1.0f;

    float _runRadius = 3.5f;
    float _runAngularSpeed = 1.2f;
    float _circleAngle = 0.f;

    bool _pendingForceRestart = true;

    Protocol::OBJECT_STATE_TYPE _currentState = Protocol::OBJECT_STATE_TYPE_IDLE;

public:
    static Shared<Monster> Create();
};

NS_END
