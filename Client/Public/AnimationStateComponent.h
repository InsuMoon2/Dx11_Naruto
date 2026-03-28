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
        EPlayerState        state = EPlayerState::Idle;
        EMoveInputDirection dir = EMoveInputDirection::Forward;

        EAnimPhase          phase = EAnimPhase::Start;

        // 루프용
        bool                forceRestart = false;
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

public:
    json To_Json() const override;
    void From_Json(const json& data) override;

private:
    static string To_AnimationStateName(EPlayerState state);

    static bool Requires_ForceRestart(EPlayerState state);

    static Protocol::OBJECT_STATE_TYPE      To_ProtoState(EPlayerState state);
    static Protocol::MOVE_INPUT_DIR_TYPE    To_ProtoDir(EMoveInputDirection dir);

    static Protocol::ANIM_PHASE_TYPE        To_ProtoAnimPhase(EAnimPhase phase);

    static EPlayerState                     From_ProtoState(Protocol::OBJECT_STATE_TYPE state);
    static EMoveInputDirection              From_ProtoDir(Protocol::MOVE_INPUT_DIR_TYPE dir);
    static EAnimPhase                       From_ProtoAnimPhase(Protocol::ANIM_PHASE_TYPE phase);

    Shared<Model>                           Resolve_Model();

private:
    Shared<Model>                     _model;
    umap<string, FStateAnimationDesc> _stateAnimations;

    string _currentStateName    = "";
    string _prevStateName       = "";

private: /* Network */
    FAnimReplicatedState _replicatedState{};
    FAnimReplicatedState _appliedState{};


public:
    static Shared<AnimationStateComponent> Create(ComPtr<Device> device, ComPtr<DeviceContext> context);
    Shared<Component> Clone(void* arg) override;
    void Free() override;
};

NS_END
