#pragma once

#include "Component.h"

NS_BEGIN(Engine)
class Model;
NS_END

NS_BEGIN(Client)

class AnimationStateComponent : public Component
{
    GENERATED_COMPONENT(AnimationStateComponent, Protocol::COMPONENT_TYPE_ANIMATION_STATE)

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
    const FStateAnimationDesc* Find_State(const string& stateName) const;
    FStateAnimationDesc& Edit_State(const string& stateName);

    bool    Is_CurrentStateFinished() const;
    bool    Is_CurrentStateSequenceFinished() const;

    EAnimPhase  Get_CurrentAnimPhase() const;
    float       Get_CurrentTrackPosition() const;
    float       Get_CurrentAnimationDuration() const;

    const string& Get_CurrentStateName() const { return _currentStateName; }
    const string& Get_PrevStateName() const { return _prevStateName; }

    vector<string>             Get_StateNames() const;
    vector<string>             Get_ModelAnimationNames() const;

    bool Preview_State(const string& stateName, int32 slotIndex, EMoveInputDirection dir);

public:
    json To_Json() const override;
    void From_Json(const json& data) override;

private:
    Shared<Model> Resolve_Model();

private:
    Shared<Model>                     _model;
    umap<string, FStateAnimationDesc> _stateAnimations;

    string _currentStateName    = "";
    string _prevStateName       = "";

public:
    static Shared<AnimationStateComponent> Create(ComPtr<Device> device, ComPtr<DeviceContext> context);
    Shared<Component> Clone(void* arg) override;
    void Free() override;
};

NS_END
