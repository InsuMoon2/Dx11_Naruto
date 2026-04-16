#include "pch.h"
#include "AnimationStateComponent.h"
#include "GameObject.h"
#include "Model.h"
#include "PlayerStateMachine.h"
#include <unordered_set>

#include "PlayerState_Attack.h"
#include "ComboProfile_Manager.h"
#include "EquipmentComponent.h"

// 애니메이션 이름 목록을 보기 좋게 정렬하기 위한 대소문자 무시 비교 함수다.
static bool Compare_AnimationNameCaseInsensitive(const string& lhs, const string& rhs)
{
    return std::lexicographical_compare(
        lhs.begin(), lhs.end(),
        rhs.begin(), rhs.end(),
        [](char l, char r)
        {
            return std::tolower(static_cast<unsigned char>(l)) <
                   std::tolower(static_cast<unsigned char>(r));
        });
}

AnimationStateComponent::AnimationStateComponent(ComPtr<Device> device, ComPtr<DeviceContext> context)
    : Component(device, context)
{
}

AnimationStateComponent::AnimationStateComponent(const AnimationStateComponent& rhs)
    : Component(rhs)
    , _stateAnimations(rhs._stateAnimations)
    , _currentStateName("")
    , _prevStateName("")
{
}

HRESULT AnimationStateComponent::Initialize_Prototype()
{
    return Component::Initialize_Prototype();
}

HRESULT AnimationStateComponent::Initialize(void* arg)
{
    return Component::Initialize(arg);
}

void AnimationStateComponent::BeginPlay()
{
    Component::BeginPlay();

    _model = Resolve_Model();
}

Shared<Model> AnimationStateComponent::Resolve_Model()
{
    auto owner = Get_Owner();
    if (!owner)
        return nullptr;

    return owner->Get_Component<Model>();
}

bool AnimationStateComponent::Play_State(const string& stateKey)
{
    if (!_model)
        _model = Resolve_Model();

    if (!_model || stateKey.empty())
        return false;

    const auto* animDesc = Find_State(stateKey);
    if (!animDesc)
        return false;

    _prevStateName = _currentStateName;
    _currentStateName = stateKey;

    if (animDesc->mode == EStateAnimationMode::Sequence)
    {
        if (!animDesc->Has_Sequence())
            return false;

        _model->Set_AnimationSequence(animDesc->start, animDesc->loop, animDesc->end);
        return true;
    }

    if (animDesc->mode == EStateAnimationMode::DirectionalSingle)
        return false;

    if (animDesc->single.animationName.empty())
        return false;

    _model->Set_Animation(animDesc->single);
    return true;
}

bool AnimationStateComponent::Play_DirectionalState(const string& stateName, EMoveInputDirection dir)
{
    if (!_model)
        _model = Resolve_Model();

    if (!_model || stateName.empty())
        return false;

    const auto* animDesc = Find_State(stateName);
    if (!animDesc)
        return false;

    if (animDesc->mode != EStateAnimationMode::DirectionalSingle)
        return false;

    const FAnimationClipSetting* clip = animDesc->directional.Find(dir);
    if (!clip || clip->animationName.empty())
        return false;

    _prevStateName = _currentStateName;
    _currentStateName = stateName;

    _model->Set_Animation(*clip);
    return true;
}

bool AnimationStateComponent::Play_StateLoopOnly(const string& stateName)
{
    if (!_model)
        _model = Resolve_Model();

    if (!_model || stateName.empty())
        return false;

    const auto* desc = Find_State(stateName);
    if (!desc)
        return false;

    if (desc->mode != EStateAnimationMode::Sequence)
        return Play_State(stateName);

    FAnimationClipSetting emptyStart = desc->start;
    emptyStart.animationName.clear();

    _prevStateName = _currentStateName;
    _currentStateName = stateName;

    _model->Set_AnimationSequence(emptyStart, desc->loop, desc->end);
    return true;
}

void AnimationStateComponent::Request_StateEnd()
{
    if (!_model)
        _model = Resolve_Model();

    if (_model)
        _model->Request_AnimEnd();
}

bool AnimationStateComponent::Is_CurrentStateFinished() const
{
    return _model ? _model->Is_CurrentAnimationFinished() : false;
}

bool AnimationStateComponent::Is_CurrentStateSequenceFinished() const
{
    return _model ? _model->Is_AnimationSequenceFinished() : false;
}

EAnimPhase AnimationStateComponent::Get_CurrentAnimPhase() const
{
    return _model ? _model->Get_AnimPhase() : EAnimPhase::Start;
}

float AnimationStateComponent::Get_CurrentTrackPositionTicks() const
{
    return _model ? _model->Get_CurrentTrackPositionTicks() : 0.f;
}

float AnimationStateComponent::Get_CurrentAnimationDurationTicks() const
{
    return _model ? _model->Get_CurrentAnimationDurationTicks() : 0.f;
}

float AnimationStateComponent::Get_CurrentTrackPositionSec() const
{
    return _model ? _model->Get_CurrentTrackPositionSec() : 0.f;
}

float AnimationStateComponent::Get_CurrentAnimationDurationSec() const
{
    return _model ? _model->Get_CurrentAnimationDurationSec() : 0.f;
}

const FStateAnimationDesc* AnimationStateComponent::Find_State(const string& stateName) const
{
    auto iter = _stateAnimations.find(stateName);
    if (iter == _stateAnimations.end())
        return nullptr;

    return &iter->second;
}

FStateAnimationDesc& AnimationStateComponent::Edit_State(const string& stateName)
{
    return _stateAnimations[stateName];
}

vector<string> AnimationStateComponent::Get_StateNames() const
{
    vector<string> result;
    result.reserve(_stateAnimations.size());

    for (const auto& [key, value] : _stateAnimations)
    {
        result.push_back(key);
    }

    sort(result.begin(), result.end());
    return result;
}

vector<string> AnimationStateComponent::Get_ModelAnimationNames() const
{
    vector<string> result;

    if (!_model)
        return result;

    const uint32 count = _model->Get_AnimationCount();
    result.reserve(count);
    unordered_set<string> uniqueNames;
    uniqueNames.reserve(count);

    for (uint32 i = 0; i < count; ++i)
    {
        const string& name = _model->Get_AnimationName(i);
        if (name.empty())
            continue;

        // Model 단계에서 한 번 정리되더라도, 인스펙터 표시에서는 마지막으로 한 번 더 중복을 막는다.
        if (!uniqueNames.insert(name).second)
            continue;

        result.push_back(name);
    }

    // 애니메이션 컴포넌트 리스트에서는 표시 순서만 정렬하고 실제 모델 인덱스는 바꾸지 않는다.
    sort(result.begin(), result.end(), Compare_AnimationNameCaseInsensitive);

    return result;
}

bool AnimationStateComponent::Preview_State(const string& stateName, int32 slotIndex, EMoveInputDirection dir)
{
    if (!_model)
        _model = Resolve_Model();

    if (!_model)
        return false;

    const auto* animDesc = Find_State(stateName);
    if (!animDesc)
        return false;

    if (animDesc->mode == EStateAnimationMode::Sequence)
    {
        const FAnimationClipSetting* clip = nullptr;

        if (slotIndex == 0)
            clip = &animDesc->start;
        else if (slotIndex == 1)
            clip = &animDesc->loop;
        else
            clip = &animDesc->end;

        if (!clip || clip->animationName.empty())
            return false;

        _model->Set_Animation(*clip);
        return true;
    }

    if (animDesc->mode == EStateAnimationMode::DirectionalSingle)
    {
        const FAnimationClipSetting* clip = nullptr;

        if (slotIndex == 0)
            clip = &animDesc->directional.forward;
        else if (slotIndex == 1)
            clip = &animDesc->directional.backward;
        else if (slotIndex == 2)
            clip = &animDesc->directional.left;
        else
            clip = &animDesc->directional.right;

        if (!clip || clip->animationName.empty())
            return false;

        _model->Set_Animation(*clip);
        return true;
    }

    if (animDesc->single.animationName.empty())
        return false;

    _model->Set_Animation(animDesc->single);
    return true;
}

bool AnimationStateComponent::Remove_State(const string& stateName)
{
    auto iter = _stateAnimations.find(stateName);
    if (iter == _stateAnimations.end())
        return false;

    _stateAnimations.erase(iter);

    if (_currentStateName == stateName)
        _currentStateName.clear();

    if (_prevStateName == stateName)
        _prevStateName.clear();

    return true;
}


json AnimationStateComponent::To_Json() const
{
    json root = Component::To_Json();
    root["current_state"] = _currentStateName;

    json stateArray = json::array();

    for (const auto& [stateKey, desc] : _stateAnimations)
    {
        json item;
        item["key"] = stateKey;
        item["mode"] = string(magic_enum::enum_name(desc.mode));

        item["single"] = {
            { "animationName", desc.single.animationName },
            { "loop", desc.single.loop },
            { "playRate", desc.single.playRate }
        };

        item["start"] = {
            { "animationName", desc.start.animationName },
            { "loop", desc.start.loop },
            { "playRate", desc.start.playRate }
        };

        item["loopClip"] = {
            { "animationName", desc.loop.animationName },
            { "loop", desc.loop.loop },
            { "playRate", desc.loop.playRate }
        };

        item["end"] = {
            { "animationName", desc.end.animationName },
            { "loop", desc.end.loop },
            { "playRate", desc.end.playRate }
        };

        item["directional"] = {
            { "forward", {
                { "animationName", desc.directional.forward.animationName },
                { "loop", desc.directional.forward.loop },
                { "playRate", desc.directional.forward.playRate }
            } },
            { "backward", {
                { "animationName", desc.directional.backward.animationName },
                { "loop", desc.directional.backward.loop },
                { "playRate", desc.directional.backward.playRate }
            } },
            { "left", {
                { "animationName", desc.directional.left.animationName },
                { "loop", desc.directional.left.loop },
                { "playRate", desc.directional.left.playRate }
            } },
            { "right", {
                { "animationName", desc.directional.right.animationName },
                { "loop", desc.directional.right.loop },
                { "playRate", desc.directional.right.playRate }
            } }
        };

        stateArray.push_back(item);
    }

    root["animation_states"] = stateArray;
    return root;
}

void AnimationStateComponent::From_Json(const json& data)
{
    Component::From_Json(data);

    _stateAnimations.clear();
    _currentStateName = data.value("current_state", "");

    if (!data.contains("animation_states") || !data["animation_states"].is_array())
        return;

    for (const auto& item : data["animation_states"])
    {
        const string stateKey = item.value("key", "");
        if (stateKey.empty())
            continue;

        FStateAnimationDesc desc;

        const string modeName = item.value("mode", "Single");
        const auto modeOpt = magic_enum::enum_cast<EStateAnimationMode>(modeName);
        desc.mode = modeOpt.value_or(EStateAnimationMode::Single);

        if (item.contains("single"))
        {
            const auto& single = item["single"];
            desc.single.animationName = single.value("animationName", "");
            desc.single.loop = single.value("loop", true);
            desc.single.playRate = single.value("playRate", 1.f);
        }

        if (item.contains("start"))
        {
            const auto& start = item["start"];
            desc.start.animationName = start.value("animationName", "");
            desc.start.loop = start.value("loop", false);
            desc.start.playRate = start.value("playRate", 1.f);
        }

        if (item.contains("loopClip"))
        {
            const auto& loopClip = item["loopClip"];
            desc.loop.animationName = loopClip.value("animationName", "");
            desc.loop.loop = loopClip.value("loop", true);
            desc.loop.playRate = loopClip.value("playRate", 1.f);
        }

        if (item.contains("end"))
        {
            const auto& end = item["end"];
            desc.end.animationName = end.value("animationName", "");
            desc.end.loop = end.value("loop", false);
            desc.end.playRate = end.value("playRate", 1.f);
        }

        if (item.contains("directional"))
        {
            const auto& directional = item["directional"];

            if (directional.contains("forward"))
            {
                const auto& forward = directional["forward"];
                desc.directional.forward.animationName = forward.value("animationName", "");
                desc.directional.forward.loop = forward.value("loop", false);
                desc.directional.forward.playRate = forward.value("playRate", 1.f);
            }

            if (directional.contains("backward"))
            {
                const auto& backward = directional["backward"];
                desc.directional.backward.animationName = backward.value("animationName", "");
                desc.directional.backward.loop = backward.value("loop", false);
                desc.directional.backward.playRate = backward.value("playRate", 1.f);
            }

            if (directional.contains("left"))
            {
                const auto& left = directional["left"];
                desc.directional.left.animationName = left.value("animationName", "");
                desc.directional.left.loop = left.value("loop", false);
                desc.directional.left.playRate = left.value("playRate", 1.f);
            }

            if (directional.contains("right"))
            {
                const auto& right = directional["right"];
                desc.directional.right.animationName = right.value("animationName", "");
                desc.directional.right.loop = right.value("loop", false);
                desc.directional.right.playRate = right.value("playRate", 1.f);
            }
        }

        _stateAnimations[stateKey] = desc;
    }
}

void AnimationStateComponent::Capture_FromStateMachine(const Shared<PlayerStateMachine> stateMachine)
{
    if (!stateMachine)
        return;

    // 현재 FSM 기준 먼저 읽기
    const EPlayerState localState = stateMachine->Get_CurrentStateID();
    const Protocol::OBJECT_STATE_TYPE replicatedState = To_ReplicatedState(localState);

    const EMoveInputDirection nextDir = stateMachine->Get_PendingMoveInputDirection();
    const EAnimPhase nextPhase = stateMachine->Get_AnimPhase();

    const bool stateChanged = (_replicatedState.state != replicatedState);

    _replicatedState.state = replicatedState;
    _replicatedState.dir = nextDir;
    _replicatedState.phase = nextPhase;

    _replicatedState.forceRestart = stateChanged && Requires_ForceRestart(localState);

    // 다음 변경 상태가 Attack일 때에만 프로파일 체크
    if (localState == EPlayerState::Attack)
    {
        auto attackState = dynamic_pointer_cast<PlayerState_Attack>(
            stateMachine->Get_CurrentState());

        if (attackState)
        {
            _replicatedState.attackProfile = attackState->Get_ActiveProfileType();
            _replicatedState.attackComboIndex = attackState->Get_ComboIndex();
        }
    }
    else
    {
        _replicatedState.attackProfile = EAttackProfileType::Hand_Ground;
        _replicatedState.attackComboIndex = 0;
    }
}

void AnimationStateComponent::Sync_FromNetwork(const FAnimReplicatedState& state)
{
    _replicatedState = state;
}

void AnimationStateComponent::Apply_NetworkState()
{
    if (!_model)
        _model = Resolve_Model();

    if (!_model)
        return;

    const bool stateChanged =
        _replicatedState.state != _appliedState.state ||
        _replicatedState.dir != _appliedState.dir;

    const bool phaseChanged =
        _replicatedState.phase != _appliedState.phase;

    const bool attackInfoChanged =
        _replicatedState.attackComboIndex != _appliedState.attackComboIndex ||
        _replicatedState.attackProfile != _appliedState.attackProfile;

    if (!stateChanged && !phaseChanged &&
        !_replicatedState.forceRestart && !attackInfoChanged)
        return;

    bool played = false;

    // 수신된 attack_combo_index에 해당하는 animStateKey를 직접 조호ㅣ
    if (attackInfoChanged)
    {
        const FComboProfile* profile =
            GET_SINGLE(ComboProfile_Manager)->Find(_replicatedState.attackProfile);

        if (profile &&
            _replicatedState.attackComboIndex < static_cast<int32>(profile->combos.size()))
        {
            const string& animKey =
                profile->combos[_replicatedState.attackComboIndex].animStateKey;

            played = Play_State(animKey);
        }
    }

    // 위에서 재생되지 않은 경우 기존 로직 그대로 실행
    if (!played)
    {
        const EPlayerState localState = To_LocalState(_replicatedState.state);

        const string stateName = Find_StateNameByWeapon(
            Get_Owner().get(), localState);

        const auto* stateDesc = Find_State(stateName);

        if (_replicatedState.forceRestart)
        {
            if (localState == EPlayerState::Dash)
                played = Play_DirectionalState(stateName, _replicatedState.dir);
            else
                played = Play_State(stateName);
        }
        else if (stateChanged)
        {
            if (localState == EPlayerState::Dash)
                played = Play_DirectionalState(stateName, _replicatedState.dir);
            else
                played = Play_State(stateName);
        }
        else if (phaseChanged && stateDesc &&
            stateDesc->mode == EStateAnimationMode::Sequence)
        {
            if (_replicatedState.phase == EAnimPhase::End)
            {
                Request_StateEnd();
                played = true;
            }
            else if (_appliedState.phase == EAnimPhase::End &&
                _replicatedState.phase == EAnimPhase::Loop)
            {
                played = Play_StateLoopOnly(stateName);
            }
            else
            {
                played = true;
            }
        }
    }

    if (played)
    {
        _appliedState = _replicatedState;
        _replicatedState.forceRestart = false;
    }
}



void AnimationStateComponent::Write_ToObjectInfo(Protocol::ObjectInfo& info) const
{
    info.set_object_state(_replicatedState.state);
    info.set_move_dir(To_ProtoDir(_replicatedState.dir));

    info.set_anim_phase(To_ProtoAnimPhase(_replicatedState.phase));
    info.set_anim_force_restart(_replicatedState.forceRestart);

    info.set_attack_profile(To_ProtoAttackProfile(_replicatedState.attackProfile));
    info.set_attack_combo_index(_replicatedState.attackComboIndex);
}

void AnimationStateComponent::Read_FromObjectInfo(const Protocol::ObjectInfo& info)
{
    FAnimReplicatedState state{};

    state.state = info.object_state();
    state.dir = From_ProtoDir(info.move_dir());

    state.phase = From_ProtoAnimPhase(info.anim_phase());
    state.forceRestart = info.anim_force_restart();

    state.attackProfile = From_ProtoAttackProfile(info.attack_profile());
    state.attackComboIndex = info.attack_combo_index();

    Sync_FromNetwork(state);
}

string AnimationStateComponent::To_AnimationStateName(EPlayerState state)
{
    auto name = magic_enum::enum_name(state);
    return name.empty() ? "" : string(name);
}

string AnimationStateComponent::Find_StateNameByWeapon(GameObject* owner, EPlayerState state)
{
    // 원래 애니메이션 스테이트 이름 가져와서,
    string baseName = To_AnimationStateName(state);
    if (baseName.empty())
        return baseName;

    if (!owner)
        return baseName;

    auto equipment = owner->Get_Component<EquipmentComponent>();
    if (!equipment)
        return baseName;

    // Hand면 그대로
    EWeaponType weaponType = equipment->Get_CurrentWeaponType();
    if (weaponType == EWeaponType::Hand)
        return baseName;

    // BigSword면 접두사 추가, 좋은 방식은 아니긴한데..
    string variantName = "BigSword_" + baseName;

    if (Find_State(variantName) != nullptr)
        return variantName;

    //variant가 없으면 기본 상태명으로
    return baseName;
}

bool AnimationStateComponent::Requires_ForceRestart(EPlayerState state)
{
    switch (state)
    {
    case EPlayerState::Jump:
    case EPlayerState::DoubleJump:
    case EPlayerState::SuperJump:
    case EPlayerState::HeightLand:
    case EPlayerState::Dash:
    case EPlayerState::JumpDash:
    case EPlayerState::WireDash:
    case EPlayerState::AirApproach:
    case EPlayerState::Hit:
    case EPlayerState::JumpAttack:
    case EPlayerState::Skill_Rasengan:
    case EPlayerState::Skill_Rasengan_Air:
    case EPlayerState::Skill_Rasengan_End:
    case EPlayerState::Skill_RasenShuriken:
    case EPlayerState::Skill_RasenShuriken_Air:
    case EPlayerState::Skill_Chidori:
    case EPlayerState::Skill_Chidori_Air:
    case EPlayerState::Skill_Chidori_End:
    case EPlayerState::Skill_FireBall:
    case EPlayerState::Skill_FireBall_Air:
        return true;

    default:
        return false;
    }
}

Protocol::OBJECT_STATE_TYPE AnimationStateComponent::To_ReplicatedState(EPlayerState state)
{
    switch (state)
    {
    case EPlayerState::Idle:                return Protocol::OBJECT_STATE_TYPE_IDLE;
    case EPlayerState::BigSword_Idle:       return Protocol::OBJECT_STATE_TYPE_BIGSWORD_IDLE;
    case EPlayerState::Wall_Idle:           return Protocol::OBJECT_STATE_TYPE_WALL_IDLE;
    case EPlayerState::Run:                 return Protocol::OBJECT_STATE_TYPE_RUN;
    case EPlayerState::Wall_Run:            return Protocol::OBJECT_STATE_TYPE_WALL_RUN;
    case EPlayerState::Jump:                return Protocol::OBJECT_STATE_TYPE_JUMP;
    case EPlayerState::JumpFall:            return Protocol::OBJECT_STATE_TYPE_JUMP_FALL;
    case EPlayerState::DoubleJump:          return Protocol::OBJECT_STATE_TYPE_DOUBLE_JUMP;
    case EPlayerState::JumpDash:            return Protocol::OBJECT_STATE_TYPE_JUMP_DASH;
    case EPlayerState::SuperJumpCharge:     return Protocol::OBJECT_STATE_TYPE_SUPER_JUMP_CHARGE;
    case EPlayerState::SuperJump:           return Protocol::OBJECT_STATE_TYPE_SUPER_JUMP;
    case EPlayerState::HeightLand:          return Protocol::OBJECT_STATE_TYPE_HEIGHT_LAND;
    case EPlayerState::WireDash:            return Protocol::OBJECT_STATE_TYPE_WIRE_DASH;
    case EPlayerState::AirApproach:         return Protocol::OBJECT_STATE_TYPE_AIR_APPROACH;
    case EPlayerState::Attack:              return Protocol::OBJECT_STATE_TYPE_ATTACK;
    case EPlayerState::JumpAttack:          return Protocol::OBJECT_STATE_TYPE_JUMP_ATTACK;
    case EPlayerState::Attack_01:           return Protocol::OBJECT_STATE_TYPE_ATTACK_01;
    case EPlayerState::Attack_02:           return Protocol::OBJECT_STATE_TYPE_ATTACK_02;
    case EPlayerState::Attack_03:           return Protocol::OBJECT_STATE_TYPE_ATTACK_03;
    case EPlayerState::Attack_04:           return Protocol::OBJECT_STATE_TYPE_ATTACK_04;
    case EPlayerState::Attack_Air_01:       return Protocol::OBJECT_STATE_TYPE_ATTACK_AIR_01;
    case EPlayerState::Attack_Air_02:       return Protocol::OBJECT_STATE_TYPE_ATTACK_AIR_02;
    case EPlayerState::Attack_Air_03:       return Protocol::OBJECT_STATE_TYPE_ATTACK_AIR_03;
    case EPlayerState::Attack_Air_04:       return Protocol::OBJECT_STATE_TYPE_ATTACK_AIR_04;
    case EPlayerState::Attack_Sword_01:     return Protocol::OBJECT_STATE_TYPE_ATTACK_SWORD_01;
    case EPlayerState::Attack_Sword_02:     return Protocol::OBJECT_STATE_TYPE_ATTACK_SWORD_02;
    case EPlayerState::Attack_Sword_03:     return Protocol::OBJECT_STATE_TYPE_ATTACK_SWORD_03;
    case EPlayerState::Attack_Sword_04:     return Protocol::OBJECT_STATE_TYPE_ATTACK_SWORD_04;
    case EPlayerState::Attack_SwordAir_01:  return Protocol::OBJECT_STATE_TYPE_ATTACK_SWORD_AIR_01;
    case EPlayerState::Attack_SwordAir_02:  return Protocol::OBJECT_STATE_TYPE_ATTACK_SWORD_AIR_02;
    case EPlayerState::Hit:                 return Protocol::OBJECT_STATE_TYPE_HIT;
    case EPlayerState::Dash:                return Protocol::OBJECT_STATE_TYPE_DASH;
    case EPlayerState::Skill_Rasengan:      return Protocol::OBJECT_STATE_TYPE_SKILL_RASENGAN;
    case EPlayerState::Skill_Rasengan_Air:  return Protocol::OBJECT_STATE_TYPE_SKILL_RASENGAN_AIR;
    case EPlayerState::Skill_Rasengan_End:  return Protocol::OBJECT_STATE_TYPE_SKILL_RASENGAN_END;
    case EPlayerState::Skill_RasenShuriken: return Protocol::OBJECT_STATE_TYPE_SKILL_RASENSHURIKEN;
    case EPlayerState::Skill_RasenShuriken_Air: return Protocol::OBJECT_STATE_TYPE_SKILL_RASENSHURIKEN_AIR;
    case EPlayerState::Skill_Chidori:       return Protocol::OBJECT_STATE_TYPE_SKILL_CHIDORI;
    case EPlayerState::Skill_Chidori_Air:   return Protocol::OBJECT_STATE_TYPE_SKILL_CHIDORI_AIR;
    case EPlayerState::Skill_Chidori_End:   return Protocol::OBJECT_STATE_TYPE_SKILL_CHIDORI_END;
    case EPlayerState::Skill_FireBall:      return Protocol::OBJECT_STATE_TYPE_SKILL_FIREBALL;
    case EPlayerState::Skill_FireBall_Air:  return Protocol::OBJECT_STATE_TYPE_SKILL_FIREBALL_AIR;
    case EPlayerState::Dead:                return Protocol::OBJECT_STATE_TYPE_DEAD;
    default:                                return Protocol::OBJECT_STATE_TYPE_IDLE;
    }
}

EPlayerState AnimationStateComponent::To_LocalState(Protocol::OBJECT_STATE_TYPE state)
{
    switch (state)
    {
    case Protocol::OBJECT_STATE_TYPE_IDLE:                  return EPlayerState::Idle;
    case Protocol::OBJECT_STATE_TYPE_BIGSWORD_IDLE:         return EPlayerState::BigSword_Idle;
    case Protocol::OBJECT_STATE_TYPE_WALL_IDLE:             return EPlayerState::Wall_Idle;
    case Protocol::OBJECT_STATE_TYPE_RUN:                   return EPlayerState::Run;
    case Protocol::OBJECT_STATE_TYPE_WALL_RUN:              return EPlayerState::Wall_Run;
    case Protocol::OBJECT_STATE_TYPE_JUMP:                  return EPlayerState::Jump;
    case Protocol::OBJECT_STATE_TYPE_JUMP_FALL:             return EPlayerState::JumpFall;
    case Protocol::OBJECT_STATE_TYPE_DOUBLE_JUMP:           return EPlayerState::DoubleJump;
    case Protocol::OBJECT_STATE_TYPE_JUMP_DASH:             return EPlayerState::JumpDash;
    case Protocol::OBJECT_STATE_TYPE_SUPER_JUMP_CHARGE:     return EPlayerState::SuperJumpCharge;
    case Protocol::OBJECT_STATE_TYPE_SUPER_JUMP:            return EPlayerState::SuperJump;
    case Protocol::OBJECT_STATE_TYPE_HEIGHT_LAND:           return EPlayerState::HeightLand;
    case Protocol::OBJECT_STATE_TYPE_WIRE_DASH:             return EPlayerState::WireDash;
    case Protocol::OBJECT_STATE_TYPE_AIR_APPROACH:          return EPlayerState::AirApproach;
    case Protocol::OBJECT_STATE_TYPE_ATTACK:                return EPlayerState::Attack;
    case Protocol::OBJECT_STATE_TYPE_JUMP_ATTACK:           return EPlayerState::JumpAttack;
    case Protocol::OBJECT_STATE_TYPE_ATTACK_01:             return EPlayerState::Attack_01;
    case Protocol::OBJECT_STATE_TYPE_ATTACK_02:             return EPlayerState::Attack_02;
    case Protocol::OBJECT_STATE_TYPE_ATTACK_03:             return EPlayerState::Attack_03;
    case Protocol::OBJECT_STATE_TYPE_ATTACK_04:             return EPlayerState::Attack_04;
    case Protocol::OBJECT_STATE_TYPE_ATTACK_AIR_01:         return EPlayerState::Attack_Air_01;
    case Protocol::OBJECT_STATE_TYPE_ATTACK_AIR_02:         return EPlayerState::Attack_Air_02;
    case Protocol::OBJECT_STATE_TYPE_ATTACK_AIR_03:         return EPlayerState::Attack_Air_03;
    case Protocol::OBJECT_STATE_TYPE_ATTACK_AIR_04:         return EPlayerState::Attack_Air_04;
    case Protocol::OBJECT_STATE_TYPE_ATTACK_SWORD_01:       return EPlayerState::Attack_Sword_01;
    case Protocol::OBJECT_STATE_TYPE_ATTACK_SWORD_02:       return EPlayerState::Attack_Sword_02;
    case Protocol::OBJECT_STATE_TYPE_ATTACK_SWORD_03:       return EPlayerState::Attack_Sword_03;
    case Protocol::OBJECT_STATE_TYPE_ATTACK_SWORD_04:       return EPlayerState::Attack_Sword_04;
    case Protocol::OBJECT_STATE_TYPE_ATTACK_SWORD_AIR_01:   return EPlayerState::Attack_SwordAir_01;
    case Protocol::OBJECT_STATE_TYPE_ATTACK_SWORD_AIR_02:   return EPlayerState::Attack_SwordAir_02;
    case Protocol::OBJECT_STATE_TYPE_HIT:                   return EPlayerState::Hit;
    case Protocol::OBJECT_STATE_TYPE_DASH:                  return EPlayerState::Dash;
    case Protocol::OBJECT_STATE_TYPE_SKILL_RASENGAN:        return EPlayerState::Skill_Rasengan;
    case Protocol::OBJECT_STATE_TYPE_SKILL_RASENGAN_AIR:    return EPlayerState::Skill_Rasengan_Air;
    case Protocol::OBJECT_STATE_TYPE_SKILL_RASENGAN_END:    return EPlayerState::Skill_Rasengan_End;
    case Protocol::OBJECT_STATE_TYPE_SKILL_RASENSHURIKEN:   return EPlayerState::Skill_RasenShuriken;
    case Protocol::OBJECT_STATE_TYPE_SKILL_RASENSHURIKEN_AIR:return EPlayerState::Skill_RasenShuriken_Air;
    case Protocol::OBJECT_STATE_TYPE_SKILL_CHIDORI:         return EPlayerState::Skill_Chidori;
    case Protocol::OBJECT_STATE_TYPE_SKILL_CHIDORI_AIR:     return EPlayerState::Skill_Chidori_Air;
    case Protocol::OBJECT_STATE_TYPE_SKILL_CHIDORI_END:     return EPlayerState::Skill_Chidori_End;
    case Protocol::OBJECT_STATE_TYPE_SKILL_FIREBALL:        return EPlayerState::Skill_FireBall;
    case Protocol::OBJECT_STATE_TYPE_SKILL_FIREBALL_AIR:    return EPlayerState::Skill_FireBall_Air;
    case Protocol::OBJECT_STATE_TYPE_DEAD:                  return EPlayerState::Dead;
    default:                                                return EPlayerState::Idle;
    }
}


Protocol::MOVE_INPUT_DIR_TYPE AnimationStateComponent::To_ProtoDir(EMoveInputDirection dir)
{
    switch (dir)
    {
    case EMoveInputDirection::Forward:  return Protocol::MOVE_INPUT_DIR_TYPE_FORWARD;
    case EMoveInputDirection::Backward: return Protocol::MOVE_INPUT_DIR_TYPE_BACKWARD;
    case EMoveInputDirection::Left:     return Protocol::MOVE_INPUT_DIR_TYPE_LEFT;
    case EMoveInputDirection::Right:    return Protocol::MOVE_INPUT_DIR_TYPE_RIGHT;
    default:                            return Protocol::MOVE_INPUT_DIR_TYPE_FORWARD;
    }
}

Protocol::ANIM_PHASE_TYPE AnimationStateComponent::To_ProtoAnimPhase(EAnimPhase phase)
{
    switch (phase)
    {
    case EAnimPhase::Start: return Protocol::ANIM_PHASE_START;
    case EAnimPhase::Loop:  return Protocol::ANIM_PHASE_LOOP;
    case EAnimPhase::End:   return Protocol::ANIM_PHASE_END;
    default:                return Protocol::ANIM_PHASE_START;
    }
}

Protocol::ATTACK_PROFILE_TYPE AnimationStateComponent::To_ProtoAttackProfile(EAttackProfileType profileType)
{
    switch (profileType)
    {
    case EAttackProfileType::Hand_Ground:       return Protocol::ATTACK_PROFILE_TYPE_HAND_GROUND;
    case EAttackProfileType::Hand_Aerial:       return Protocol::ATTACK_PROFILE_TYPE_HAND_AERIAL;
    case EAttackProfileType::BigSword_Ground:   return Protocol::ATTACK_PROFILE_TYPE_BIGSWORD_GROUND;
    case EAttackProfileType::BigSword_Aerial:   return Protocol::ATTACK_PROFILE_TYPE_BIGSWORD_AERIAL;
    default:                                    return Protocol::ATTACK_PROFILE_TYPE_HAND_GROUND;
    }
}

EMoveInputDirection AnimationStateComponent::From_ProtoDir(Protocol::MOVE_INPUT_DIR_TYPE dir)
{
    switch (dir)
    {
    case Protocol::MOVE_INPUT_DIR_TYPE_FORWARD:  return EMoveInputDirection::Forward;
    case Protocol::MOVE_INPUT_DIR_TYPE_BACKWARD: return EMoveInputDirection::Backward;
    case Protocol::MOVE_INPUT_DIR_TYPE_LEFT:     return EMoveInputDirection::Left;
    case Protocol::MOVE_INPUT_DIR_TYPE_RIGHT:    return EMoveInputDirection::Right;
    default:                                     return EMoveInputDirection::Forward;
    }
}

EAnimPhase AnimationStateComponent::From_ProtoAnimPhase(Protocol::ANIM_PHASE_TYPE phase)
{
    switch (phase)
    {
    case Protocol::ANIM_PHASE_START: return EAnimPhase::Start;
    case Protocol::ANIM_PHASE_LOOP:  return EAnimPhase::Loop;
    case Protocol::ANIM_PHASE_END:   return EAnimPhase::End;
    default:                              return EAnimPhase::Start;
    }
}

EAttackProfileType AnimationStateComponent::From_ProtoAttackProfile(Protocol::ATTACK_PROFILE_TYPE profileType)
{
    switch (profileType)
    {
    case Protocol::ATTACK_PROFILE_TYPE_HAND_GROUND:     return EAttackProfileType::Hand_Ground;
    case Protocol::ATTACK_PROFILE_TYPE_HAND_AERIAL:     return EAttackProfileType::Hand_Aerial;
    case Protocol::ATTACK_PROFILE_TYPE_BIGSWORD_GROUND: return EAttackProfileType::BigSword_Ground;
    case Protocol::ATTACK_PROFILE_TYPE_BIGSWORD_AERIAL: return EAttackProfileType::BigSword_Aerial;
    default:                                            return EAttackProfileType::Hand_Ground;
    }
}

Shared<AnimationStateComponent> AnimationStateComponent::Create(ComPtr<Device> device, ComPtr<DeviceContext> context)
{
    auto instance = make_shared<AnimationStateComponent>(device, context);

    if (FAILED(instance->Initialize_Prototype()))
    {
        MSG_BOX("Failed to Create : AnimationStateComponent");

        return nullptr;
    }

    return instance;
}

Shared<Component> AnimationStateComponent::Clone(void* arg)
{
    auto clone = make_shared<AnimationStateComponent>(*this);

    if (FAILED(clone->Initialize(arg)))
    {
        MSG_BOX("Failed to Clone : AnimationStateComponent");

        return nullptr;
    }

    return clone;
}

void AnimationStateComponent::Free()
{
    Component::Free();
}
