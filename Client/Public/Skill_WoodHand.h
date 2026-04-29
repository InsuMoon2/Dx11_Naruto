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

    // Wood Hand가 솟아오르는 순간 중앙 바닥과 좌우 손 위치 바닥에 Test_Smoke를 한 번씩 깔아 줄 때 호출된다.
    void    Spawn_ImpactSmokeBurst();

    // Wood Hand가 솟아오르는 순간 중앙 바닥과 좌우 손 위치 바닥에서 돌 파편을 함께 튀길 때 호출된다.
    void    Spawn_ImpactDebrisBurst();

    Matrix  Build_HandWorldMatrix(bool leftHand) const;
    Vec3    Resolve_ImpactWorldPosition() const;

    // 좌우 손 월드 위치를 구한 뒤 연막을 바닥 높이에 맞춰 투영할 때 호출된다.
    Vec3    Resolve_HandSmokeWorldPosition(bool leftHand) const;

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
    float _impactDelayAfterRise = 1.35f;

    string _impactEffectName = "WoodHand_Impact";

    float _impactActiveDuration = 0.18f;

    float _impactRadius = 4.f;

    Vec3 _impactEffectScale = Vec3(2.f, 2.f, 2.f);

    string _impactSmokeEffectName = "Test_Smoke"; // Wood Hand 등장 순간 바닥과 좌우 손 자리에서 함께 터뜨릴 연막 이펙트 이름이다.
    Vec3 _impactSmokeEffectScale = Vec3(0.9f, 0.9f, 0.9f); // Wood Hand 크기에 맞게 바닥 연막이 과하지 않게 보이도록 적용할 스케일이다.
    string _impactDebrisEffectName = "SmallRock"; // Wood Hand 등장 순간 바닥에서 함께 튀길 돌 파편 메시 이름이다.
    int32 _impactDebrisCountPerBurst = 6; // 중앙과 좌우 손 자리 각각에서 튀길 돌 파편 개수다.
    float _impactDebrisScatterRadius = 1.1f; // 각 바닥 지점 주변으로 돌 파편 시작 위치를 퍼뜨릴 때 사용할 반경이다.

    bool _impactSpawned = false;
    float _impactActiveRemain = 0.f;
    float _skillElapsedTime = 0.f;

public:
    static Shared<GameObject> Create(ComPtr<Device> device, ComPtr<DeviceContext> context);
    Shared<GameObject> Clone(void* arg) override;
    void Free() override;
};

NS_END
