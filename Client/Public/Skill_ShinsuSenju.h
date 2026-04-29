#pragma once

#include "SkillObject.h"

NS_BEGIN(Engine)
class Shader;
class Model;
NS_END

NS_BEGIN(Client)

class Skill_ShinsuSenju : public SkillObject
{
    GENERATED_BODY(Skill_ShinsuSenju)

public:
    explicit Skill_ShinsuSenju(ComPtr<Device> device, ComPtr<DeviceContext> context);
    explicit Skill_ShinsuSenju(const Skill_ShinsuSenju& rhs);
    virtual ~Skill_ShinsuSenju() = default;
    
public:
    HRESULT Initialize_Prototype() override;
    HRESULT Initialize(void* arg) override;

    void    Update(float timeDelta) override;
    void    Late_Update(float timeDelta) override;
    HRESULT Render() override;

private:
    HRESULT Ready_Components();
    HRESULT Bind_ShaderResources();

    void    Resolve_BurstCenter();
    Vec3    Build_ArmImpactPoint(int32 burstIndex);

    void    Fire_NextArm();

    bool    Resolve_GroundPoint(const Vec3& samplePosition, Vec3& outGroundPoint) const;


    Vec3    Build_ArmSpawnPoint(int32 burstIndex) const;

    Vec2    Get_ArmSpawnPattern(int32 burstIndex) const;
    void    Spawn_Arm(const Vec3& spawnPoint, const Vec3& impactPoint);

private:
    Shared<Shader> _shader;
    Shared<Model>  _model;

private:
    float   _initialFireDelay = 0.45f; // 너무 빨리 시작하는 현상 수정을 위해 발동 선딜레이 증가 (기존 0.15 -> 0.45)
    float   _burstInterval = 0.08f; // 너무 빠른 연타로 프레임 드랍 발생을 예방하기 위해 간격 소폭 증가

    int32   _maxBurstCount = 8; // 성능 밸런스를 위해 15타에서 8타로 축소

    float   _forwardRange = 14.f;
    float   _impactRadius = 12.f; // 타격 범위를 조금 넓힘
    float   _armSpeed = 150.f; // 투사체 속도 증가

    Vec3    _burstCenter = Vec3::Zero;

    float   _delayElapsed = 0.f; // 소환 후 경과시간
    int32   _burstCount = 0;

    bool    _isBurstFinished = false;

    // 더 높고 더 넓게 둥귀래 퍼지도록 반경과 높이 증가 (10->22, 8->22)
    float _armSpawnRadius = 22.f;
    float _armSpawnHeight = 22.f;

public:
    static Shared<GameObject> Create(ComPtr<Device> device, ComPtr<DeviceContext> context);
    Shared<GameObject> Clone(void* arg) override;
    void Free() override;

};

NS_END
