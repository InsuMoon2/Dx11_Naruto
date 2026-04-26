#pragma once

#include "SkillObject.h"

NS_BEGIN(Engine)
class Shader;
class Model;
NS_END

NS_BEGIN(Client)

class AnimationStateComponent;

class Skill_WoodHand final : public SkillObject
{
    GENERATED_BODY(Skill_WoodHand)

public:
    explicit Skill_WoodHand(ComPtr<Device> device, ComPtr<DeviceContext> context);
    explicit Skill_WoodHand(const Skill_WoodHand& rhs);
    virtual ~Skill_WoodHand() = default;

public:
    HRESULT Initialize_Prototype() override;
    HRESULT Initialize(void* arg) override;

    void    Update(float timeDelta) override;
    void    Late_Update(float timeDelta) override;
    HRESULT Render() override;

private:
    HRESULT Ready_Components();
    HRESULT Ready_AnimState();

    void    Update_AnimationPhase(float timeDelta);

    // 손이 목표 높이까지 올라온 뒤 중앙 충돌체와 임팩트 이펙트를 한 번 스폰할 때 호출된다.
    void    Spawn_Impact();

    Matrix  Build_HandWorldMatrix(bool leftHand) const;
    Vec3    Resolve_ImpactWorldPosition() const;

    HRESULT Render_Model(Shared<Model> model, const Matrix& worldMatrix);
    HRESULT Bind_ShaderResources(const Matrix& worldMatrix);

public:
    void OnBeginOverlap(Shared<Collider> self, Shared<Collider> other) override;

private:
    Shared<Shader>   _shader;
    Shared<Model>    _leftModel;
    Shared<Model>    _rightModel;
    Shared<AnimationStateComponent> _leftAnimState;
    Shared<AnimationStateComponent> _rightAnimState;

private:
    string _leftModelTag = "WoodHand_L";
    string _rightModelTag = "WoodHand_R";

    string _leftAnimStateKey = "WoodHand_L";
    string _rightAnimStateKey = "WoodHand_R";
    string _leftStartAnimationName = "SK_ITM_WoodHandShort_L|WoodHandShort_L_Ninjutsu_LaughingMonk_Start";
    string _leftEndAnimationName = "SK_ITM_WoodHandShort_L|WoodHandShort_L_Ninjutsu_LaughingMonk_End";
    string _rightStartAnimationName = "SK_ITM_WoodHandShort_R|WoodHandShort_R_Ninjutsu_LaughingMonk_Start";
    string _rightEndAnimationName = "SK_ITM_WoodHandShort_R|WoodHandShort_R_Ninjutsu_LaughingMonk_End";

private:
    float _handSpacing = 2.75f;
    float _forwardOffset = 1.8f;

    Vec3 _handScale = Vec3(10.f, 10.f, 10.f);

    float _riseStartOffsetY = 5.f;

    float _riseDuration = 1.6f;

    // 손 상승이 끝난 뒤 임팩트를 몇 초 늦게 터뜨릴지 정하는 지연값이다.
    float _impactDelayAfterRise = 0.1667f;

    string _impactEffectName = "WoodHand_Impact";

    float _impactActiveDuration = 0.18f;

    float _impactRadius = 4.f;

    Vec3 _impactEffectScale = Vec3(1.f, 1.f, 1.f);

    bool _impactSpawned = false;
    float _impactActiveRemain = 0.f;
    float _skillElapsedTime = 0.f;

public:
    static Shared<GameObject> Create(ComPtr<Device> device, ComPtr<DeviceContext> context);
    Shared<GameObject> Clone(void* arg) override;
    void Free() override;
};

NS_END
