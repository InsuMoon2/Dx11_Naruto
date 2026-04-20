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
    float  _initialFireDelay = 0.25f;
    float   _burstInterval = 1.12f;

    int32   _maxBurstCount = 5;

    float   _forwardRange = 14.f;
    float   _impactRadius = 10.f;
    float   _armSpeed = 130.f;

    Vec3    _burstCenter = Vec3::Zero;

    float   _delayElapsed = 0.f; // 소환 후 경과시간
    int32   _burstCount = 0;

    bool    _isBurstFinished = false;

    float _armSpawnRadius = 10.f;
    float _armSpawnHeight = 8.f;

public:
    static Shared<GameObject> Create(ComPtr<Device> device, ComPtr<DeviceContext> context);
    Shared<GameObject> Clone(void* arg) override;
    void Free() override;

};

NS_END
