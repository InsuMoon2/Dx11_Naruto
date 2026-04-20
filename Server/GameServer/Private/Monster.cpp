#include "pch.h"
#include "Monster.h"

Monster::Monster()
{
}

Monster::~Monster()
{
}

float Monster::Convert_RadiansToDegrees(float radians)
{
    return radians * (180.f / 3.14159265358979323846f);
}

void Monster::Initialize_FromSpawn(const Vec3& spawnPos, float spawnYaw)
{
    _spawnPos = spawnPos;
    _spawnYaw = spawnYaw;
    _circleAngle = 0.f;

    auto* pos = info.mutable_pos();
    pos->set_x(spawnPos.x);
    pos->set_y(spawnPos.y);
    pos->set_z(spawnPos.z);

    info.set_rot_y(spawnYaw);
    info.set_move_dir(Protocol::MOVE_INPUT_DIR_TYPE_FORWARD);
    info.set_attack_profile(Protocol::ATTACK_PROFILE_TYPE_HAND_GROUND);
    info.set_attack_combo_index(0);

    Write_DefaultStat();
    Enter_State(Protocol::OBJECT_STATE_TYPE_IDLE, _idleDuration);
}

void Monster::Enter_State(Protocol::OBJECT_STATE_TYPE state, float durationSec)
{
    _currentState = state;
    _stateDuration = durationSec;
    _stateElapsed = 0.f;
    _pendingForceRestart = true;

    info.set_object_state(state);
    info.set_anim_phase(Protocol::ANIM_PHASE_START);
}

void Monster::Write_DefaultStat()
{
    auto* stat = info.mutable_stat();
    stat->set_max_hp(100.f);
    stat->set_current_hp(100.f);
    stat->set_max_mp(0.f);
    stat->set_current_mp(0.f);
    stat->set_attack(10.f);
    stat->set_defense(0.f);
    stat->set_speed(2.f);
}

void Monster::Update_Idle(float timeDelta)
{
    info.set_object_state(Protocol::OBJECT_STATE_TYPE_IDLE);
    info.set_move_dir(Protocol::MOVE_INPUT_DIR_TYPE_FORWARD);

    if (_stateElapsed >= 0.1f)
        info.set_anim_phase(Protocol::ANIM_PHASE_LOOP);

    if (_stateElapsed >= _stateDuration)
        Enter_State(Protocol::OBJECT_STATE_TYPE_RUN, _runDuration);
}

void Monster::Update_Run(float timeDelta)
{
    _circleAngle += _runAngularSpeed * timeDelta;

    Vec3 worldPos = _spawnPos;
    worldPos.x += cosf(_circleAngle) * _runRadius;
    worldPos.z += sinf(_circleAngle) * _runRadius;

    Vec3 tangent(-sinf(_circleAngle), 0.f, cosf(_circleAngle));
    tangent.Normalize();

    auto* pos = info.mutable_pos();
    pos->set_x(worldPos.x);
    pos->set_y(worldPos.y);
    pos->set_z(worldPos.z);

    info.set_rot_y(Convert_RadiansToDegrees(atan2f(tangent.x, tangent.z)));
    info.set_object_state(Protocol::OBJECT_STATE_TYPE_RUN);
    info.set_move_dir(Protocol::MOVE_INPUT_DIR_TYPE_FORWARD);

    if (_stateElapsed >= 0.1f)
        info.set_anim_phase(Protocol::ANIM_PHASE_LOOP);

    if (_stateElapsed >= _stateDuration)
        Enter_State(Protocol::OBJECT_STATE_TYPE_ATTACK, _attackDuration);
}

void Monster::Update_Attack(float timeDelta)
{
    info.set_object_state(Protocol::OBJECT_STATE_TYPE_ATTACK);
    info.set_move_dir(Protocol::MOVE_INPUT_DIR_TYPE_FORWARD);

    if (_stateElapsed >= 0.15f)
        info.set_anim_phase(Protocol::ANIM_PHASE_LOOP);

    if (_stateElapsed >= _stateDuration)
        Enter_State(Protocol::OBJECT_STATE_TYPE_IDLE, _idleDuration);
}

void Monster::Update(float timeDelta)
{
    // [변경] 고정 1/60초 하드코딩 대신 실제 서버 루프 경과 시간을 사용해 과속 회전을 막는다.
    const float safeDelta = max(0.f, min(timeDelta, 0.0333f));

    _stateElapsed += safeDelta;

    // 상태 진입 첫 프레임에만 재시작을 주고, 이후엔 끈다.
    if (_pendingForceRestart)
    {
        info.set_anim_force_restart(true);
        _pendingForceRestart = false;
    }
    else
    {
        info.set_anim_force_restart(false);
    }

    switch (_currentState)
    {
    case Protocol::OBJECT_STATE_TYPE_IDLE:
        Update_Idle(safeDelta);
        break;

    case Protocol::OBJECT_STATE_TYPE_RUN:
        Update_Run(safeDelta);
        break;

    case Protocol::OBJECT_STATE_TYPE_ATTACK:
        Update_Attack(safeDelta);
        break;

    default:
        Update_Idle(safeDelta);
        break;
    }
}

Shared<Monster> Monster::Create()
{
    auto monster = make_shared<Monster>();

    monster->info.set_objectid(s_idGenerator++);
    monster->info.set_objecttype(Protocol::OBJECT_TYPE_MONSTER);

    return monster;
}
