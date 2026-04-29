#pragma once

#include "SkillObject_Projectile.h"

NS_BEGIN(Engine)
class Shader;
class Model;
NS_END

NS_BEGIN(Client)

class Skill_ShinsuSenju_Arm : public SkillObject_Projectile
{
    GENERATED_BODY(Skill_ShinsuSenju_Arm)

public:
    explicit Skill_ShinsuSenju_Arm(ComPtr<Device> device, ComPtr<DeviceContext> context);
    explicit Skill_ShinsuSenju_Arm(const Skill_ShinsuSenju_Arm& rhs);
    virtual ~Skill_ShinsuSenju_Arm() = default;
    
public:
    HRESULT Initialize_Prototype() override;
    HRESULT Initialize(void* arg) override;

    void    Update(float timeDelta) override;
    void    Late_Update(float timeDelta) override;
    HRESULT Render() override;

    void    Apply_InitialRotation(const Vec3& launchDirection);

    void    OnBeginOverlap(Shared<Collider> self, Shared<Collider> other) override;

private:
    HRESULT Ready_Components();
    HRESULT Bind_ShaderResources();

    // 팔이 바닥 목표 지점에 닿았을 때 돌 파편을 여러 개 튀길 때 호출한다.
    void Spawn_Particle(const string& assetName, int32 spawnIndex);

    // 바닥 충돌 순간 중심과 주변 바닥에 Test_Smoke를 한 번에 퍼뜨릴 때 호출한다.
    void Spawn_GroundImpactSmokeBurst();

    // 바닥 충돌 순간 활성 카메라에 짧은 임팩트 셰이크를 요청할 때 호출한다.
    void Spawn_GroundImpactCameraShake();

private:
    Shared<Shader> _shader;
    Shared<Model>  _model;

private:
    Vec3 _targetPoint = Vec3::Zero;
    bool _hasTargetPoint = false;
    float _stopDistance = 1.0f;

    bool _isStop = false;
    int32 _groundDebrisCount = 5; // 8타 동시 착타 시 MeshDebris 부하를 줄이기 위해 쳐내 (10->5)
    string _groundSmokeEffectName = "Test_Smoke"; 
    Vec3 _groundSmokeEffectScale = Vec3(4.0f, 4.0f, 4.0f); // 연기를 훨씬 크게 (2.4 -> 4.0)
    float _groundSmokeRadius = 2.1f; // 중심 연막 주변에 추가 연막을 배치할 때 사용할 반경이다.
    int32 _groundSmokeBurstCount = 0; // 타수가 많아졌으므로 추가 외곽 연막은 생성하지 않는다 (기존 6 -> 0)
    string _groundImpactShakeTag = "shinsu_senju_ground"; // 진수천수 바닥 충돌 셰이크를 식별할 태그다.
    float _groundImpactShakeDuration = 0.22f; // 바닥 히트 순간 짧고 묵직하게 흔들릴 셰이크 지속 시간이다.
    float _groundImpactShakeFrequency = 23.f; // 바닥 충돌 리듬감을 줄 셰이크 주파수다.
    Vec3 _groundImpactShakePosAmplitude = Vec3(0.04f, 0.08f, 0.03f); // 카메라 위치를 살짝 흔들어 바닥 충돌 충격을 전달할 진폭이다.
    Vec3 _groundImpactShakeRotAmplitudeDeg = Vec3(0.7f, 0.45f, 0.2f); // 카메라 회전을 살짝 흔들어 타격감을 보강할 진폭이다.

public:
    static Shared<GameObject> Create(ComPtr<Device> device, ComPtr<DeviceContext> context);
    Shared<GameObject> Clone(void* arg) override;
    void Free() override;

};

NS_END
