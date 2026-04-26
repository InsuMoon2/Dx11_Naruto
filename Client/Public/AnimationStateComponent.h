#pragma once

#include "Component.h"

NS_BEGIN(Engine)
class Model;
NS_END

NS_BEGIN(Client)

class PlayerStateMachine;

class AnimationStateComponent : public Component
{
    GENERATED_COMPONENT(AnimationStateComponent, Protocol::COMPONENT_TYPE_ANIMATION_STATE)

public:
    struct FAnimReplicatedState final
    {
        //EPlayerState        state = EPlayerState::Idle;
        Protocol::OBJECT_STATE_TYPE state = Protocol::OBJECT_STATE_TYPE_IDLE;

        // 방향도 프로토버프 쪽으로 바꿔줄지
        EMoveInputDirection dir = EMoveInputDirection::Forward;

        EAnimPhase          phase = EAnimPhase::Start;
        bool                forceRestart = false;

        // 공격 프로파일과 콤보 인덱스 -> Attack 상태일 때만
        EAttackProfileType  attackProfile = EAttackProfileType::Hand_Ground;
        int32               attackComboIndex = 0;

        // 실제 재생한 애니메이션 상태 키다. 같은 OBJECT_STATE 내 세부 상태를 구분할 때 사용한다.
        string              animStateKey = "";

        // 피격 상태일 때 어떤 리액션인지 원격 클라에 전달한다.
        EHitReactionType    hitReactionType = EHitReactionType::Default;

        // 같은 피격 애니메이션이 다시 들어와도 재생을 재시작시키기 위한 serial 값이다.
        uint32              hitReactionSerial = 0;
    };

public:
    explicit AnimationStateComponent(ComPtr<Device> device, ComPtr<DeviceContext> context);
    explicit AnimationStateComponent(const AnimationStateComponent& rhs);
    virtual ~AnimationStateComponent() = default;

public:
    HRESULT Initialize_Prototype() override;
    HRESULT Initialize(void* arg) override;
    void    BeginPlay() override;

public:
    // 스킬 오브젝트처럼 한 오브젝트에 여러 Model이 붙는 경우, 이 애니메이션 스테이트가 제어할 Model을 직접 지정한다.
    void    Set_Model(Shared<Model> model);

    // stateKey에 해당하는 애니메이션 재생되게
    bool    Play_State(const string& stateKey);

    bool    Play_DirectionalState(const string& stateName, EMoveInputDirection dir);
    // Start 무시하고 Loop 부터 실행할 때
    bool    Play_StateLoopOnly(const string& stateName);
    // 현재 애니메이션 시퀀스 상태 End 요청
    void    Request_StateEnd();

public:
    const FStateAnimationDesc*  Find_State(const string& stateName) const;
    FStateAnimationDesc&        Edit_State(const string& stateName);

    bool                        Is_CurrentStateFinished() const;
    bool                        Is_CurrentStateSequenceFinished() const;

    EAnimPhase                  Get_CurrentAnimPhase() const;
    float                       Get_CurrentTrackPositionTicks() const;
    float                       Get_CurrentAnimationDurationTicks() const;
    float                       Get_CurrentTrackPositionSec() const;
    float                       Get_CurrentAnimationDurationSec() const;

    const string&               Get_CurrentStateName() const { return _currentStateName; }
    const string&               Get_PrevStateName() const { return _prevStateName; }

    vector<string>              Get_StateNames() const;
    vector<string>              Get_ModelAnimationNames() const;

    bool Preview_State(const string& stateName, int32 slotIndex, EMoveInputDirection dir);

    bool Remove_State(const string& stateName);

public: /* Network */
    // 로컬 플레이어 상태 세팅
    void Capture_FromStateMachine(const Shared<PlayerStateMachine> stateMachine);

    // 네트워크 수신값을 컴포넌트 내부 상태로 세팅
    void Sync_FromNetwork(const FAnimReplicatedState& state);
    void Apply_NetworkState();

    // 패킷 송/수신용
    void Write_ToObjectInfo(Protocol::ObjectInfo& info) const;
    void Read_FromObjectInfo(const Protocol::ObjectInfo& info);

    const FAnimReplicatedState& Get_ReplicatedState() const { return _replicatedState; }

    string Find_StateNameByWeapon(GameObject* owner, EPlayerState state);

public:
    json To_Json() const override;
    void From_Json(const json& data) override;

private:
    static string To_AnimationStateName(EPlayerState state);

    static bool Is_HitState(EPlayerState state);
    static EHitReactionType Resolve_HitReactionType(EPlayerState state);

    static EPlayerState Resolve_HitReactionState(EHitReactionType type);

    static Protocol::HIT_REACTION_TYPE To_ProtoHitReaction(EHitReactionType hitReactionType);
    static EHitReactionType From_ProtoHitReaction(Protocol::HIT_REACTION_TYPE hitReactionType);

    static bool Requires_ForceRestart(EPlayerState state);

    static Protocol::OBJECT_STATE_TYPE      To_ReplicatedState(EPlayerState state);

    static Protocol::MOVE_INPUT_DIR_TYPE    To_ProtoDir(EMoveInputDirection dir);
    static Protocol::ANIM_PHASE_TYPE        To_ProtoAnimPhase(EAnimPhase phase);
    static Protocol::ATTACK_PROFILE_TYPE    To_ProtoAttackProfile(EAttackProfileType profileType);

    static EPlayerState                     To_LocalState(Protocol::OBJECT_STATE_TYPE state);
    static EMoveInputDirection              From_ProtoDir(Protocol::MOVE_INPUT_DIR_TYPE dir);
    static EAnimPhase                       From_ProtoAnimPhase(Protocol::ANIM_PHASE_TYPE phase);
    static EAttackProfileType               From_ProtoAttackProfile(Protocol::ATTACK_PROFILE_TYPE profileType);

    Shared<Model>                           Resolve_Model();

    void                                    Update_LocalHitReactionSerial(const string& stateKey);

private:
    Shared<Model>                     _model;
    umap<string, FStateAnimationDesc> _stateAnimations;

    string _currentStateName    = "";
    string _prevStateName       = "";

private: /* Network */
    FAnimReplicatedState _replicatedState{};
    FAnimReplicatedState _appliedState{};

    // 로컬 플레이어의 마지막 Hit 재생 serial이다.
    uint32 _localHitReactionSerial = 0;


public:
    static Shared<AnimationStateComponent> Create(ComPtr<Device> device, ComPtr<DeviceContext> context);
    Shared<Component> Clone(void* arg) override;
    void Free() override;
};


NS_END
